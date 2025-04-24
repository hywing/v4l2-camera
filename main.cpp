// main.cpp
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "multicamerahandler.h"
#include "multicameraimageprovider.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    // 创建多摄像头处理器
    MultiCameraHandler cameraHandler;

    // 创建并注册图像提供者
    MultiCameraImageProvider *imageProvider = new MultiCameraImageProvider(&cameraHandler);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("cameraHandler", &cameraHandler);
    engine.addImageProvider("multicamera", imageProvider);

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    // 启动所有摄像头
    cameraHandler.startAllCameras();

    return app.exec();
}
