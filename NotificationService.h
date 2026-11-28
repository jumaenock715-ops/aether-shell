#pragma once
#include <QAbstractListModel>
#include <QVariantMap>
#include <QHash>
class NotificationDBusAdaptor;
class QTimer;
class NotificationService final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *items READ items CONSTANT)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString dbusStatus READ dbusStatus CONSTANT)
public:
    enum Role { IdRole = Qt::UserRole + 1, AppNameRole, AppIconRole, TitleRole, BodyRole, TimeRole };
    Q_ENUM(Role)
    explicit NotificationService(QObject *parent = nullptr);
    ~NotificationService() override;
    QAbstractItemModel *items();
    int count() const;
    QString dbusStatus() const { return m_dbusStatus; }
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE void push(const QString &title, const QString &body);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void invokeAction(uint id, const QString &actionKey);
    uint postDesktopNotification(const QString &appName, uint replacesId,
        const QString &appIcon, const QString &summary, const QString &body,
        const QStringList &actions, const QVariantMap &hints, int expireTimeout);
    bool closeNotification(uint id, uint reason = 3);
signals:
    void countChanged();
    void pushed(const QString &title, const QString &body);
    void notificationClosed(uint id, uint reason);
    void actionInvoked(uint id, const QString &actionKey);
private:
    int rowForId(uint id) const;
    uint nextNotificationId();
    QList<QVariantMap> m_notifications;
    uint m_nextId = 1;
    NotificationDBusAdaptor *m_dbusAdaptor = nullptr;
    QString m_dbusStatus = QStringLiteral("unavailable");
    bool m_dbusRegistered = false;
    QHash<uint, QTimer *> m_expiryTimers;
};
