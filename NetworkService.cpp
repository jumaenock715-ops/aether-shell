#include "NetworkService.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QDBusVariant>
#include <QDBusServiceWatcher>
#include <QTimer>
#include <algorithm>

namespace {
const QString kService = QStringLiteral("org.freedesktop.NetworkManager");
const QString kRoot = QStringLiteral("/org/freedesktop/NetworkManager");
const QString kManager = QStringLiteral("org.freedesktop.NetworkManager");
const QString kProperties = QStringLiteral("org.freedesktop.DBus.Properties");
const QString kDevice = QStringLiteral("org.freedesktop.NetworkManager.Device");
const QString kWireless = QStringLiteral("org.freedesktop.NetworkManager.Device.Wireless");
const QString kAccessPoint = QStringLiteral("org.freedesktop.NetworkManager.AccessPoint");

QVariant readDbusProperty(const QString &path, const QString &interfaceName, const QString &name)
{
    QDBusInterface iface(kService, path, kProperties, QDBusConnection::systemBus());
    const QDBusReply<QDBusVariant> reply =
        iface.call(QStringLiteral("Get"), interfaceName, name);
    return reply.isValid() ? reply.value().variant() : QVariant{};
}

QVariantMap allProperties(const QString &path, const QString &interfaceName)
{
    QDBusInterface iface(kService, path, kProperties, QDBusConnection::systemBus());
    const QDBusReply<QVariantMap> reply =
        iface.call(QStringLiteral("GetAll"), interfaceName);
    return reply.isValid() ? reply.value() : QVariantMap{};
}

QList<QDBusObjectPath> objectPaths(const QString &path, const QString &interfaceName,
                                   const QString &method)
{
    QDBusInterface iface(kService, path, interfaceName, QDBusConnection::systemBus());
    const QDBusReply<QList<QDBusObjectPath>> reply = iface.call(method);
    return reply.isValid() ? reply.value() : QList<QDBusObjectPath>{};
}

QString ssidFrom(const QVariant &value)
{
    return QString::fromUtf8(value.toByteArray());
}
}

NetworkService::NetworkService(QObject *parent)
    : QObject(parent),
      m_watcher(new QDBusServiceWatcher(kService, QDBusConnection::systemBus(),
                                         QDBusServiceWatcher::WatchForOwnerChange, this))
{
    connect(m_watcher, &QDBusServiceWatcher::serviceOwnerChanged,
            this, [this]() { refresh(); });
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
    const bool previousAvailability = m_available;
    QDBusInterface manager(kService, kRoot, kManager, QDBusConnection::systemBus());
    m_available = manager.isValid();
    if (previousAvailability != m_available) emit availableChanged();

    if (!m_available) {
        if (m_wifiEnabled) { m_wifiEnabled = false; emit wifiEnabledChanged(); }
        if (!m_activeSsid.isEmpty()) { m_activeSsid.clear(); emit activeSsidChanged(); }
        if (!m_accessPoints.isEmpty()) { m_accessPoints.clear(); emit accessPointsChanged(); }
        setStatus(QStringLiteral("NetworkManager unavailable"));
        setScanning(false);
        setError(QStringLiteral("NetworkManager is not running."));
        return;
    }

    setError(QString());
    const bool enabled = readDbusProperty(kRoot, kManager, QStringLiteral("WirelessEnabled")).toBool();
    if (m_wifiEnabled != enabled) { m_wifiEnabled = enabled; emit wifiEnabledChanged(); }

    updateAccessPoints();
    updateActiveConnection();
}

void NetworkService::updateActiveConnection()
{
    // The active SSID is derived from the wireless device's ActiveAccessPoint.
    QString activeSsid;
    const auto devices = objectPaths(kRoot, kManager, QStringLiteral("GetDevices"));
    for (const auto &devicePath : devices) {
        const QVariantMap device = allProperties(devicePath.path(), kDevice);
        if (device.value(QStringLiteral("DeviceType")).toUInt() != 2) continue;
        const QString apPath = readDbusProperty(devicePath.path(), kWireless,
                                        QStringLiteral("ActiveAccessPoint")).value<QDBusObjectPath>().path();
        if (apPath.isEmpty() || apPath == QStringLiteral("/")) continue;
        activeSsid = ssidFrom(allProperties(apPath, kAccessPoint).value(QStringLiteral("Ssid")));
        break;
    }

    if (m_activeSsid != activeSsid) { m_activeSsid = activeSsid; emit activeSsidChanged(); }
    setStatus(activeSsid.isEmpty() ? QStringLiteral("Not connected")
                                   : QStringLiteral("Connected"));
}

