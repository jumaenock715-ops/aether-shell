#include "LinuxHardwareService.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QTextStream>
#include <QTimer>
#include <QVariantMap>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

namespace {
QString readText(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    return QString::fromUtf8(file.readAll()).trimmed();
}

int readInt(const QString &path, int fallback = -1)
{
    bool ok = false;
    const int value = readText(path).toInt(&ok);
    return ok ? value : fallback;
}

QStringList deviceDirectories(const QString &root)
{
    QDir dir(root);
    const QFileInfoList entries = dir.entryInfoList(
        QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable, QDir::Name);
    QStringList paths;
    for (const QFileInfo &entry : entries)
        paths.append(entry.absoluteFilePath());
    return paths;
}

QStringList parseControllers(const QString &listing)
{
    QStringList controllers;
    static const QRegularExpression pattern(
        QStringLiteral("^Controller\\s+([0-9A-Fa-f:]{17})\\s*(.*)$"));
    for (const QString &line : listing.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        const auto match = pattern.match(line.trimmed());
        if (match.hasMatch())
            controllers.append(match.captured(1) + QStringLiteral("  ") + match.captured(2).trimmed());
    }
    return controllers;
}

QString commandOutput(const QString &program, const QStringList &args, int timeout = 4000)
{
    QProcess process;
    process.start(program, args);
    if (!process.waitForStarted(timeout) || !process.waitForFinished(timeout)) {
        process.kill();
        return {};
    }
    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}
}

LinuxHardwareService::LinuxHardwareService(QObject *parent) : QObject(parent)
{
    m_bluetoothScanProcess = new QProcess(this);
    connect(m_bluetoothScanProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        const QString output = QString::fromUtf8(m_bluetoothScanProcess->readAllStandardOutput()).trimmed()
            + QLatin1Char('\n')
            + QString::fromUtf8(m_bluetoothScanProcess->readAllStandardError()).trimmed();
        setBluetoothScanning(false);
        refresh();
        static const QRegularExpression failurePattern(
            QStringLiteral("(^|\n)(Failed|Error|No default controller|not available|not ready)\\b"),
            QRegularExpression::CaseInsensitiveOption);
        if (exitStatus != QProcess::NormalExit || exitCode != 0 || failurePattern.match(output).hasMatch()) {
            setError(output.isEmpty() ? QStringLiteral("Bluetooth discovery failed.") : output);
            emit operationFinished(false, m_lastError);
            return;
        }
        setError({});
        emit operationFinished(true, QStringLiteral("Bluetooth discovery finished."));
    });
    connect(m_bluetoothScanProcess, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart) return;
        setBluetoothScanning(false);
        setError(QStringLiteral("Could not start Bluetooth discovery. Check bluetoothctl and the BlueZ service."));
        emit operationFinished(false, m_lastError);
    });

    m_bluetoothPairingProcess = new QProcess(this);
    m_bluetoothPairingTimeout = new QTimer(this);
    m_bluetoothPairingTimeout->setSingleShot(true);
    connect(m_bluetoothPairingTimeout, &QTimer::timeout, this, [this]() {
        if (m_bluetoothPairingProcess->state() != QProcess::NotRunning) {
            m_bluetoothPairingProcess->write("no\n");
            m_bluetoothPairingProcess->write("quit\n");
            m_bluetoothPairingProcess->terminate();
            QTimer::singleShot(1500, m_bluetoothPairingProcess, &QProcess::kill);
        }
        finishBluetoothPairing(false, QStringLiteral("Bluetooth pairing timed out."));
    });
    connect(m_bluetoothPairingProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        m_bluetoothPairingBuffer += m_bluetoothPairingProcess->readAllStandardOutput();
        const QList<QByteArray> lines = m_bluetoothPairingBuffer.split('\n');
        m_bluetoothPairingBuffer = lines.isEmpty() ? QByteArray() : lines.last();
        for (int i = 0; i + 1 < lines.size(); ++i)
            processBluetoothPairingOutput(QString::fromUtf8(lines.at(i)).trimmed());
        if (!m_bluetoothPairingBuffer.isEmpty())
            processBluetoothPairingOutput(QString::fromUtf8(m_bluetoothPairingBuffer).trimmed());
    });
    connect(m_bluetoothPairingProcess, &QProcess::readyReadStandardError, this, [this]() {
        const QString error = QString::fromUtf8(m_bluetoothPairingProcess->readAllStandardError()).trimmed();
        if (!error.isEmpty()) processBluetoothPairingOutput(error);
    });
    connect(m_bluetoothPairingProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        m_bluetoothPairingTimeout->stop();
        if (!m_bluetoothPairing) return;
        const bool success = m_bluetoothPairingSucceeded
            && exitStatus == QProcess::NormalExit && exitCode == 0;
        finishBluetoothPairing(success, success
            ? QStringLiteral("Bluetooth device paired successfully.")
            : QStringLiteral("Bluetooth pairing failed or was rejected."));
        if (success) {
            setError({});
            refresh();
        } else {
            setError(QStringLiteral("Bluetooth pairing failed or was rejected."));
            refresh();
        }
    });
    connect(m_bluetoothPairingProcess, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart || !m_bluetoothPairing) return;
        m_bluetoothPairingTimeout->stop();
        finishBluetoothPairing(false, QStringLiteral("Could not start bluetoothctl for pairing."));
        setError(QStringLiteral("Could not start bluetoothctl for pairing. Check the BlueZ installation."));
    });

    refresh();
    auto *monitor = new QTimer(this);
    connect(monitor, &QTimer::timeout, this, &LinuxHardwareService::refresh);
    monitor->start(30000);
}

