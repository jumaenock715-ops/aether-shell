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
#include <QDBusError>
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

QString deviceReasonText(uint reason)
{
    switch (reason) {
    case 4: return QStringLiteral("NetworkManager could not configure the Wi-Fi connection.");
    case 5: return QStringLiteral("An IP address could not be obtained.");
    case 6: return QStringLiteral("The IP configuration expired.");
    case 7: return QStringLiteral("Wi-Fi credentials were missing or rejected. Check the username and password.");
    case 8: return QStringLiteral("The Wi-Fi access point disconnected the authentication session.");
    case 9: return QStringLiteral("The Wi-Fi authentication settings were rejected.");
    case 10: return QStringLiteral("The Wi-Fi authentication service failed.");
    case 11: return QStringLiteral("Wi-Fi authentication timed out.");
    default: return QStringLiteral("NetworkManager reported a Wi-Fi device failure (reason %1).").arg(reason);
    }
}

uint stateReasonCode(const QVariant &value)
{
    if (!value.isValid()) return 0;
    const QDBusArgument arg = value.value<QDBusArgument>();
    if (arg.currentType() != QDBusArgument::StructureType) return 0;
    uint state = 0, reason = 0;
    arg.beginStructure();
    arg >> state >> reason;
    arg.endStructure();
    Q_UNUSED(state);
    return reason;
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

void NetworkService::setConnecting(bool value)
{
    if (m_connecting == value) return;
    m_connecting = value;
    emit connectingChanged();
}

void NetworkService::setConnectionState(const QString &value)
{
    if (m_connectionState == value) return;
    m_connectionState = value;
    emit connectionStateChanged();
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
        setConnectionState(QStringLiteral("unavailable"));
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
    if (!m_connecting)
        setConnectionState(activeSsid.isEmpty() ? QStringLiteral("disconnected")
                                                : QStringLiteral("connected"));
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
            const uint apFlags = ap.value(QStringLiteral("Flags")).toUInt();
            const uint wpa = ap.value(QStringLiteral("WpaFlags")).toUInt();
            const uint rsn = ap.value(QStringLiteral("RsnFlags")).toUInt();
            const uint flags = wpa | rsn;
            // NM_802_11_AP_FLAGS_PRIVACY marks WEP when no WPA/RSN key
            // management is advertised; never present such an AP as open.
            QString security = QStringLiteral("open");
            if (flags & 0x00000400u)
                security = QStringLiteral("sae");
            else if (flags & 0x00000100u)
                security = QStringLiteral("wpa-psk");
            else if (flags & 0x00000200u)
                security = QStringLiteral("enterprise");
            else if (flags != 0)
                security = QStringLiteral("unsupported");
            else if (apFlags & 0x00000001u)
                security = QStringLiteral("wep-unsupported");
            entry.insert(QStringLiteral("security"), security);
            entry.insert(QStringLiteral("secured"), security != QStringLiteral("open"));
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

void NetworkService::connectToNetwork(const QString &ssid, const QString &password,
                                       bool hidden, const QString &username, bool enterpriseAuth)
{
    if (!m_available || ssid.trimmed().isEmpty()) {
        setError(!m_available ? QStringLiteral("NetworkManager is unavailable.")
                              : QStringLiteral("Choose a network first."));
        emit operationFinished(false, m_lastError);
        return;
    }
    if (m_connecting) {
        setError(QStringLiteral("A Wi-Fi connection attempt is already in progress."));
        emit operationFinished(false, m_lastError);
        return;
    }

    QDBusObjectPath selectedDevice;
    QDBusObjectPath selectedAccessPoint;
    uint securityFlags = 0;
    uint accessPointFlags = 0;
    bool accessPointFound = false;
    const auto devices = objectPaths(kRoot, kManager, QStringLiteral("GetDevices"));
    for (const auto &devicePath : devices) {
        const QVariantMap device = allProperties(devicePath.path(), kDevice);
        if (device.value(QStringLiteral("DeviceType")).toUInt() != 2) continue;
        const auto points = objectPaths(devicePath.path(), kWireless,
                                        QStringLiteral("GetAllAccessPoints"));
        for (const auto &point : points) {
            const QVariantMap ap = allProperties(point.path(), kAccessPoint);
            if (ssidFrom(ap.value(QStringLiteral("Ssid"))) != ssid) continue;
            selectedDevice = devicePath;
            selectedAccessPoint = point;
            accessPointFound = true;
            securityFlags = ap.value(QStringLiteral("WpaFlags")).toUInt()
                          | ap.value(QStringLiteral("RsnFlags")).toUInt();
            accessPointFlags = ap.value(QStringLiteral("Flags")).toUInt();
            break;
        }
        if (!selectedDevice.path().isEmpty()) break;
    }

    if (selectedDevice.path().isEmpty() && hidden) {
        for (const auto &devicePath : devices) {
            const QVariantMap device = allProperties(devicePath.path(), kDevice);
            if (device.value(QStringLiteral("DeviceType")).toUInt() == 2) {
                selectedDevice = devicePath;
                break;
            }
        }
    }
    if (selectedDevice.path().isEmpty() || (!hidden && !accessPointFound)) {
        setError(hidden ? QStringLiteral("No Wi-Fi adapter is available for a hidden network.")
                        : QStringLiteral("That network is no longer visible. Scan again and retry."));
        emit operationFinished(false, m_lastError);
        return;
    }

    constexpr uint keyMgmtPsk = 0x00000100u;
    constexpr uint keyMgmt8021x = 0x00000200u;
    constexpr uint keyMgmtSae = 0x00000400u;
    const bool sae = (securityFlags & keyMgmtSae) != 0;
    const bool psk = (securityFlags & keyMgmtPsk) != 0;
    const bool enterpriseAvailable = (securityFlags & keyMgmt8021x) != 0;
    const bool enterprise = enterpriseAuth && (enterpriseAvailable || hidden);
    const bool wep = securityFlags == 0 && (accessPointFlags & 0x00000001u);
    const bool open = securityFlags == 0 && !wep && password.isEmpty() && !enterprise;

    if (wep) {
        setError(QStringLiteral("This network uses WEP, which Aether does not support. Use a WPA2/WPA3 network instead."));
        emit operationFinished(false, m_lastError);
        return;
    }
    if (enterpriseAuth && !enterpriseAvailable && !hidden) {
        setError(QStringLiteral("This access point does not advertise enterprise (802.1X) authentication."));
        emit operationFinished(false, m_lastError);
        return;
    }
    if (enterprise && (username.trimmed().isEmpty() || password.isEmpty())) {
        setError(QStringLiteral("Enter both your enterprise username and password."));
        emit operationFinished(false, m_lastError);
        return;
    }
    if (!open && !sae && !psk) {
        setError(QStringLiteral("This Wi-Fi security type is not supported yet (for example, legacy WEP)."));
        emit operationFinished(false, m_lastError);
        return;
    }
    if (!open && password.isEmpty()) {
        setError(QStringLiteral("Enter the Wi-Fi password to authenticate."));
        emit operationFinished(false, m_lastError);
        return;
    }
    const bool isHexPsk = password.size() == 64
        && std::all_of(password.cbegin(), password.cend(),
                       [](QChar ch) { return ch.isDigit()
                           || (ch.toLower() >= QLatin1Char('a')
                               && ch.toLower() <= QLatin1Char('f')); });
    if (!open && !enterprise && (password.size() < 8 || password.size() > 63)
        && !(psk && isHexPsk)) {
        setError(QStringLiteral("Enter an 8–63 character Wi-Fi passphrase, or a 64-character hexadecimal WPA pre-shared key."));
        emit operationFinished(false, m_lastError);
        return;
    }

    QVariantMap connection;
    connection.insert(QStringLiteral("id"), QStringLiteral("Aether - ") + ssid);
    connection.insert(QStringLiteral("type"), QStringLiteral("802-11-wireless"));
    connection.insert(QStringLiteral("autoconnect"), true);

    QVariantMap wireless;
    wireless.insert(QStringLiteral("ssid"), ssid.toUtf8());
    wireless.insert(QStringLiteral("mode"), QStringLiteral("infrastructure"));
    if (hidden) wireless.insert(QStringLiteral("hidden"), true);

    QVariantMap settings;
    settings.insert(QStringLiteral("connection"), connection);
    settings.insert(QStringLiteral("802-11-wireless"), wireless);
    if (enterprise) {
        QVariantMap security;
        security.insert(QStringLiteral("key-mgmt"), QStringLiteral("wpa-eap"));
        settings.insert(QStringLiteral("802-11-wireless-security"), security);
        QVariantMap eap;
        eap.insert(QStringLiteral("eap"), QStringList{QStringLiteral("peap")});
        eap.insert(QStringLiteral("identity"), username.trimmed());
        eap.insert(QStringLiteral("password"), password);
        eap.insert(QStringLiteral("password-flags"), 0u);
        eap.insert(QStringLiteral("phase2-auth"), QStringLiteral("mschapv2"));
        settings.insert(QStringLiteral("802-1x"), eap);
    } else if (!open) {
        QVariantMap security;
        // SAE is preferred when advertised, except that a 64-hex WPA PSK
        // is a raw PSK and must use wpa-psk rather than SAE.
        security.insert(QStringLiteral("key-mgmt"),
                        (sae && !isHexPsk) ? QStringLiteral("sae")
                                           : QStringLiteral("wpa-psk"));
        security.insert(QStringLiteral("psk"), password);
        // Store the secret in the NetworkManager system connection profile.
        security.insert(QStringLiteral("psk-flags"), 0u);
        settings.insert(QStringLiteral("802-11-wireless-security"), security);
    }
    QVariantMap ipv4;
    ipv4.insert(QStringLiteral("method"), QStringLiteral("auto"));
    settings.insert(QStringLiteral("ipv4"), ipv4);
    QVariantMap ipv6;
    ipv6.insert(QStringLiteral("method"), QStringLiteral("auto"));
    settings.insert(QStringLiteral("ipv6"), ipv6);

    QDBusInterface manager(kService, kRoot, kManager, QDBusConnection::systemBus());
    const QDBusMessage reply = manager.callWithArgumentList(
        QDBus::Block, QStringLiteral("AddAndActivateConnection"),
        {QVariant::fromValue(settings), QVariant::fromValue(selectedDevice),
         QVariant::fromValue(hidden ? QDBusObjectPath(QStringLiteral("/")) : selectedAccessPoint)});
    if (reply.type() == QDBusMessage::ErrorMessage) {
        setError(reply.errorMessage());
        emit operationFinished(false, m_lastError);
        return;
    }

    m_pendingSsid = ssid;
    m_connectionPolls = 0;
    setError(QString());
    setConnecting(true);
    setStatus(QStringLiteral("Authenticating and connecting…"));
    setConnectionState(QStringLiteral("authenticating"));
    QTimer::singleShot(500, this, [this]() { pollConnection(); });
}

void NetworkService::pollConnection()
{
    refresh();
    if (!m_connecting) return;
    if (m_activeSsid == m_pendingSsid) {
        const QString connected = m_pendingSsid;
        m_pendingSsid.clear();
        setConnecting(false);
        setConnectionState(QStringLiteral("connected"));
        setError(QString());
        emit operationFinished(true, QStringLiteral("Connected to ") + connected);
        return;
    }

    // Inspect the wireless device state before falling back to a timeout.
    const auto devices = objectPaths(kRoot, kManager, QStringLiteral("GetDevices"));
    for (const auto &devicePath : devices) {
        const QVariantMap device = allProperties(devicePath.path(), kDevice);
        if (device.value(QStringLiteral("DeviceType")).toUInt() != 2) continue;
        const uint state = device.value(QStringLiteral("State")).toUInt();
        const uint reason = stateReasonCode(device.value(QStringLiteral("StateReason")));
        if (state == 120 && reason != 0) {
            m_pendingSsid.clear();
            setConnecting(false);
            setConnectionState(QStringLiteral("failed"));
            setStatus(QStringLiteral("Connection failed"));
            setError(deviceReasonText(reason));
            emit operationFinished(false, m_lastError);
            return;
        }
        if (state == 60 || state == 40 || state == 50 || state == 70 || state == 80)
            setConnectionState(QStringLiteral("authenticating"));
        else if (state == 90)
            setConnectionState(QStringLiteral("configuring"));
    }
    ++m_connectionPolls;
    if (m_connectionPolls >= 60) {
        m_pendingSsid.clear();
        setConnecting(false);
        setConnectionState(QStringLiteral("failed"));
        if (m_lastError.isEmpty())
            setError(QStringLiteral("Connection timed out. Check the password and signal, then try again."));
        setStatus(QStringLiteral("Connection failed"));
        emit operationFinished(false, m_lastError);
        return;
    }
    QTimer::singleShot(500, this, [this]() { pollConnection(); });
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
    setConnectionState(QStringLiteral("disconnected"));
    emit operationFinished(true, QStringLiteral("Already disconnected"));
}
