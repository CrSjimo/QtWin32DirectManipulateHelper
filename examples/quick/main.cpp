#include <QQmlApplicationEngine>
#include <QGuiApplication>
#include <QQuickWindow>

#include <QWDMHCore/DirectManipulationSystem.h>

using namespace QWDMH;

int main(int argc, char *argv[]) {
    QGuiApplication a(argc, argv);
    DirectManipulationSystem dms;

    QQmlApplicationEngine engine;
    engine.load(":/qt/qml/QtWin32DirectManipulateHelper/Examples/Quick/main.qml");

    auto window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    DirectManipulationSystem::registerWindow(window);

    return a.exec();
}