void LinuxHardwareService::setError(const QString &message)
{
    if (m_lastError == message) return;
    m_lastError = message;
    emit lastErrorChanged();
}

bool LinuxHardwareService::run(const QString &program, const QStringList &arguments,
                               const QString &successMessage, int timeoutMs)
{
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForStarted(3000) || !process.waitForFinished(timeoutMs)) {
        process.kill();
        setError(program + QStringLiteral(" is unavailable or did not respond."));
        emit operationFinished(false, m_lastError);
        return false;
    }
    const QString standardOutput = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    const QString standardError = QString::fromUtf8(process.readAllStandardError()).trimmed();
    const QString output = standardOutput + QLatin1Char('\n') + standardError;
    static const QRegularExpression failurePattern(
        QStringLiteral("(^|\n)(Failed|Error|No default controller|not available|not ready)\\b"),
        QRegularExpression::CaseInsensitiveOption);
    const bool reportedFailure = failurePattern.match(output).hasMatch();
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0 || reportedFailure) {
        const QString detail = !standardError.isEmpty() ? standardError
            : (!standardOutput.isEmpty() ? standardOutput : program + QStringLiteral(" failed."));
        setError(detail);
        refresh();
        emit operationFinished(false, m_lastError);
        return false;
    }
    setError({});
    emit operationFinished(true, successMessage);
    refresh();
    return true;
}

bool LinuxHardwareService::validBluetoothAddress(const QString &address) const
{
    static const QRegularExpression pattern(QStringLiteral("^[0-9A-Fa-f]{2}(:[0-9A-Fa-f]{2}){5}$"));
    return pattern.match(address.trimmed()).hasMatch();
}

void LinuxHardwareService::setBluetoothScanning(bool scanning)
{
    if (m_bluetoothScanning == scanning) return;
    m_bluetoothScanning = scanning;
    emit bluetoothScanningChanged();
}

