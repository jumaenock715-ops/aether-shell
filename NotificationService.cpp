#include "NotificationService.h"
#include "NotificationDBusAdaptor.h"
#include <QDateTime>
#include <QDBusConnection>
#include <QDebug>
#include <utility>
namespace {
const QString kNotificationService = QStringLiteral("org.freedesktop.Notifications");
const QString kNotificationPath = QStringLiteral("/org/freedesktop/Notifications");
}
NotificationService::NotificationService(QObject *parent) : QAbstractListModel(parent)
{
    m_dbusAdaptor = new NotificationDBusAdaptor(this);
    connect(this, &NotificationService::notificationClosed, m_dbusAdaptor, &NotificationDBusAdaptor::NotificationClosed);
    connect(this, &NotificationService::actionInvoked, m_dbusAdaptor, &NotificationDBusAdaptor::ActionInvoked);
    const QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        m_dbusStatus = QStringLiteral("session-bus-unavailable");
        qWarning() << "Aether notifications: session D-Bus is unavailable";
        return;
    }
    if (!bus.registerService(kNotificationService)) {
        m_dbusStatus = QStringLiteral("name-in-use");
        qWarning() << "Aether notifications could not own" << kNotificationService << ":" << bus.lastError().message();
        return;
    }
    if (!bus.registerObject(kNotificationPath, this, QDBusConnection::ExportAdaptors)) {
        bus.unregisterService(kNotificationService);
        m_dbusStatus = QStringLiteral("object-registration-failed");
        qWarning() << "Aether notifications could not register" << kNotificationPath << ":" << bus.lastError().message();
        return;
    }
    m_dbusRegistered = true;
    m_dbusStatus = QStringLiteral("active");
}
NotificationService::~NotificationService()
{
    if (!m_dbusRegistered) return;
    const QDBusConnection bus = QDBusConnection::sessionBus();
    bus.unregisterObject(kNotificationPath);
    bus.unregisterService(kNotificationService);
}
QAbstractItemModel *NotificationService::items() { return this; }
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
    default: return {};
    }
}
QHash<int, QByteArray> NotificationService::roleNames() const
{
    return {{IdRole, "notificationId"}, {AppNameRole, "appName"}, {AppIconRole, "appIcon"},
            {TitleRole, "title"}, {BodyRole, "body"}, {TimeRole, "time"}};
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
    postDesktopNotification(QStringLiteral("Aether Shell"), 0, QString(), title, body, {}, {}, 0);
}
bool NotificationService::closeNotification(uint id, uint reason)
{
    const int row = rowForId(id);
    if (row < 0) return false;
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
    beginResetModel();
    m_notifications.clear();
    endResetModel();
    emit countChanged();
    for (uint id : std::as_const(ids)) emit notificationClosed(id, 2);
}
