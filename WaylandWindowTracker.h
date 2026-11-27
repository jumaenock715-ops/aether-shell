#pragma once

#include <QObject>
#include <QString>

class QSocketNotifier;
struct wl_display;
struct wl_registry;
struct ext_foreign_toplevel_list_v1;

class WaylandWindowTracker final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    explicit WaylandWindowTracker(QObject *parent = nullptr);
    ~WaylandWindowTracker() override;

    QString status() const { return m_status; }
    Q_INVOKABLE bool start();
    Q_INVOKABLE void stop();

signals:
    void statusChanged();
    // Raw compositor identity only. It is not an Android package name.
    // eventState is opened, updated, or closed.
    void windowChanged(const QString &appId, const QString &windowId,
                       const QString &title, const QString &eventState);
    void diagnostic(const QString &message);

private:
    void setStatus(const QString &status);
    void dispatchPending();
    bool armRead();
    void handleReadable();
    void handleRegistryGlobal(const char *interface, quint32 name, quint32 version);
    void handleToplevel(void *handle);
    void handleTitle(void *handle, const char *title);
    void handleAppId(void *handle, const char *appId);
    void handleIdentifier(void *handle, const char *identifier);
    void handleDone(void *handle);
    void handleClosed(void *handle);

    wl_display *m_display = nullptr;
    wl_registry *m_registry = nullptr;
    ext_foreign_toplevel_list_v1 *m_manager = nullptr;
    QSocketNotifier *m_readNotifier = nullptr;
    QString m_status = QStringLiteral("not-started");
    quint64 m_nextWindowId = 1;
};
