#ifndef CAMERACAPTURE_H
#define CAMERACAPTURE_H

#include <QObject>
#include <QImage>

class CameraCapture : public QObject
{
    Q_OBJECT

public:
    explicit CameraCapture(QObject *parent = nullptr);

    ~CameraCapture();

    bool init(const QString &device, int width, int height);

    void start();

    void stop();

    QImage captureFrame();

private:
    QImage NV12ToRGB(const quint8* nv12, int width, int height);
    int fd;
    quint8* buffers[4];  // 映射的缓冲区
    QSize frameSize;
};
#endif // CAMERACAPTURE_H