void LinuxHardwareService::refresh()
{
    // Controller inventory is distinct from the list of remote Bluetooth devices.
    const QString controllerListing = commandOutput(
        QStringLiteral("bluetoothctl"), {QStringLiteral("list")});
    const QStringList controllers = parseControllers(controllerListing);
    bool anyPowered = false;
    for (const QString &controller : controllers) {
        const QString address = controller.section(QLatin1Char(' '), 0, 0);
        const QString details = commandOutput(QStringLiteral("bluetoothctl"),
                                               {QStringLiteral("show"), address});
        anyPowered = anyPowered || details.contains(QStringLiteral("Powered: yes"));
    }
    const bool available = !controllers.isEmpty();
    if (m_bluetoothControllers != controllers) {
        m_bluetoothControllers = controllers;
        emit bluetoothDevicesChanged();
    }
    if (m_bluetoothAvailable != available || m_bluetoothPowered != anyPowered) {
        m_bluetoothAvailable = available;
        m_bluetoothPowered = anyPowered;
        emit bluetoothChanged();
    }

    QStringList devices;
    QVariantList deviceDetails;
    const QString listing = commandOutput(QStringLiteral("bluetoothctl"), {QStringLiteral("devices")});
    static const QRegularExpression devicePattern(QStringLiteral("^Device\\s+([0-9A-Fa-f:]{17})\\s*(.*)$"));
    for (const QString &line : listing.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        const auto match = devicePattern.match(line.trimmed());
        if (!match.hasMatch()) continue;
        const QString address = match.captured(1);
        const QString name = match.captured(2).trimmed();
        const QString info = commandOutput(QStringLiteral("bluetoothctl"), {QStringLiteral("info"), address});
        const bool paired = info.contains(QStringLiteral("Paired: yes"));
        const bool trusted = info.contains(QStringLiteral("Trusted: yes"));
        const bool connected = info.contains(QStringLiteral("Connected: yes"));
        devices.append(address + QStringLiteral("  ") + name);
        deviceDetails.append(QVariantMap{{QStringLiteral("address"), address},
            {QStringLiteral("name"), name}, {QStringLiteral("paired"), paired},
            {QStringLiteral("trusted"), trusted}, {QStringLiteral("connected"), connected}});
    }
    if (m_bluetoothDevices != devices || m_bluetoothDeviceDetails != deviceDetails) {
        m_bluetoothDevices = devices;
        m_bluetoothDeviceDetails = deviceDetails;
        emit bluetoothDevicesChanged();
    }

    struct BatteryReading {
        QString label;
        int percent;
        double full;
        double now;
        int metric;
        double design;
        QString status;
    };
    QList<BatteryReading> batteries;
    for (const QString &path : deviceDirectories(QStringLiteral("/sys/class/power_supply"))) {
        if (readText(path + QStringLiteral("/type")) != QStringLiteral("Battery")) continue;
        const QString presentText = readText(path + QStringLiteral("/present"));
        if (!presentText.isEmpty() && presentText != QStringLiteral("1")) continue;

        bool capacityOk = false;
        int capacity = readText(path + QStringLiteral("/capacity")).toInt(&capacityOk);
        if (capacityOk && (capacity < 0 || capacity > 100)) capacityOk = false;

        bool fullOk = false, nowOk = false;
        double full = readText(path + QStringLiteral("/energy_full")).toDouble(&fullOk);
        double now = readText(path + QStringLiteral("/energy_now")).toDouble(&nowOk);
        int metric = fullOk && nowOk && full > 0.0 && now >= 0.0 ? 1 : 0;
        if (!metric) {
            full = readText(path + QStringLiteral("/charge_full")).toDouble(&fullOk);
            now = readText(path + QStringLiteral("/charge_now")).toDouble(&nowOk);
            if (fullOk && nowOk && full > 0.0 && now >= 0.0) metric = 2;
        }
        if (!capacityOk && metric) {
            capacity = qBound(0, qRound(now / full * 100.0), 100);
            capacityOk = true;
        }
        if (!capacityOk) continue;

        double design = readText(path + QStringLiteral("/energy_full_design")).toDouble(&fullOk);
        if (metric == 2 || !fullOk || design <= 0.0)
            design = readText(path + QStringLiteral("/charge_full_design")).toDouble(&fullOk);
        if (!fullOk || design <= 0.0) design = 0.0;

        QString status = readText(path + QStringLiteral("/status"));
        if (status.isEmpty()) status = QStringLiteral("Unknown");
        batteries.append({QFileInfo(path).fileName(), capacity, full, now, metric, design, status});
    }

    QStringList batteryNames;
    double totalFull = 0.0, totalNow = 0.0, totalDesign = 0.0;
    bool comparable = !batteries.isEmpty() && batteries.first().metric != 0;
    bool healthComparable = comparable;
    bool allDesignKnown = true;
    bool anyCharging = false, anyDischarging = false, allFull = !batteries.isEmpty();
    bool anyNotCharging = false;
    int percentSum = 0;
    for (const BatteryReading &battery : batteries) {
        batteryNames.append(battery.label + QStringLiteral(": ")
            + QString::number(battery.percent) + QLatin1Char('%')
            + QStringLiteral(" · ") + battery.status);
        percentSum += battery.percent;
        if (battery.metric != batteries.first().metric) { comparable = false; healthComparable = false; }
        if (battery.metric == batteries.first().metric && battery.metric != 0) {
            totalFull += battery.full;
            totalNow += battery.now;
        }
        if (battery.design > 0.0) totalDesign += battery.design;
        else allDesignKnown = false;
        anyCharging = anyCharging || battery.status.compare(QStringLiteral("Charging"), Qt::CaseInsensitive) == 0;
        anyDischarging = anyDischarging || battery.status.compare(QStringLiteral("Discharging"), Qt::CaseInsensitive) == 0;
        anyNotCharging = anyNotCharging || battery.status.compare(QStringLiteral("Not charging"), Qt::CaseInsensitive) == 0;
        allFull = allFull && battery.status.compare(QStringLiteral("Full"), Qt::CaseInsensitive) == 0;
    }

    const bool present = !batteries.isEmpty();
    const int capacity = !present ? 0 : (comparable && totalFull > 0.0
        ? qBound(0, qRound(totalNow / totalFull * 100.0), 100)
        : qRound(static_cast<double>(percentSum) / batteries.size()));
    const QString status = !present ? QStringLiteral("Unknown")
        : anyCharging ? QStringLiteral("Charging")
        : anyDischarging ? QStringLiteral("Discharging")
        : allFull ? QStringLiteral("Full")
        : anyNotCharging ? QStringLiteral("Not charging")
        : QStringLiteral("Unknown");
    const int health = healthComparable && allDesignKnown && totalDesign > 0.0 && totalFull > 0.0
        ? qBound(0, qRound(totalFull / totalDesign * 100.0), 100) : -1;

    const bool batteryChangedValue = m_batteryDevices != batteryNames
        || m_batteryPresent != present || m_batteryPercent != capacity
        || m_batteryStatus != status || m_batteryHealth != health;
    if (batteryChangedValue) {
        m_batteryDevices = batteryNames;
        m_batteryPresent = present;
        m_batteryPercent = capacity;
        m_batteryStatus = status;
        m_batteryHealth = health;
        emit batteryChanged();
    }

    const QStringList backlights = deviceDirectories(QStringLiteral("/sys/class/backlight"));
    const QString backlightDir = backlights.isEmpty() ? QString() : backlights.first();
    const int maxBrightness = backlightDir.isEmpty() ? -1
        : readInt(backlightDir + QStringLiteral("/max_brightness"));
    const int currentBrightness = backlightDir.isEmpty() ? -1
        : readInt(backlightDir + QStringLiteral("/brightness"));
    const int brightness = maxBrightness > 0 && currentBrightness >= 0
        ? (currentBrightness * 100 / maxBrightness) : -1;
    if (m_brightness != brightness) { m_brightness = brightness; emit brightnessChanged(); }

    QString audio = commandOutput(QStringLiteral("pactl"),
        {QStringLiteral("get-sink-volume"), QStringLiteral("@DEFAULT_SINK@")});
    static const QRegularExpression pactlVolumePattern(QStringLiteral("([0-9]{1,3})%"));
    auto match = pactlVolumePattern.match(audio);
    int volume = match.hasMatch() ? qBound(0, match.captured(1).toInt(), 150) : -1;
    QString muteState = commandOutput(QStringLiteral("pactl"),
        {QStringLiteral("get-sink-mute"), QStringLiteral("@DEFAULT_SINK@")});
    bool muted = muteState.contains(QStringLiteral("yes"), Qt::CaseInsensitive);
    if (volume < 0) {
        audio = commandOutput(QStringLiteral("wpctl"),
            {QStringLiteral("get-volume"), QStringLiteral("@DEFAULT_AUDIO_SINK@")});
        static const QRegularExpression volumePattern(QStringLiteral("Volume:\\s*([0-9.]+)"));
        const auto wpctlMatch = volumePattern.match(audio);
        volume = wpctlMatch.hasMatch()
            ? qBound(0, qRound(wpctlMatch.captured(1).toDouble() * 100.0), 150) : -1;
        muted = audio.contains(QStringLiteral("[MUTED]"));
    }
    if (m_volume != volume || m_muted != muted) {
        m_volume = volume; m_muted = muted; emit volumeChanged();
    }

    // PipeWire's PulseAudio compatibility layer exposes USB, analogue and
    // BlueZ Bluetooth sinks/sources through pactl, without device-specific code.
    const auto readAudioNodes = [](const QString &kind) {
        QVariantList nodes;
        const QString json = commandOutput(QStringLiteral("pactl"),
            {QStringLiteral("-f"), QStringLiteral("json"), QStringLiteral("list"), kind});
        const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8());
        if (!document.isArray()) return nodes;
        for (const QJsonValue &value : document.array()) {
            if (!value.isObject()) continue;
            const QJsonObject object = value.toObject();
            const QString name = object.value(QStringLiteral("name")).toString();
            if (name.isEmpty()
                || (kind == QStringLiteral("sources") && name.endsWith(QStringLiteral(".monitor"))))
                continue;
            const QString description = object.value(QStringLiteral("description")).toString(name);
            const int index = object.value(QStringLiteral("index")).toInt(-1);
            QVariantList ports;
            QString activePort;
            const QJsonValue active = object.value(QStringLiteral("active_port"));
            if (active.isObject())
                activePort = active.toObject().value(QStringLiteral("name")).toString();
            const QJsonValue portValues = object.value(QStringLiteral("ports"));
            if (portValues.isArray()) {
                for (const QJsonValue &portValue : portValues.toArray()) {
                    if (!portValue.isObject()) continue;
                    const QJsonObject port = portValue.toObject();
                    const QString portName = port.value(QStringLiteral("name")).toString();
                    if (portName.isEmpty()) continue;
                    ports.append(QVariantMap{
                        {QStringLiteral("name"), portName},
                        {QStringLiteral("description"), port.value(QStringLiteral("description")).toString(portName)},
                        {QStringLiteral("available"), port.value(QStringLiteral("availability")).toString()}
                    });
                }
            }
            nodes.append(QVariantMap{
                {QStringLiteral("name"), name},
                {QStringLiteral("description"), description},
                {QStringLiteral("index"), index},
                {QStringLiteral("state"), object.value(QStringLiteral("state")).toString()},
                {QStringLiteral("ports"), ports},
                {QStringLiteral("activePort"), activePort}
            });
        }
        return nodes;
    };
    const QVariantList outputs = readAudioNodes(QStringLiteral("sinks"));
    const QVariantList inputs = readAudioNodes(QStringLiteral("sources"));
    const QString defaultOutput = commandOutput(QStringLiteral("pactl"),
        {QStringLiteral("get-default-sink")});
    const QString defaultInput = commandOutput(QStringLiteral("pactl"),
        {QStringLiteral("get-default-source")});
    QVariantList outputPorts;
    QString activeOutputPort;
    for (const QVariant &item : outputs) {
        const QVariantMap sink = item.toMap();
        if (sink.value(QStringLiteral("name")).toString() != defaultOutput) continue;
        outputPorts = sink.value(QStringLiteral("ports")).toList();
        activeOutputPort = sink.value(QStringLiteral("activePort")).toString();
        break;
    }
    if (m_audioOutputs != outputs || m_audioInputs != inputs
        || m_audioOutput != defaultOutput || m_audioInput != defaultInput
        || m_audioOutputPorts != outputPorts || m_audioOutputPort != activeOutputPort) {
        m_audioOutputs = outputs;
        m_audioInputs = inputs;
        m_audioOutput = defaultOutput;
        m_audioInput = defaultInput;
        m_audioOutputPorts = outputPorts;
        m_audioOutputPort = activeOutputPort;
        emit audioDevicesChanged();
    }
}

