#pragma once
#include <QObject>
#include <QHash>
#include <QString>
#include <QByteArray>

class QTimer;
class NotificationService;
class ClipboardService;

// File-based bridge to the optional Aether Android companion running inside
// Waydroid. The companion writes notifications and clipboard updates beneath
// Android/data/org.aetheros.bridge/files in shared external storage.
class WaydroidBridgeService final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(int forwardedNotifications READ forwardedNotifications NOTIFY changed)
public:
    WaydroidBridgeService(NotificationService *notifications,
                          ClipboardService *clipboard,
                          QObject *parent = nullptr);
    bool available() const { return m_available; }
    QString status() const { return m_status; }
    int forwardedNotifications() const { return m_forwardedNotifications; }
private:
    void poll();
    void discoverBridgeDirectory();
    void processNotificationEvents();
    void processClipboard();
    void writeHostClipboard(const QString &text);
    void setStatus(const QString &status);
    NotificationService *m_notifications = nullptr;
    ClipboardService *m_clipboard = nullptr;
    QTimer *m_timer = nullptr;
    QString m_sharedDirectory;
    QString m_bridgeDirectory;
    QString m_status = QStringLiteral("waiting-for-waydroid-storage");
    bool m_available = false;
    qint64 m_notificationOffset = 0;
    int m_forwardedNotifications = 0;
    QByteArray m_lastHostClipboard;
    QHash<QString, uint> m_notificationIds;
};
