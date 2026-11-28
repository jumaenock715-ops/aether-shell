#include "NetworkService.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QDBusVariant>
#include <QTimer>
#include <QVariantMap>

namespace {
const QString service = QStringLiteral("org.freedesktop.NetworkManager");
const QString rootPath = QStringLiteral("/org/freedesktop/NetworkManager");
const QString rootInterface = QStringLiteral("org.freedesktop.NetworkManager");
const QString propertiesInterface = QStringLiteral("org.freedesktop.DBus.Properties");
const QString deviceInterface = QStringLiteral("org.freedesktop.NetworkManager.Device");
const QString wifiInterface = QStringLiteral("org.freedesktop.NetworkManager.Device.Wireless");
const QString accessPointInterface = QStringLiteral("org.freedesktop.NetworkManager.AccessPoint");

QVariantMap readProperties(const QString &path, const QString &interfaceName)
{
    QDBusInterface properties(service, path, propertiesInterface,
                              QDBusConnection::systemBus());
    const QDBusReply<QVariantMap> reply =
        properties.call(QStringLiteral("GetAll"), interfaceName);
    return reply.isValid() ? reply.value() : QVariantMap{};
}

QString decodeSsid(const QByteArray &bytes)
{
    return QString::fromUtf8(bytes);
}
}

NetworkService::NetworkService(QObject *parent)
    : QObject(parent),
      m_watcher(new QDBusServiceWatcher(service, QDBusConnection::systemBus(),
                                         QDBusServiceWatcher::WatchForOwnerChange, this))
{
    connect(m_watcher, &QDBusServiceWatcher::serviceOwnerChanged,
            this, [this](const QString &, const QString &, const QString &) {
        refresh();
    });
    refresh();
}

void NetworkService::setError(const QString &message)
{
    if (m_lastError == message) return;
    m_lastError = message;
    emit lastErrorChanged();
}

void NetworkService::setScanning(bool value)
{
    if (m_scanning == value) return;
    m_scanning = value;
    emit scanningChanged();
}

void NetworkService::setStatus(const QString &value)
{
    if (m_status == value) return;
    m_status = value;
    emit statusChanged();
}

void NetworkService::refresh()
{
    const bool wasAvailable = m_available;
    QDBusInterface manager(service, rootPath, rootInterface,
                           QDBusConnection::systemBus());
    m_available = manager.isValid();
    if (wasAvailable != m_available) emit availableChanged();

    if (!m_available) {
        m_wifiEnabled = false;
        m_activeSsid.clear();
        m_accessPoints.clear();
        setStatus(QStringLiteral("NetworkManager unavailable"));
        setScanning(false);
        emit wifiEnabledChanged();
        emit activeSsidChanged();
        emit accessPointsChanged();
        setError(QStringLiteral("NetworkManager is not running or its system D-Bus API is unavailable."));
        return;
    }

    setError(QString());
    updateWifiEnabled();
    updateActiveConnection();
    updateAccessPoints();
}

void NetworkService::updateWifiEnabled()
{
    QDBusInterface manager(service, rootPath, rootInterface,
                           QDBusConnection::systemBus());
    const QDBusReply<bool> reply = manager.call(QStringLiteral("WirelessEnabled")).type()
        == QDBusMessage::ErrorMessage
        ? QDBusReply<bool>() : manager.call(QStringLiteral("Get"), rootInterface,
                                             QStringLiteral("WirelessEnabled"));
    bool enabled = false;
    if (reply.isValid()) enabled = reply.value();
    else {
        const QVariantMap props = readProperties(rootPath, rootInterface);
        enabled = props.value(QStringLiteral("WirelessEnabled")).toBool();
    }
    if (m_wifiEnabled != enabled) {
        m_wifiEnabled = enabled;
        emit wifiEnabledChanged();
    }
}