void LinuxHardwareService::setBluetoothPowered(bool enabled)
{
    run(QStringLiteral("bluetoothctl"),
        {QStringLiteral("power"), enabled ? QStringLiteral("on") : QStringLiteral("off")},
        enabled ? QStringLiteral("Bluetooth powered on") : QStringLiteral("Bluetooth powered off"));
}
void LinuxHardwareService::scanBluetooth()
{
    if (!m_bluetoothAvailable) {
        setError(QStringLiteral("No Bluetooth controller is available. Check the adapter and BlueZ service."));
        emit operationFinished(false, m_lastError);
        return;
    }
    if (!m_bluetoothPowered) {
        setError(QStringLiteral("Bluetooth is powered off. Turn it on before scanning."));
        emit operationFinished(false, m_lastError);
        return;
    }
    if (m_bluetoothScanProcess->state() != QProcess::NotRunning) {
        setError(QStringLiteral("Bluetooth discovery is already running."));
        emit operationFinished(false, m_lastError);
        return;
    }

    m_bluetoothScanProcess->setProgram(QStringLiteral("bluetoothctl"));
    m_bluetoothScanProcess->setArguments(
        {QStringLiteral("--timeout"), QStringLiteral("12"),
         QStringLiteral("scan"), QStringLiteral("on")});
    m_bluetoothScanProcess->start();
    if (!m_bluetoothScanProcess->waitForStarted(3000))
        return;

    setBluetoothScanning(true);
    setError({});
    emit operationFinished(true, QStringLiteral("Bluetooth discovery started; scanning for up to 12 seconds."));
}
void LinuxHardwareService::setBluetoothPairingPrompt(const QString &prompt, const QString &passkey)
{
    if (m_bluetoothPairingPrompt == prompt && m_bluetoothPairingPasskey == passkey) return;
    m_bluetoothPairingPrompt = prompt;
    m_bluetoothPairingPasskey = passkey;
    emit bluetoothPairingChanged();
}

