#include "WaylandWindowTracker.h"

#include <QSocketNotifier>

#include <cerrno>
#include <cstring>

#if AETHER_HAVE_WAYLAND
#include <wayland-client.h>
#include "ext-foreign-toplevel-list-v1-client-protocol.h"
#endif

WaylandWindowTracker::WaylandWindowTracker(QObject *parent)
    : QObject(parent)
{
#if !AETHER_HAVE_WAYLAND
    setStatus(QStringLiteral("unsupported-build"));
#endif
}

WaylandWindowTracker::~WaylandWindowTracker()
{
    stop();
}

void WaylandWindowTracker::setStatus(const QString &status)
{
    if (m_status == status)
        return;
    m_status = status;
    emit statusChanged();
}

bool WaylandWindowTracker::start()
{
#if !AETHER_HAVE_WAYLAND
    emit diagnostic(QStringLiteral(
        "Wayland window tracking was not built: wayland-client and wayland-scanner are required."));
    return false;
#else
    if (m_display)
        return true;

    m_display = wl_display_connect(nullptr);
    if (!m_display) {
        setStatus(QStringLiteral("connection-failed"));
        emit diagnostic(QStringLiteral("Could not connect to the Wayland display."));
        return false;
    }

    m_registry = wl_display_get_registry(m_display);
    if (!m_registry) {
        setStatus(QStringLiteral("error"));
        emit diagnostic(QStringLiteral("Could not obtain the Wayland registry."));
        stop();
        return false;
    }

    static const wl_registry_listener registryListener = {
        &WaylandWindowTracker::registryGlobal,
        &WaylandWindowTracker::registryGlobalRemove
    };
    wl_registry_add_listener(m_registry, &registryListener, this);

    if (wl_display_roundtrip(m_display) < 0) {
        setStatus(QStringLiteral("error"));
        emit diagnostic(QStringLiteral("Wayland registry discovery failed."));
        stop();
        return false;
    }

    if (!m_manager) {
        setStatus(QStringLiteral("unsupported"));
        emit diagnostic(QStringLiteral(
            "The compositor does not expose ext-foreign-toplevel-list-v1 to this client."));
        stop();
        return false;
    }

    if (wl_display_roundtrip(m_display) < 0) {
        setStatus(QStringLiteral("error"));
        emit diagnostic(QStringLiteral("Initial toplevel enumeration failed."));
        stop();
        return false;
    }

    m_readNotifier = new QSocketNotifier(
        wl_display_get_fd(m_display), QSocketNotifier::Read, this);
    connect(m_readNotifier, &QSocketNotifier::activated,
            this, [this]() { handleReadable(); });

    setStatus(QStringLiteral("active"));
    if (!armRead()) {
        setStatus(QStringLiteral("error"));
        stop();
        return false;
    }
    return true;
#endif
}

void WaylandWindowTracker::stop()
{
#if AETHER_HAVE_WAYLAND
    if (m_readNotifier) {
        m_readNotifier->setEnabled(false);
        delete m_readNotifier;
        m_readNotifier = nullptr;
    }

    if (m_display && m_readPrepared) {
        wl_display_cancel_read(m_display);
        m_readPrepared = false;
    }
    if (m_display && m_manager) {
        ext_foreign_toplevel_list_v1_stop(m_manager);
        wl_display_roundtrip(m_display);
    }

    const auto handles = m_windows.keys();
    for (ext_foreign_toplevel_handle_v1 *handle : handles) {
        const WindowState state = m_windows.value(handle);
        if (state.announced)
            emit windowChanged(state.appId, state.windowId,
                               state.title, QStringLiteral("closed"));
        if (handle)
            ext_foreign_toplevel_handle_v1_destroy(handle);
    }
    m_windows.clear();

    if (m_manager) {
        ext_foreign_toplevel_list_v1_destroy(m_manager);
        m_manager = nullptr;
    }
    if (m_registry) {
        wl_registry_destroy(m_registry);
        m_registry = nullptr;
    }
    if (m_display) {
        wl_display_disconnect(m_display);
        m_display = nullptr;
    }
#endif
    if (m_status == QStringLiteral("active") ||
        m_status == QStringLiteral("connecting"))
        setStatus(QStringLiteral("stopped"));
}

#if AETHER_HAVE_WAYLAND
void WaylandWindowTracker::registryGlobal(void *data, wl_registry *registry,
                                          quint32 name, const char *interface,
                                          quint32 version)
{
    auto *self = static_cast<WaylandWindowTracker *>(data);
    if (!self || self->m_manager ||
        std::strcmp(interface, ext_foreign_toplevel_list_v1_interface.name) != 0)
        return;

    const quint32 supportedVersion = qMin<quint32>(version, 1);
    self->m_manager = static_cast<ext_foreign_toplevel_list_v1 *>(
        wl_registry_bind(registry, name,
                         &ext_foreign_toplevel_list_v1_interface,
                         supportedVersion));

    if (!self->m_manager)
        return;

    static const ext_foreign_toplevel_list_v1_listener managerListener = {
        &WaylandWindowTracker::managerToplevel,
        &WaylandWindowTracker::managerFinished
    };
    ext_foreign_toplevel_list_v1_add_listener(
        self->m_manager, &managerListener, self);
}

void WaylandWindowTracker::registryGlobalRemove(void *, wl_registry *, quint32)
{
    // The manager is connection-scoped. A compositor global disappearing is
    // handled as a connection capability loss on the next protocol error.
}

