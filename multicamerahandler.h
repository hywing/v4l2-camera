// multicamerahandler.h
#ifndef MULTICAMERAHANDLER
#define MULTICAMERAHANDLER
#include <QObject>
#include <QImage>
#include <QCamera>
#include <QCameraImageCapture>
#include <QVector>
#include <QMutex>

class CameraCapture;

class MultiCameraHandler : public QObject
{
    Q_OBJECT
public:
    explicit MultiCameraHandler(QObject *parent = nullptr);
    ~MultiCameraHandler();

    Q_INVOKABLE void startAllCameras();
    Q_INVOKABLE void stopAllCameras();

    QImage getCameraImage(int cameraIndex);
    int cameraCount() const { return 4; } // 假设固定4路摄像头

signals:
    void cameraImageUpdated(int cameraIndex);

private slots:
    void processCapturedImage(int cameraIndex);

private:
    QList<CameraCapture *> m_captures;
    bool m_cameras[4] = {false, false, false, false};
    QTimer *timer;

    void initializeCamera();
};
#endif