void NetworkService::updateActiveConnection()
{
    QString ssid;
    QDBusInterface manager(service, rootPath, rootInterface,
                           QDBusConnection::systemBus());
    const QVariantMap props = readProperties(rootPath, rootInterface);
    const QStringList activePaths =
        props.value(QStringLiteral("ActiveConnections")).toStringList();

    for (const QString &activePath : activePaths) {
        const QVariantMap active = readProperties(activePath,
            QStringLiteral("org.freedesktop.NetworkManager.Connection.Active"));
        if (active.value(QStringLiteral("Type")).toString() != QStringLiteral("802-11-wireless"))
            continue;
        const QString devicePath = active.value(QStringLiteral("Devices")).toStringList().value(0);
        if (devicePath.isEmpty()) continue;
        const QVariantMap device = readProperties(devicePath, deviceInterface);
        const QString apPath = device.value(QStringLiteral("ActiveAccessPoint")).toString();
        if (apPath.isEmpty() || apPath == QStringLiteral("/")) continue;
        const QVariantMap ap = readProperties(apPath, accessPointInterface);
        ssid = decodeSsid(ap.value(QStringLiteral("Ssid")).toByteArray());
        break;
    }

    if (m_activeSsid != ssid) {
        m_activeSsid = ssid;
        emit activeSsidChanged();
    }
    setStatus(ssid.isEmpty() ? QStringLiteral("Not connected")
                             : QStringLiteral("Connected"));
}

void NetworkService::updateAccessPoints()
{
    QVariantList found;
    QDBusInterface manager(service, rootPath, rootInterface,
                           QDBusConnection::systemBus());
    const QVariantMap managerProps = readProperties(rootPath, rootInterface);
    const QStringList devicePaths = managerProps.value(QStringLiteral("Devices")).toStringList();

    for (const QString &devicePath : devicePaths) {
        const QVariantMap device = readProperties(devicePath, deviceInterface);
        if (device.value(QStringLiteral("DeviceType")).toUInt() != 2) continue;

        QDBusInterface wireless(service, devicePath, wifiInterface,
                                QDBusConnection::systemBus());
        const QDBusReply<QList<QDBusObjectPath>> aps =
            wireless.call(QStringLiteral("GetAllAccessPoints"));
        if (!aps.isValid()) continue;

        for (const QDBusObjectPath &apPath : aps.value()) {
            const QVariantMap ap = readProperties(apPath.path(), accessPointInterface);
            const QString ssid = decodeSsid(ap.value(QStringLiteral("Ssid")).toByteArray());
            if (ssid.isEmpty()) continue;
            QVariantMap item;
            item.insert(QStringLiteral("ssid"), ssid);
            item.insert(QStringLiteral("strength"), ap.value(QStringLiteral("Strength")).toUInt());
            item.insert(QStringLiteral("path"), apPath.path());
            item.insert(QStringLiteral("security"),
                        ap.value(QStringLiteral("WpaFlags")).toUInt() != 0 ||
                        ap.value(QStringLiteral("RsnFlags")).toUInt() != 0);
            found.append(item);
        }
    }

    if (m_accessPoints != found) {
        m_accessPoints = found;
        emit accessPointsChanged();
    }
}

void NetworkService::setWifiEnabled(bool enabled)
{
    if (!m_available) {
        setError(QStringLiteral("NetworkManager is unavailable."));
        emit operationFinished(false, m_lastError);
        return;
    }

    QDBusInterface manager(service, rootPath, rootInterface,
                           QDBusConnection::systemBus());
    const QDBusMessage reply =
        manager.call(QStringLiteral("SetWirelessEnabled"), enabled);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        setError(reply.errorMessage());
        emit operationFinished(false, m_lastError);
        return;
    }

    setError(QString());
    QTimer::singleShot(350, this, [this]() { refresh(); });
    emit operationFinished(true, enabled ? QStringLiteral("Wi-Fi enabled")
                                         : QStringLiteral("Wi-Fi disabled"));
}

