#include "LinuxHardwareService.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QTextStream>
#include <QTimer>
#include <QVariantMap>
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
            QStringLiteral("(^|\\n)(Failed|Error|No default controller|not available|not ready)\\b"),
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
        QStringLiteral("(^|\\n)(Failed|Error|No default controller|not available|not ready)\\b"),
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

    struct BatteryReading { QString label; int percent; double full; double now; int metric; };
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
        if (capacityOk) batteries.append({QFileInfo(path).fileName(), capacity, full, now, metric});
    }
    QStringList batteryNames;
    int percentSum = 0;
    double totalFull = 0.0, totalNow = 0.0;
    const int commonMetric = batteries.isEmpty() ? 0 : batteries.first().metric;
    bool comparable = commonMetric != 0;
    for (const BatteryReading &battery : batteries) {
        batteryNames.append(battery.label + QStringLiteral(": ") + QString::number(battery.percent) + QLatin1Char('%'));
        percentSum += battery.percent;
        if (battery.metric != commonMetric) comparable = false;
        if (battery.metric == commonMetric && commonMetric != 0) {
            totalFull += battery.full;
            totalNow += battery.now;
        }
    }
    const bool present = !batteries.isEmpty();
    const int capacity = !present ? 0 : (comparable && totalFull > 0.0
        ? qBound(0, qRound(totalNow / totalFull * 100.0), 100)
        : qRound(static_cast<double>(percentSum) / batteries.size()));
    if (m_batteryDevices != batteryNames) { m_batteryDevices = batteryNames; emit batteryChanged(); }
    if (m_batteryPresent != present || m_batteryPercent != capacity) {
        m_batteryPresent = present; m_batteryPercent = capacity; emit batteryChanged();
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

    const QString audio = commandOutput(QStringLiteral("wpctl"),
                                        {QStringLiteral("get-volume"),
                                         QStringLiteral("@DEFAULT_AUDIO_SINK@")});
    static const QRegularExpression volumePattern(QStringLiteral("Volume:\\s*([0-9.]+)"));
    const auto match = volumePattern.match(audio);
    const int volume = match.hasMatch()
        ? qBound(0, qRound(match.captured(1).toDouble() * 100.0), 150) : -1;
    const bool muted = audio.contains(QStringLiteral("[MUTED]"));
    if (m_volume != volume || m_muted != muted) {
        m_volume = volume; m_muted = muted; emit volumeChanged();
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
void LinuxHardwareService::pairBluetooth(const QString &address)
{
    if (!validBluetoothAddress(address)) {
        setError(QStringLiteral("Invalid Bluetooth device address."));
        emit operationFinished(false, m_lastError);
        return;
    }
    run(QStringLiteral("bluetoothctl"), {QStringLiteral("pair"), address},
        QStringLiteral("Bluetooth device paired"), 45000);
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
    run(QStringLiteral("wpctl"),
        {QStringLiteral("set-volume"), QStringLiteral("@DEFAULT_AUDIO_SINK@"),
         QString::number(qBound(0, percent, 150)) + QLatin1Char('%')},
        QStringLiteral("Audio volume updated"));
}
void LinuxHardwareService::setMuted(bool muted)
{
    run(QStringLiteral("wpctl"),
        {QStringLiteral("set-mute"), QStringLiteral("@DEFAULT_AUDIO_SINK@"),
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