void WaylandWindowTracker::managerToplevel(
    void *data, ext_foreign_toplevel_list_v1 *,
    ext_foreign_toplevel_handle_v1 *handle)
{
    auto *self = static_cast<WaylandWindowTracker *>(data);
    if (!self || !handle)
        return;

    WindowState state;
    state.windowId = QStringLiteral("wayland-window-%1").arg(self->m_nextWindowId++);
    self->m_windows.insert(handle, state);

    static const ext_foreign_toplevel_handle_v1_listener handleListener = {
        &WaylandWindowTracker::handleClosed,
        &WaylandWindowTracker::handleDone,
        &WaylandWindowTracker::handleTitle,
        &WaylandWindowTracker::handleAppId,
        &WaylandWindowTracker::handleIdentifier
    };
    ext_foreign_toplevel_handle_v1_add_listener(
        handle, &handleListener, self);
}

void WaylandWindowTracker::managerFinished(
    void *data, ext_foreign_toplevel_list_v1 *manager)
{
    auto *self = static_cast<WaylandWindowTracker *>(data);
    if (!self)
        return;
    if (self->m_manager == manager) {
        ext_foreign_toplevel_list_v1_destroy(self->m_manager);
        self->m_manager = nullptr;
    }
    self->setStatus(QStringLiteral("finished"));
    emit self->diagnostic(QStringLiteral("The compositor ended the toplevel-list stream."));
}

void WaylandWindowTracker::handleClosed(
    void *data, ext_foreign_toplevel_handle_v1 *handle)
{
    auto *self = static_cast<WaylandWindowTracker *>(data);
    if (!self || !self->m_windows.contains(handle))
        return;

    const WindowState state = self->m_windows.take(handle);
    if (state.announced)
        emit self->windowChanged(state.appId, state.windowId,
                                 state.title, QStringLiteral("closed"));
    ext_foreign_toplevel_handle_v1_destroy(handle);
}

void WaylandWindowTracker::handleDone(
    void *data, ext_foreign_toplevel_handle_v1 *handle)
{
    auto *self = static_cast<WaylandWindowTracker *>(data);
    if (!self || !self->m_windows.contains(handle))
        return;

    WindowState &state = self->m_windows[handle];
    const QString eventState = state.announced
        ? QStringLiteral("updated") : QStringLiteral("opened");
    state.announced = true;
    self->publishWindow(handle, eventState);
}

void WaylandWindowTracker::handleTitle(
    void *data, ext_foreign_toplevel_handle_v1 *handle, const char *title)
{
    auto *self = static_cast<WaylandWindowTracker *>(data);
    if (!self || !self->m_windows.contains(handle))
        return;
    self->m_windows[handle].title = QString::fromUtf8(title ? title : "");
}

void WaylandWindowTracker::handleAppId(
    void *data, ext_foreign_toplevel_handle_v1 *handle, const char *appId)
{
    auto *self = static_cast<WaylandWindowTracker *>(data);
    if (!self || !self->m_windows.contains(handle))
        return;
    self->m_windows[handle].appId = QString::fromUtf8(appId ? appId : "");
}

void WaylandWindowTracker::handleIdentifier(
    void *data, ext_foreign_toplevel_handle_v1 *handle,
    const char *identifier)
{
    auto *self = static_cast<WaylandWindowTracker *>(data);
    if (!self || !self->m_windows.contains(handle) || !identifier || !*identifier)
        return;
    self->m_windows[handle].windowId = QString::fromUtf8(identifier);
}
#endif

void WaylandWindowTracker::publishWindow(
    ext_foreign_toplevel_handle_v1 *handle, const QString &eventState)
{
#if AETHER_HAVE_WAYLAND
    const auto it = m_windows.constFind(handle);
    if (it == m_windows.cend())
        return;
    emit windowChanged(it->appId, it->windowId, it->title, eventState);
#else
    Q_UNUSED(handle)
    Q_UNUSED(eventState)
#endif
}

void WaylandWindowTracker::dispatchPending()
{
#if AETHER_HAVE_WAYLAND
    if (!m_display)
        return;
    int result = 0;
    do {
        result = wl_display_dispatch_pending(m_display);
    } while (result > 0);
    if (result < 0) {
        setStatus(QStringLiteral("error"));
        emit diagnostic(QStringLiteral("Wayland event dispatch failed."));
        stop();
    }
#endif
}

bool WaylandWindowTracker::armRead()
{
#if AETHER_HAVE_WAYLAND
    if (!m_display || !m_readNotifier)
        return false;

    while (wl_display_prepare_read(m_display) != 0) {
        if (wl_display_dispatch_pending(m_display) < 0)
            return false;
    }

    if (wl_display_flush(m_display) < 0 && errno != EAGAIN) {
        wl_display_cancel_read(m_display);
        return false;
    }

    m_readPrepared = true;
    m_readNotifier->setEnabled(true);
    return true;
#else
    return false;
#endif
}

void WaylandWindowTracker::handleReadable()
{
#if AETHER_HAVE_WAYLAND
    if (!m_display || !m_readNotifier)
        return;

    m_readNotifier->setEnabled(false);
    if (wl_display_read_events(m_display) < 0) {
        m_readPrepared = false;
        setStatus(QStringLiteral("error"));
        emit diagnostic(QStringLiteral("Reading Wayland events failed."));
        stop();
        return;
    }
    m_readPrepared = false;

    dispatchPending();
    if (m_display && m_readNotifier && !armRead()) {
        setStatus(QStringLiteral("error"));
        emit diagnostic(QStringLiteral("Could not prepare the next Wayland read."));
        stop();
    }
#endif
}
