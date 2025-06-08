#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>
#include "camera_view.h"

class QGridLayout;

class MainWindow : public QWidget
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);

    ~MainWindow();

private slots:
    void updateFrames();

    void toggleFullscreen(int index);

private:
    bool safeResolutionChange(int index, int width, int height);

private:
    QGridLayout *mainLayout;
    CameraView *views[4];
    V4L2Camera cameras[4];
    QTimer *frameTimer;
    int fullscreenIndex;
    const QString cameraDevices[4] = {"/dev/video11", "/dev/video12", "/dev/video13", "/dev/video14"};

};
#endif // MAINWINDOW_H
