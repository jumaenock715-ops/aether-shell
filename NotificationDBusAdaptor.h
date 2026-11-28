#pragma once
#include <QDBusAbstractAdaptor>
#include <QDBusContext>
#include <QStringList>
#include <QVariantMap>
class NotificationService;
class NotificationDBusAdaptor final : public QDBusAbstractAdaptor, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Notifications")
    Q_CLASSINFO("D-Bus Introspection",
        "<interface name=\"org.freedesktop.Notifications\">"
        "<method name=\"GetCapabilities\"><arg direction=\"out\" type=\"as\"/></method>"
        "<method name=\"Notify\"><arg direction=\"in\" type=\"s\"/><arg direction=\"in\" type=\"u\"/>"
        "<arg direction=\"in\" type=\"s\"/><arg direction=\"in\" type=\"s\"/><arg direction=\"in\" type=\"s\"/>"
        "<arg direction=\"in\" type=\"as\"/><arg direction=\"in\" type=\"a{sv}\"/><arg direction=\"in\" type=\"i\"/>"
        "<arg direction=\"out\" type=\"u\"/></method>"
        "<method name=\"CloseNotification\"><arg direction=\"in\" type=\"u\"/></method>"
        "<method name=\"GetServerInformation\"><arg direction=\"out\" type=\"s\"/><arg direction=\"out\" type=\"s\"/>"
        "<arg direction=\"out\" type=\"s\"/><arg direction=\"out\" type=\"s\"/></method>"
        "<signal name=\"NotificationClosed\"><arg type=\"u\"/><arg type=\"u\"/></signal>"
        "<signal name=\"ActionInvoked\"><arg type=\"u\"/><arg type=\"s\"/></signal></interface>")
public:
    explicit NotificationDBusAdaptor(NotificationService *service);
public slots:
    QStringList GetCapabilities() const;
    uint Notify(const QString &appName, uint replacesId, const QString &appIcon,
        const QString &summary, const QString &body, const QStringList &actions,
        const QVariantMap &hints, int expireTimeout);
    void CloseNotification(uint id);
    void GetServerInformation();
signals:
    void NotificationClosed(uint id, uint reason);
    void ActionInvoked(uint id, const QString &actionKey);
private:
    NotificationService *m_service = nullptr;
};
