#include "NotificationDBusAdaptor.h"
#include "NotificationService.h"
#include <QDBusMessage>
NotificationDBusAdaptor::NotificationDBusAdaptor(NotificationService *service)
    : QDBusAbstractAdaptor(service), m_service(service) {}
QStringList NotificationDBusAdaptor::GetCapabilities() const
{
    // Aether renders plain-text bodies and exposes action buttons in its centre.
    return {QStringLiteral("body"), QStringLiteral("actions")};
}
uint NotificationDBusAdaptor::Notify(const QString &appName, uint replacesId,
    const QString &appIcon, const QString &summary, const QString &body,
    const QStringList &actions, const QVariantMap &hints, int expireTimeout)
{
    return m_service->postDesktopNotification(appName, replacesId, appIcon,
        summary, body, actions, hints, expireTimeout);
}
void NotificationDBusAdaptor::CloseNotification(uint id)
{
    m_service->closeNotification(id, 3);
}
void NotificationDBusAdaptor::GetServerInformation()
{
    if (!calledFromDBus()) return;
    const QVariantList info{QStringLiteral("Aether OS"), QStringLiteral("Aether Project"),
                            QStringLiteral("0.1"), QStringLiteral("1.2")};
    setDelayedReply(true);
    connection().send(message().createReply(info));
}