void NetworkService::updateAccessPoints()
{
    QVariantList found;
    const auto devices = objectPaths(kRoot, kManager, QStringLiteral("GetDevices"));
    for (const auto &devicePath : devices) {
        const QVariantMap device = allProperties(devicePath.path(), kDevice);
        if (device.value(QStringLiteral("DeviceType")).toUInt() != 2) continue;

        const auto accessPoints = objectPaths(devicePath.path(), kWireless,
                                               QStringLiteral("GetAllAccessPoints"));
        for (const auto &apPath : accessPoints) {
            const QVariantMap ap = allProperties(apPath.path(), kAccessPoint);
            const QString ssid = ssidFrom(ap.value(QStringLiteral("Ssid")));
            if (ssid.isEmpty()) continue;

            QVariantMap entry;
            entry.insert(QStringLiteral("ssid"), ssid);
            entry.insert(QStringLiteral("strength"), ap.value(QStringLiteral("Strength")).toUInt());
            entry.insert(QStringLiteral("path"), apPath.path());
            entry.insert(QStringLiteral("security"),
                         ap.value(QStringLiteral("WpaFlags")).toUInt() != 0 ||
                         ap.value(QStringLiteral("RsnFlags")).toUInt() != 0);
            found.append(entry);
        }
    }

    if (m_accessPoints != found) { m_accessPoints = found; emit accessPointsChanged(); }
}

void NetworkService::setWifiEnabled(bool enabled)
{
    if (!m_available) {
        setError(QStringLiteral("NetworkManager is unavailable."));
        emit operationFinished(false, m_lastError);
        return;
    }

    QDBusInterface properties(kService, kRoot, kProperties, QDBusConnection::systemBus());
    const QDBusMessage reply = properties.call(
        QStringLiteral("Set"), kManager, QStringLiteral("WirelessEnabled"),
        QVariant::fromValue(QDBusVariant(enabled)));
    if (reply.type() == QDBusMessage::ErrorMessage) {
        setError(reply.errorMessage());
        emit operationFinished(false, m_lastError);
        return;
    }

    setError(QString());
    QTimer::singleShot(300, this, [this]() { refresh(); });
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

    const auto devices = objectPaths(kRoot, kManager, QStringLiteral("GetDevices"));
    bool foundWireless = false;
    for (const auto &devicePath : devices) {
        const QVariantMap device = allProperties(devicePath.path(), kDevice);
        if (device.value(QStringLiteral("DeviceType")).toUInt() != 2) continue;
        foundWireless = true;
        QDBusInterface wireless(kService, devicePath.path(), kWireless,
                                QDBusConnection::systemBus());
        const QDBusMessage reply = wireless.call(QStringLiteral("RequestScan"), QVariantMap{});
        if (reply.type() == QDBusMessage::ErrorMessage) {
            setError(reply.errorMessage());
            emit operationFinished(false, m_lastError);
            return;
        }
    }

    if (!foundWireless) {
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
    Q_UNUSED(ssid);
    setError(QStringLiteral("Network connection setup is not implemented in this increment. Network discovery and Wi-Fi radio control are available first; secure saved-profile handling follows next."));
    emit operationFinished(false, m_lastError);
}

void NetworkService::disconnectFromNetwork()
{
    const auto activeConnections = readDbusProperty(
        kRoot, kManager, QStringLiteral("ActiveConnections"))
        .value<QList<QDBusObjectPath>>();
    for (const auto &activePath : activeConnections) {
        const QVariantMap active = allProperties(activePath.path(),
            QStringLiteral("org.freedesktop.NetworkManager.Connection.Active"));
        if (active.value(QStringLiteral("Type")).toString() != QStringLiteral("802-11-wireless"))
            continue;

        QDBusInterface manager(kService, kRoot, kManager, QDBusConnection::systemBus());
        const QDBusMessage reply = manager.call(QStringLiteral("DeactivateConnection"), activePath);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            setError(reply.errorMessage());
            emit operationFinished(false, m_lastError);
        } else {
            QTimer::singleShot(300, this, [this]() { refresh(); });
            emit operationFinished(true, QStringLiteral("Disconnected"));
        }
        return;
    }
    emit operationFinished(true, QStringLiteral("Already disconnected"));
}
