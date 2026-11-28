#include "NotificationService.h"
#include "NotificationDBusAdaptor.h"
#include <QDateTime>
#include <QDBusConnection>
#include <QDebug>
#include <QTimer>
#include <utility>
namespace {
const QString kNotificationService = QStringLiteral("org.freedesktop.Notifications");
const QString kNotificationPath = QStringLiteral("/org/freedesktop/Notifications");
}
NotificationService::NotificationService(QObject *parent) : QAbstractListModel(parent)
{
    m_groupedItems.setSourceModel(this);
    m_groupedItems.setSortRole(AppNameRole);
    m_groupedItems.sort(0, Qt::AscendingOrder);
    m_dbusAdaptor = new NotificationDBusAdaptor(this);
    connect(this, &NotificationService::notificationClosed, m_dbusAdaptor, &NotificationDBusAdaptor::NotificationClosed);
    connect(this, &NotificationService::actionInvoked, m_dbusAdaptor, &NotificationDBusAdaptor::ActionInvoked);
    m_serviceWatcher = new QDBusServiceWatcher(
        kNotificationService, QDBusConnection::sessionBus(),
        QDBusServiceWatcher::WatchForOwnerChange, this);
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceOwnerChanged,
            this, [this](const QString &, const QString &, const QString &newOwner) {
        if (newOwner.isEmpty()) {
            // Do not silently leave Aether without notifications when another
            // desktop daemon exits after the shell has started.
            registerNotificationEndpoint();
        } else if (!m_dbusRegistered) {
            setDbusStatus(QStringLiteral("name-in-use"));
        }
    });
    registerNotificationEndpoint();
}
void NotificationService::setDbusStatus(const QString &status)
{
    if (m_dbusStatus == status) return;
    m_dbusStatus = status;
    emit dbusStatusChanged();
}

