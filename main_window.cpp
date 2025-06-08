#include "main_window.h"

MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("四路摄像头监控 - 单击全屏 (640x360/1280x720)");
    resize(1280, 720);

    // 主布局
    mainLayout = new QGridLayout(this);
    mainLayout->setSpacing(2);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setRowStretch(0, 1);
    mainLayout->setRowStretch(1, 1);
    mainLayout->setColumnStretch(0, 1);
    mainLayout->setColumnStretch(1, 1);

    // 创建4个摄像头视图 (每个640x360)
    for (int i = 0; i < 4; i++) {
        views[i] = new CameraView(i);
        views[i]->setInfoText(QString("摄像头 %1\n%2\n640x360").arg(i+1).arg(cameraDevices[i]));
        mainLayout->addWidget(views[i], i/2, i%2);

        connect(views[i], &CameraView::clicked, this, &MainWindow::toggleFullscreen);
    }

    // 初始化摄像头 (默认640x360分辨率)
    for (int i = 0; i < 4; i++) {
        if (cameras[i].start(cameraDevices[i], 640, 360)) {
            connect(&cameras[i], &V4L2Camera::fpsUpdated, this, [this, i](int fps) {
                if (fullscreenIndex == i) {
                    setWindowTitle(QString("摄像头%1 - %2x%3 @ %4 FPS (ESC退出)")
                                 .arg(i+1).arg(cameras[i].getCurrentWidth())
                                 .arg(cameras[i].getCurrentHeight()).arg(fps));
                }
            });
        } else {
            views[i]->setInfoText(QString("摄像头 %1\n初始化失败").arg(i+1));
        }
    }

    // 定时捕获帧
    frameTimer = new QTimer(this);
    connect(frameTimer, &QTimer::timeout, this, &MainWindow::updateFrames);
    frameTimer->start(33);  // ~30 FPS
}

MainWindow::~MainWindow()
{
    for (int i = 0; i < 4; i++) {
        cameras[i].stop();
    }
}

void MainWindow::updateFrames()
{
    for (int i = 0; i < 4; i++) {
        QImage frame = cameras[i].captureFrame();
        if (!frame.isNull()) {
            QPixmap pixmap = QPixmap::fromImage(frame).scaled(
                views[i]->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            views[i]->setPixmap(pixmap);
        }
    }
}

void MainWindow::toggleFullscreen(int index)
{
    if (fullscreenIndex == index) {
        // 退出全屏模式，恢复640x360分辨率
        if (safeResolutionChange(index, 640, 360)) {
            views[index]->setFullscreen(false);
            fullscreenIndex = -1;
            setWindowTitle("四路摄像头监控 - 单击全屏 (640x360/1280x720)");
            qDebug("Camera %d exited fullscreen, resolution set to 640x360", index+1);
        }
    } else {
        // 进入全屏模式，设置1280x720分辨率
        if (fullscreenIndex >= 0) {
            // 先退出当前全屏的摄像头
            safeResolutionChange(fullscreenIndex, 640, 360);
            views[fullscreenIndex]->setFullscreen(false);
        }

        if (safeResolutionChange(index, 1280, 720)) {
            fullscreenIndex = index;
            views[index]->setFullscreen(true);
            qDebug("Camera %d entered fullscreen, resolution set to 1280x720", index+1);
        }
    }
}

bool MainWindow::safeResolutionChange(int index, int width, int height)
{
    int retry = 0;
    while (retry < 3) {
        if (cameras[index].changeResolution(width, height)) {
            views[index]->setInfoText(QString("摄像头 %1\n%2\n%3x%4")
                                    .arg(index+1).arg(cameraDevices[index])
                                    .arg(width).arg(height));
            return true;
        }
        retry++;
        QThread::msleep(200); // 等待200ms再重试
    }

    QMessageBox::warning(this, "错误",
        QString("无法设置摄像头%1分辨率\n设备可能正忙").arg(index+1));
    return false;
}
