// multicamerahandler.cpp
#include "multicamerahandler.h"
#include "camera_capture.h"
#include <QTimer>
#include <QDebug>
#include <QCameraInfo>
#include <QTimer>

#define WIDTH 640
#define HEIGHT 360

MultiCameraHandler::MultiCameraHandler(QObject *parent)
    : QObject(parent)
{
    this->initializeCamera();
}

MultiCameraHandler::~MultiCameraHandler()
{
    stopAllCameras();
}

void MultiCameraHandler::initializeCamera()
{
    {
        auto cameraCapture = new CameraCapture(this);
        m_captures << cameraCapture;
        m_cameras[0] = cameraCapture->init("/dev/video11", WIDTH, HEIGHT);
    }

    {
        auto cameraCapture = new CameraCapture(this);
        m_captures << cameraCapture;
        m_cameras[1] = cameraCapture->init("/dev/video12", WIDTH, HEIGHT);
    }

    {
        auto cameraCapture = new CameraCapture(this);
        m_captures << cameraCapture;
        m_cameras[2] = cameraCapture->init("/dev/video13", WIDTH, HEIGHT);
    }

    {
        auto cameraCapture = new CameraCapture(this);
        m_captures << cameraCapture;
        m_cameras[3] = cameraCapture->init("/dev/video14", WIDTH, HEIGHT);
    }
}

void MultiCameraHandler::startAllCameras()
{
    // 设置定时捕获
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, [this]() {
        processCapturedImage(0);
        processCapturedImage(1);
        processCapturedImage(2);
        processCapturedImage(3);
    });
    timer->start(33); // 约30fps
}

void MultiCameraHandler::stopAllCameras()
{
    timer->stop();
}

void MultiCameraHandler::processCapturedImage(int cameraIndex)
{
//    m_captures[cameraIndex]->captureFrame();

    emit cameraImageUpdated(cameraIndex);
}

QImage MultiCameraHandler::getCameraImage(int cameraIndex)
{
//    qDebug() << "time up!!!";
    return m_captures[cameraIndex]->captureFrame();
}