void LinuxHardwareService::processBluetoothPairingOutput(const QString &output)
{
    const QString line = output.trimmed();
    if (line.isEmpty()) return;
    if (line.contains(QStringLiteral("Pairing successful"), Qt::CaseInsensitive)) {
        m_bluetoothPairingSucceeded = true;
        m_bluetoothPairingTimeout->stop();
        m_bluetoothPairingProcess->write("quit\n");
        return;
    }
    if (line.contains(QStringLiteral("Failed to pair"), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("AuthenticationFailed"), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("Authentication Canceled"), Qt::CaseInsensitive)) {
        m_bluetoothPairingSucceeded = false;
        m_bluetoothPairingTimeout->stop();
        m_bluetoothPairingProcess->write("quit\n");
        setBluetoothPairingPrompt({});
        return;
    }
    static const QRegularExpression passkeyPattern(
        QStringLiteral("(?:Confirm passkey|passkey)\\s+([0-9]{6})"), QRegularExpression::CaseInsensitiveOption);
    const auto passkeyMatch = passkeyPattern.match(line);
    if (passkeyMatch.hasMatch() && (line.contains(QStringLiteral("confirm"), Qt::CaseInsensitive)
                                    || line.contains(QStringLiteral("yes/no"), Qt::CaseInsensitive))) {
        setBluetoothPairingPrompt(QStringLiteral("confirm"), passkeyMatch.captured(1));
        return;
    }
    if (line.contains(QStringLiteral("Enter PIN code"), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("Enter passkey"), Qt::CaseInsensitive)) {
        setBluetoothPairingPrompt(QStringLiteral("pin"));
        return;
    }
    if (line.contains(QStringLiteral("Request confirmation"), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("Authorize service"), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("Confirm?"), Qt::CaseInsensitive)) {
        setBluetoothPairingPrompt(QStringLiteral("authorize"));
    }
}

