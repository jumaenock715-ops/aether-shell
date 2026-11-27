#pragma once

#include <QObject>
#include <QHash>
#include <QString>

class QSocketNotifier;
struct wl_display;
struct wl_registry;
struct ext_foreign_toplevel_list_v1;
struct ext_foreign_toplevel_handle_v1;

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
    struct WindowState {
        QString appId;
        QString windowId;
        QString title;
        bool announced = false;
    };

    static void registryGlobal(void *data, wl_registry *registry, quint32 name,
                               const char *interface, quint32 version);
    static void registryGlobalRemove(void *data, wl_registry *registry, quint32 name);
    static void managerToplevel(void *data, ext_foreign_toplevel_list_v1 *manager,
                                ext_foreign_toplevel_handle_v1 *handle);
    static void managerFinished(void *data, ext_foreign_toplevel_list_v1 *manager);
    static void handleClosed(void *data, ext_foreign_toplevel_handle_v1 *handle);
    static void handleDone(void *data, ext_foreign_toplevel_handle_v1 *handle);
    static void handleTitle(void *data, ext_foreign_toplevel_handle_v1 *handle,
                            const char *title);
    static void handleAppId(void *data, ext_foreign_toplevel_handle_v1 *handle,
                            const char *appId);
    static void handleIdentifier(void *data, ext_foreign_toplevel_handle_v1 *handle,
                                 const char *identifier);

    void setStatus(const QString &status);
    void dispatchPending();
    bool armRead();
    void handleReadable();
    void publishWindow(ext_foreign_toplevel_handle_v1 *handle, const QString &eventState);

    wl_display *m_display = nullptr;
    wl_registry *m_registry = nullptr;
    ext_foreign_toplevel_list_v1 *m_manager = nullptr;
    QSocketNotifier *m_readNotifier = nullptr;
    QString m_status = QStringLiteral("not-started");
    quint64 m_nextWindowId = 1;
    bool m_readPrepared = false;
    QHash<ext_foreign_toplevel_handle_v1 *, WindowState> m_windows;
};
