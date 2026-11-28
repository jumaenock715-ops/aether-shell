#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "AppCatalog.h"
#include "SystemBridge.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("Aether Shell");

    AppCatalog catalog;
    catalog.reload();
    SystemBridge systemBridge;

    QQmlApplicationEngine engine;
    engine.addImageProvider("aethericon", new IconProvider);
    engine.rootContext()->setContextProperty("AppCatalog", &catalog);
    engine.rootContext()->setContextProperty("SystemBridge", &systemBridge);
    // Non-empty only in CI: enables the automated click-through that saves screenshots.
    engine.rootContext()->setContextProperty("demoDir", QString::fromLocal8Bit(qgetenv("AETHER_DEMO_DIR")));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);
    engine.loadFromModule("Aether", "Main");
    return app.exec();
}
