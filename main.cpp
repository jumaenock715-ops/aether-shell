#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("Aether Shell");

    QQmlApplicationEngine engine;
    // Non-empty only in CI: enables the automated click-through that saves screenshots.
    engine.rootContext()->setContextProperty("demoDir", QString::fromLocal8Bit(qgetenv("AETHER_DEMO_DIR")));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);
    engine.loadFromModule("Aether", "Main");
    return app.exec();
}