void LinuxHardwareService::finishBluetoothPairing(bool success, const QString &message)
{
    m_bluetoothPairingTimeout->stop();
    const bool changed = m_bluetoothPairing || !m_bluetoothPairingAddress.isEmpty()
        || !m_bluetoothPairingPrompt.isEmpty() || !m_bluetoothPairingPasskey.isEmpty();
    m_bluetoothPairing = false;
    m_bluetoothPairingAddress.clear();
    m_bluetoothPairingPrompt.clear();
    m_bluetoothPairingPasskey.clear();
    m_bluetoothPairingBuffer.clear();
    if (changed) emit bluetoothPairingChanged();
    emit operationFinished(success, message);
}

void LinuxHardwareService::pairBluetooth(const QString &address)
{
    if (!validBluetoothAddress(address)) {
        setError(QStringLiteral("Invalid Bluetooth device address."));
        emit operationFinished(false, m_lastError);
        return;
    }
    if (!m_bluetoothAvailable || !m_bluetoothPowered) {
        setError(QStringLiteral("Turn on an available Bluetooth controller before pairing."));
        emit operationFinished(false, m_lastError);
        return;
    }
    if (m_bluetoothPairingProcess->state() != QProcess::NotRunning || m_bluetoothPairing) {
        setError(QStringLiteral("Another Bluetooth pairing session is already active."));
        emit operationFinished(false, m_lastError);
        return;
    }
    m_bluetoothPairing = true;
    m_bluetoothPairingSucceeded = false;
    m_bluetoothPairingAddress = address.trimmed().toUpper();
    setBluetoothPairingPrompt({});
    m_bluetoothPairingBuffer.clear();
    emit bluetoothPairingChanged();
    setError({});
    m_bluetoothPairingProcess->start(QStringLiteral("bluetoothctl"));
    if (!m_bluetoothPairingProcess->waitForStarted(3000)) {
        finishBluetoothPairing(false, QStringLiteral("Could not start Bluetooth pairing."));
        setError(QStringLiteral("Could not start bluetoothctl. Check the BlueZ installation."));
        return;
    }
    m_bluetoothPairingTimeout->start(60000);
    m_bluetoothPairingProcess->write("agent KeyboardDisplay\n");
    m_bluetoothPairingProcess->write("default-agent\n");
    m_bluetoothPairingProcess->write(QStringLiteral("pair %1\n").arg(m_bluetoothPairingAddress).toUtf8());
}

