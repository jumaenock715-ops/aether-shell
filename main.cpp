#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "ApplicationService.h"
#include "AppCatalog.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("Aether Shell");

    ApplicationService applicationService;
    applicationService.reload();

    QQmlApplicationEngine engine;
    engine.addImageProvider("aethericon", new IconProvider);
    // Keep the existing QML-facing name so the current launcher needs no redesign.
    engine.rootContext()->setContextProperty("AppCatalog", &applicationService);
    // Non-empty only in CI: enables the automated click-through that saves screenshots.
    engine.rootContext()->setContextProperty("demoDir", QString::fromLocal8Bit(qgetenv("AETHER_DEMO_DIR")));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);
    engine.loadFromModule("Aether", "Main");
    return app.exec();
}