void NetworkService::requestScan()
{
    if (!m_available) {
        setError(QStringLiteral("NetworkManager is unavailable."));
        emit operationFinished(false, m_lastError);
        return;
    }

    QDBusInterface manager(service, rootPath, rootInterface,
                           QDBusConnection::systemBus());
    const QStringList devices = readProperties(rootPath, rootInterface)
        .value(QStringLiteral("Devices")).toStringList();
    bool requested = false;

    for (const QString &path : devices) {
        const QVariantMap device = readProperties(path, deviceInterface);
        if (device.value(QStringLiteral("DeviceType")).toUInt() != 2) continue;
        QDBusInterface wireless(service, path, wifiInterface,
                                QDBusConnection::systemBus());
        const QDBusMessage reply = wireless.call(QStringLiteral("RequestScan"),
                                                  QVariantMap{});
        if (reply.type() == QDBusMessage::ErrorMessage) {
            setError(reply.errorMessage());
            emit operationFinished(false, m_lastError);
            return;
        }
        requested = true;
    }

    if (!requested) {
        setError(QStringLiteral("No Wi-Fi device was found."));
        emit operationFinished(false, m_lastError);
        return;
    }

    setScanning(true);
    QTimer::singleShot(2500, this, [this]() {
        setScanning(false);
        refresh();
    });
    emit operationFinished(true, QStringLiteral("Wi-Fi scan requested"));
}

void NetworkService::connectToNetwork(const QString &ssid, const QString &password)
{
    Q_UNUSED(password);
    if (ssid.trimmed().isEmpty()) {
        setError(QStringLiteral("Choose a network first."));
        emit operationFinished(false, m_lastError);
        return;
    }

    const auto it = std::find_if(m_accessPoints.cbegin(), m_accessPoints.cend(),
        [&ssid](const QVariant &entry) {
            return entry.toMap().value(QStringLiteral("ssid")).toString() == ssid;
        });
    if (it == m_accessPoints.cend()) {
        setError(QStringLiteral("That network is not in the current scan results."));
        emit operationFinished(false, m_lastError);
        return;
    }

    const QString path = it->toMap().value(QStringLiteral("path")).toString();
    activateAccessPoint(path, password);
}

void NetworkService::activateAccessPoint(const QString &path, const QString &password)
{
    Q_UNUSED(password);
    setError(QStringLiteral("Network activation needs a saved NetworkManager connection profile. This first bridge milestone supports discovery and radio control; secure password/profile handling is the next increment."));
    emit operationFinished(false, m_lastError);
}

void NetworkService::disconnectFromNetwork()
{
    if (m_activeSsid.isEmpty()) {
        emit operationFinished(true, QStringLiteral("Already disconnected"));
        return;
    }

    const QVariantMap managerProps = readProperties(rootPath, rootInterface);
    const QStringList activePaths =
        managerProps.value(QStringLiteral("ActiveConnections")).toStringList();

    for (const QString &activePath : activePaths) {
        const QVariantMap active = readProperties(activePath,
            QStringLiteral("org.freedesktop.NetworkManager.Connection.Active"));
        if (active.value(QStringLiteral("Type")).toString() != QStringLiteral("802-11-wireless"))
            continue;
        QDBusInterface activeConnection(service, activePath,
            QStringLiteral("org.freedesktop.NetworkManager.Connection.Active"),
            QDBusConnection::systemBus());
        const QDBusMessage reply = activeConnection.call(QStringLiteral("Deactivate"));
        if (reply.type() == QDBusMessage::ErrorMessage) {
            setError(reply.errorMessage());
            emit operationFinished(false, m_lastError);
        } else {
            QTimer::singleShot(350, this, [this]() { refresh(); });
            emit operationFinished(true, QStringLiteral("Disconnected"));
        }
        return;
    }
    emit operationFinished(true, QStringLiteral("Already disconnected"));
}