void LinuxHardwareService::respondBluetoothPairing(const QString &pin)
{
    if (!m_bluetoothPairing || m_bluetoothPairingPrompt != QStringLiteral("pin")
        || pin.isEmpty() || pin.size() > 16 || pin.contains(QRegularExpression(QStringLiteral("[^0-9A-Za-z]")))) {
        setError(QStringLiteral("Enter a valid Bluetooth PIN or passkey when requested."));
        return;
    }
    m_bluetoothPairingProcess->write(pin.toUtf8() + "\n");
    setBluetoothPairingPrompt({});
}

void LinuxHardwareService::confirmBluetoothPairing(bool accepted)
{
    if (!m_bluetoothPairing || (m_bluetoothPairingPrompt != QStringLiteral("confirm")
                                && m_bluetoothPairingPrompt != QStringLiteral("authorize"))) return;
    m_bluetoothPairingProcess->write(accepted ? "yes\n" : "no\n");
    setBluetoothPairingPrompt({});
    if (!accepted) {
        m_bluetoothPairingProcess->write("quit\n");
        m_bluetoothPairingTimeout->stop();
    }
}

void LinuxHardwareService::cancelBluetoothPairing()
{
    if (!m_bluetoothPairing) return;
    m_bluetoothPairingProcess->write("no\n");
    m_bluetoothPairingProcess->write("quit\n");
    m_bluetoothPairingTimeout->stop();
    m_bluetoothPairingProcess->terminate();
    QTimer::singleShot(1500, m_bluetoothPairingProcess, &QProcess::kill);
    finishBluetoothPairing(false, QStringLiteral("Bluetooth pairing cancelled."));
}
void LinuxHardwareService::connectBluetooth(const QString &address)
{
    if (!validBluetoothAddress(address)) {
        setError(QStringLiteral("Invalid Bluetooth device address."));
        emit operationFinished(false, m_lastError);
        return;
    }
    run(QStringLiteral("bluetoothctl"), {QStringLiteral("connect"), address},
        QStringLiteral("Bluetooth device connected"), 20000);
}
void LinuxHardwareService::disconnectBluetooth(const QString &address)
{
    if (!validBluetoothAddress(address)) {
        setError(QStringLiteral("Invalid Bluetooth device address."));
        emit operationFinished(false, m_lastError);
        return;
    }
    run(QStringLiteral("bluetoothctl"), {QStringLiteral("disconnect"), address},
        QStringLiteral("Bluetooth device disconnected"), 12000);
}
void LinuxHardwareService::trustBluetooth(const QString &address, bool trusted)
{
    if (!validBluetoothAddress(address)) {
        setError(QStringLiteral("Invalid Bluetooth device address."));
        emit operationFinished(false, m_lastError);
        return;
    }
    run(QStringLiteral("bluetoothctl"),
        {trusted ? QStringLiteral("trust") : QStringLiteral("untrust"), address},
        trusted ? QStringLiteral("Bluetooth device trusted") : QStringLiteral("Bluetooth device trust revoked"));
}
void LinuxHardwareService::removeBluetooth(const QString &address)
{
    if (!validBluetoothAddress(address)) {
        setError(QStringLiteral("Invalid Bluetooth device address."));
        emit operationFinished(false, m_lastError);
        return;
    }
    run(QStringLiteral("bluetoothctl"), {QStringLiteral("remove"), address},
        QStringLiteral("Bluetooth device removed"));
}
void LinuxHardwareService::setBrightness(int percent)
{
    const QStringList backlights = deviceDirectories(QStringLiteral("/sys/class/backlight"));
    const QString dir = backlights.isEmpty() ? QString() : backlights.first();
    const int maximum = dir.isEmpty() ? -1 : readInt(dir + QStringLiteral("/max_brightness"));
    if (maximum <= 0) {
        setError(QStringLiteral("No backlight device is exposed by this hardware."));
        emit operationFinished(false, m_lastError);
        return;
    }
    run(QStringLiteral("brightnessctl"),
        {QStringLiteral("set"), QString::number(qBound(0, percent, 100)) + QLatin1Char('%')},
        QStringLiteral("Brightness updated"));
}
void LinuxHardwareService::setVolume(int percent)
{
    run(QStringLiteral("pactl"),
        {QStringLiteral("set-sink-volume"), QStringLiteral("@DEFAULT_SINK@"),
         QString::number(qBound(0, percent, 150)) + QLatin1Char('%')},
        QStringLiteral("Audio volume updated"));
}
void LinuxHardwareService::setAudioOutput(const QString &name)
{
    if (name.trimmed().isEmpty()) {
        setError(QStringLiteral("Select a valid audio output device."));
        emit operationFinished(false, m_lastError);
        return;
    }
    bool found = false;
    for (const QVariant &item : m_audioOutputs) {
        if (item.toMap().value(QStringLiteral("name")).toString() == name) {
            found = true; break;
        }
    }
    if (!found) {
        refresh();
        for (const QVariant &item : m_audioOutputs)
            if (item.toMap().value(QStringLiteral("name")).toString() == name) found = true;
    }
    if (!found) {
        setError(QStringLiteral("That audio output is no longer available."));
        emit operationFinished(false, m_lastError);
        return;
    }
    run(QStringLiteral("pactl"), {QStringLiteral("set-default-sink"), name},
        QStringLiteral("Audio output changed"));
}

