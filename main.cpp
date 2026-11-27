#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "ApplicationService.h"
#include "NotificationService.h"
#include "SessionManager.h"
#include "AndroidSessionManager.h"
#include "SessionSettings.h"
#include "WaydroidRuntimeAdapter.h"
#include "WaylandWindowTracker.h"
#include "AppCatalog.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("Aether");
    app.setOrganizationDomain("aetheros.local");
    app.setApplicationName("Aether Shell");

    ApplicationService applicationService;
    applicationService.reload();

    NotificationService notificationService;
    SessionManager sessionManager;
    SessionSettings sessionSettings;
    WaylandWindowTracker waylandWindowTracker;
    WaydroidRuntimeAdapter androidRuntimeAdapter;
    androidRuntimeAdapter.setWindowTracker(&waylandWindowTracker);
    AndroidSessionManager androidSessionManager(&androidRuntimeAdapter);
    applicationService.setSessionManager(&sessionManager);
    applicationService.setAndroidRuntimeAdapter(&androidRuntimeAdapter);
    applicationService.setAndroidSessionManager(&androidSessionManager);

    QQmlApplicationEngine engine;
    engine.addImageProvider("aethericon", new IconProvider);
    // Keep the existing QML-facing name so the current launcher needs no redesign.
    engine.rootContext()->setContextProperty("AppCatalog", &applicationService);
    // Preserve the existing Notifications QML API while moving its model into C++.
    engine.rootContext()->setContextProperty("Notifications", &notificationService);
    engine.rootContext()->setContextProperty("SessionManager", &sessionManager);
    engine.rootContext()->setContextProperty("SessionSettings", &sessionSettings);
    engine.rootContext()->setContextProperty("AndroidRuntime", &androidRuntimeAdapter);
    engine.rootContext()->setContextProperty("WaylandWindowTracker", &waylandWindowTracker);
    engine.rootContext()->setContextProperty("AndroidSessions", &androidSessionManager);
    // Non-empty only in CI: enables the automated click-through that saves screenshots.
    engine.rootContext()->setContextProperty("demoDir", QString::fromLocal8Bit(qgetenv("AETHER_DEMO_DIR")));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);
    engine.loadFromModule("Aether", "Main");
    // Tracking is best-effort and read-only; an unsupported compositor does
    // not prevent the existing shell from starting.
    waylandWindowTracker.start();
    return app.exec();
}
