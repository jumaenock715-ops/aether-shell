#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "ApplicationService.h"
#include "NotificationService.h"
#include "SessionManager.h"
#include "AppCatalog.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("Aether Shell");

    ApplicationService applicationService;
    applicationService.reload();

    NotificationService notificationService;
    SessionManager sessionManager;
    applicationService.setSessionManager(&sessionManager);

    QQmlApplicationEngine engine;
    engine.addImageProvider("aethericon", new IconProvider);
    // Keep the existing QML-facing name so the current launcher needs no redesign.
    engine.rootContext()->setContextProperty("AppCatalog", &applicationService);
    // Preserve the existing Notifications QML API while moving its model into C++.
    engine.rootContext()->setContextProperty("Notifications", &notificationService);
    engine.rootContext()->setContextProperty("SessionManager", &sessionManager);
    // Non-empty only in CI: enables the automated click-through that saves screenshots.
    engine.rootContext()->setContextProperty("demoDir", QString::fromLocal8Bit(qgetenv("AETHER_DEMO_DIR")));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);
    engine.loadFromModule("Aether", "Main");
    return app.exec();
}