void NotificationService::registerNotificationEndpoint()
{
    if (m_dbusRegistered) return;
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        setDbusStatus(QStringLiteral("session-bus-unavailable"));
        return;
    }
    if (!bus.registerService(kNotificationService)) {
        setDbusStatus(QStringLiteral("name-in-use"));
        qWarning() << "Aether notifications could not own" << kNotificationService
                   << ":" << bus.lastError().message();
        return;
    }
    if (!bus.registerObject(kNotificationPath, this, QDBusConnection::ExportAdaptors)) {
        bus.unregisterService(kNotificationService);
        setDbusStatus(QStringLiteral("object-registration-failed"));
        qWarning() << "Aether notifications could not register" << kNotificationPath
                   << ":" << bus.lastError().message();
        return;
    }
    m_dbusRegistered = true;
    setDbusStatus(QStringLiteral("active"));
    qInfo() << "Aether owns" << kNotificationService;
}
NotificationService::~NotificationService()
{
    if (!m_dbusRegistered) return;
    QDBusConnection bus = QDBusConnection::sessionBus();
    bus.unregisterObject(kNotificationPath);
    bus.unregisterService(kNotificationService);
}
QAbstractItemModel *NotificationService::items() { return this; }
QAbstractItemModel *NotificationService::groupedItems() { return &m_groupedItems; }
int NotificationService::count() const { return m_notifications.size(); }
int NotificationService::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_notifications.size();
}
QVariant NotificationService::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_notifications.size()) return {};
    const QVariantMap &n = m_notifications.at(index.row());
    switch (role) {
    case IdRole: return n.value(QStringLiteral("id"));
    case AppNameRole: return n.value(QStringLiteral("appName"));
    case AppIconRole: return n.value(QStringLiteral("appIcon"));
    case TitleRole: return n.value(QStringLiteral("title"));
    case BodyRole: return n.value(QStringLiteral("body"));
    case TimeRole: return n.value(QStringLiteral("time"));
    case ActionsRole: return n.value(QStringLiteral("actions"));
    default: return {};
    }
}
QHash<int, QByteArray> NotificationService::roleNames() const
{
    return {{IdRole, "notificationId"}, {AppNameRole, "appName"}, {AppIconRole, "appIcon"},
            {TitleRole, "title"}, {BodyRole, "body"}, {TimeRole, "time"}, {ActionsRole, "actions"}};
}
int NotificationService::rowForId(uint id) const
{
    for (int row = 0; row < m_notifications.size(); ++row)
        if (m_notifications.at(row).value(QStringLiteral("id")).toUInt() == id) return row;
    return -1;
}
uint NotificationService::nextNotificationId()
{
    if (m_nextId == 0) ++m_nextId;
    const uint id = m_nextId++;
    if (m_nextId == 0) ++m_nextId;
    return id;
}
uint NotificationService::postDesktopNotification(
    const QString &appName, uint replacesId, const QString &appIcon,
    const QString &summary, const QString &body, const QStringList &actions,
    const QVariantMap &hints, int expireTimeout)
{
    uint id = replacesId;
    const int row = id == 0 ? -1 : rowForId(id);
    if (row < 0) id = nextNotificationId();
    QVariantMap n;
    n.insert(QStringLiteral("id"), id);
    n.insert(QStringLiteral("appName"), appName);
    n.insert(QStringLiteral("appIcon"), appIcon);
    n.insert(QStringLiteral("title"), summary);
    n.insert(QStringLiteral("body"), body);
    n.insert(QStringLiteral("time"), QDateTime::currentDateTime().toString(QStringLiteral("hh:mm")));
    n.insert(QStringLiteral("actions"), actions);
    n.insert(QStringLiteral("hints"), hints);
    n.insert(QStringLiteral("expireTimeout"), expireTimeout);
    if (m_expiryTimers.contains(id)) {
        QTimer *oldTimer = m_expiryTimers.take(id);
        oldTimer->stop();
        oldTimer->deleteLater();
    }
    if (expireTimeout >= 0) {
        auto *timer = new QTimer(this);
        timer->setSingleShot(true);
        connect(timer, &QTimer::timeout, this, [this, id]() {
            closeNotification(id, 1); // expired
        });
        m_expiryTimers.insert(id, timer);
        timer->start(expireTimeout == 0 ? 5000 : expireTimeout);
    }

    if (row >= 0) {
        m_notifications[row] = n;
        emit dataChanged(index(row, 0), index(row, 0));
    } else {
        beginInsertRows(QModelIndex(), 0, 0);
        m_notifications.prepend(n);
        endInsertRows();
        emit countChanged();
    }
    emit pushed(summary, body);
    return id;
}
void NotificationService::push(const QString &title, const QString &body)
{
    postDesktopNotification(QStringLiteral("Aether Shell"), 0, QString(), title, body, {}, {}, -1);
}
bool NotificationService::closeNotification(uint id, uint reason)
{
    const int row = rowForId(id);
    if (row < 0) return false;
    if (m_expiryTimers.contains(id)) {
        QTimer *timer = m_expiryTimers.take(id);
        timer->stop();
        timer->deleteLater();
    }
    beginRemoveRows(QModelIndex(), row, row);
    m_notifications.removeAt(row);
    endRemoveRows();
    emit countChanged();
    emit notificationClosed(id, reason);
    return true;
}
void NotificationService::invokeAction(uint id, const QString &actionKey)
{
    const int row = rowForId(id);
    if (row < 0 || actionKey.isEmpty()) return;
    const QStringList actions = m_notifications.at(row).value(QStringLiteral("actions")).toStringList();
    for (int i = 0; i + 1 < actions.size(); i += 2) {
        if (actions.at(i) == actionKey) { emit actionInvoked(id, actionKey); return; }
    }
}
void NotificationService::clear()
{
    if (m_notifications.isEmpty()) return;
    QList<uint> ids;
    ids.reserve(m_notifications.size());
    for (const QVariantMap &n : std::as_const(m_notifications))
        ids.append(n.value(QStringLiteral("id")).toUInt());
    for (QTimer *timer : std::as_const(m_expiryTimers)) {
        timer->stop();
        timer->deleteLater();
    }
    m_expiryTimers.clear();

    beginResetModel();
    m_notifications.clear();
    endResetModel();
    emit countChanged();
    for (uint id : std::as_const(ids)) emit notificationClosed(id, 2);
}
