#include "LinuxHardwareService.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QTextStream>
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

QString firstDevice(const QString &root)
{
    const QStringList entries = QDir(root).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &entry : entries)
        if (entry != QStringLiteral("AC") && entry != QStringLiteral("USB"))
            return root + QLatin1Char('/') + entry;
    return {};
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
    refresh();
}

void LinuxHardwareService::setError(const QString &message)
{
    if (m_lastError == message) return;
    m_lastError = message;
    emit lastErrorChanged();
}

bool LinuxHardwareService::run(const QString &program, const QStringList &arguments,
                               const QString &successMessage)
{
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForStarted(3000) || !process.waitForFinished(8000)) {
        process.kill();
        setError(program + QStringLiteral(" is unavailable or did not respond."));
        emit operationFinished(false, m_lastError);
        return false;
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        const QString detail = QString::fromUtf8(process.readAllStandardError()).trimmed();
        setError(detail.isEmpty() ? program + QStringLiteral(" failed.") : detail);
        emit operationFinished(false, m_lastError);
        return false;
    }
    setError({});
    emit operationFinished(true, successMessage);
    refresh();
    return true;
}

void LinuxHardwareService::refresh()
{
    const QString bt = commandOutput(QStringLiteral("bluetoothctl"),
                                     {QStringLiteral("show")});
    const bool btAvailable = !bt.isEmpty();
    const bool btPowered = bt.contains(QStringLiteral("Powered: yes"));
    if (m_bluetoothAvailable != btAvailable || m_bluetoothPowered != btPowered) {
        m_bluetoothAvailable = btAvailable;
        m_bluetoothPowered = btPowered;
        emit bluetoothChanged();
    }

    QStringList devices;
    const QString listing = commandOutput(QStringLiteral("bluetoothctl"),
                                           {QStringLiteral("devices")});
    for (const QString &line : listing.split(QLatin1Char('\n'), Qt::SkipEmptyParts))
        devices.append(line.trimmed());
    if (m_bluetoothDevices != devices) {
        m_bluetoothDevices = devices;
        emit bluetoothDevicesChanged();
    }

    const QString batteryDir = firstDevice(QStringLiteral("/sys/class/power_supply"));
    const bool present = !batteryDir.isEmpty()
        && readText(batteryDir + QStringLiteral("/type")) == QStringLiteral("Battery");
    const int capacity = present ? readInt(batteryDir + QStringLiteral("/capacity"), 0) : 0;
    if (m_batteryPresent != present || m_batteryPercent != capacity) {
        m_batteryPresent = present;
        m_batteryPercent = capacity;
        emit batteryChanged();
    }

    const QString backlightDir = firstDevice(QStringLiteral("/sys/class/backlight"));
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
    run(QStringLiteral("bluetoothctl"), {QStringLiteral("scan"), QStringLiteral("on")},
        QStringLiteral("Bluetooth discovery started"));
}
void LinuxHardwareService::pairBluetooth(const QString &address)
{
    run(QStringLiteral("bluetoothctl"), {QStringLiteral("pair"), address},
        QStringLiteral("Pairing request sent"));
}
void LinuxHardwareService::connectBluetooth(const QString &address)
{
    run(QStringLiteral("bluetoothctl"), {QStringLiteral("connect"), address},
        QStringLiteral("Bluetooth device connected"));
}
void LinuxHardwareService::disconnectBluetooth(const QString &address)
{
    run(QStringLiteral("bluetoothctl"), {QStringLiteral("disconnect"), address},
        QStringLiteral("Bluetooth device disconnected"));
}
void LinuxHardwareService::removeBluetooth(const QString &address)
{
    run(QStringLiteral("bluetoothctl"), {QStringLiteral("remove"), address},
        QStringLiteral("Bluetooth device removed"));
}
void LinuxHardwareService::setBrightness(int percent)
{
    const QString dir = firstDevice(QStringLiteral("/sys/class/backlight"));
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
    run(QStringLiteral("systemctl"), {QStringLiteral("suspend")},
        QStringLiteral("Suspend requested"));
}
void LinuxHardwareService::restart()
{
    run(QStringLiteral("systemctl"), {QStringLiteral("reboot")},
        QStringLiteral("Restart requested"));
}
void LinuxHardwareService::shutdown()
{
    run(QStringLiteral("systemctl"), {QStringLiteral("poweroff")},
        QStringLiteral("Shutdown requested"));
}
