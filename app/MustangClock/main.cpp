#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include<QQmlContext>
#include "BleManager.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    BleManager bleManager;
    engine.rootContext()->setContextProperty("bleManager", &bleManager);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("MustangClock", "Main");

    return app.exec();
}