void LinuxHardwareService::setAudioInput(const QString &name)
{
    if (name.trimmed().isEmpty()) {
        setError(QStringLiteral("Select a valid audio input device."));
        emit operationFinished(false, m_lastError);
        return;
    }
    bool found = false;
    for (const QVariant &item : m_audioInputs)
        if (item.toMap().value(QStringLiteral("name")).toString() == name) found = true;
    if (!found) {
        refresh();
        for (const QVariant &item : m_audioInputs)
            if (item.toMap().value(QStringLiteral("name")).toString() == name) found = true;
    }
    if (!found) {
        setError(QStringLiteral("That audio input is no longer available."));
        emit operationFinished(false, m_lastError);
        return;
    }
    run(QStringLiteral("pactl"), {QStringLiteral("set-default-source"), name},
        QStringLiteral("Audio input changed"));
}

void LinuxHardwareService::setAudioOutputPort(const QString &name)
{
    if (name.trimmed().isEmpty()) {
        setError(QStringLiteral("Select a valid audio output port."));
        emit operationFinished(false, m_lastError);
        return;
    }
    bool found = false;
    for (const QVariant &item : m_audioOutputPorts)
        if (item.toMap().value(QStringLiteral("name")).toString() == name) found = true;
    if (!found) {
        refresh();
        for (const QVariant &item : m_audioOutputPorts)
            if (item.toMap().value(QStringLiteral("name")).toString() == name) found = true;
    }
    if (!found) {
        setError(QStringLiteral("That audio output port is no longer available."));
        emit operationFinished(false, m_lastError);
        return;
    }
    run(QStringLiteral("pactl"),
        {QStringLiteral("set-sink-port"), QStringLiteral("@DEFAULT_SINK@"), name},
        QStringLiteral("Audio output port changed"));
}

void LinuxHardwareService::setMuted(bool muted)
{
    run(QStringLiteral("pactl"),
        {QStringLiteral("set-sink-mute"), QStringLiteral("@DEFAULT_SINK@"),
         muted ? QStringLiteral("1") : QStringLiteral("0")},
        muted ? QStringLiteral("Audio muted") : QStringLiteral("Audio unmuted"));
}
void LinuxHardwareService::suspend()
{
    if (!QProcess::startDetached(QStringLiteral("systemctl"), {QStringLiteral("suspend")}))
        setError(QStringLiteral("Could not request suspend. Check systemd and polkit permissions."));
}
void LinuxHardwareService::restart()
{
    if (!QProcess::startDetached(QStringLiteral("systemctl"), {QStringLiteral("reboot")}))
        setError(QStringLiteral("Could not request restart. Check systemd and polkit permissions."));
}
void LinuxHardwareService::shutdown()
{
    if (!QProcess::startDetached(QStringLiteral("systemctl"), {QStringLiteral("poweroff")}))
        setError(QStringLiteral("Could not request shutdown. Check systemd and polkit permissions."));
}
