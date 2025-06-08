#ifndef V4L2CAMERA_H
#define V4L2CAMERA_H

#include <QObject>
#include <QThread>
#include <QThreadPool>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <poll.h>
#include <linux/videodev2.h>

#include "image_converter.h"

// 摄像头控制类
class V4L2Camera : public QObject {
    Q_OBJECT
public:
    explicit V4L2Camera(QObject *parent = nullptr);

    ~V4L2Camera();

    bool start(const QString &device, int width, int height);

    bool changeResolution(int width, int height);

    void stop();

    QImage captureFrame();

    int getCurrentWidth() const { return currentWidth; }
    int getCurrentHeight() const { return currentHeight; }
    QString getDeviceName() const { return deviceName; }

signals:
    void fpsUpdated(int fps);

private:
    QImage NV12ToRGB(const quint8* nv12, int width, int height) {
        QImage rgbImage(width, height, QImage::Format_RGB888);

        const int threadCount = QThread::idealThreadCount();
        QThreadPool pool;
        pool.setMaxThreadCount(threadCount);

        int segmentHeight = height / threadCount;
        for (int i = 0; i < threadCount; i++) {
            int startY = i * segmentHeight;
            int endY = (i == threadCount - 1) ? height : (i + 1) * segmentHeight;

            ImageConverter* converter = new ImageConverter(nv12, &rgbImage, width, height, startY, endY);
            pool.start(converter);
        }

        pool.waitForDone();
        return rgbImage;
    }

    int fd;
    quint8* buffers[4] = {nullptr};
    QString deviceName;
    QElapsedTimer fpsTimer;
    int fps;
    int frameCount;
    int currentWidth;
    int currentHeight;
};

#endif // V4L2CAMERA_H
