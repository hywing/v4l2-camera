#ifndef CAMERACAPTURE_H
#define CAMERACAPTURE_H

#include <QObject>
#include <QImage>
#include <QTimer>

class CameraCapture : public QObject
{
    Q_OBJECT

public:
    explicit CameraCapture(const QString &device, QObject *parent = nullptr);
    ~CameraCapture();

    bool start();
    void stop();
    bool isRunning() const;

signals:
    void newFrame(const QImage &frame);
    void error(const QString &message);

private slots:
    void captureFrame();

private:
    bool initDevice();
    bool initMMap();
    void uninitDevice();
    QImage convertYUYVToRGB(const unsigned char *data, int width, int height);

    QString deviceName;
    int fd;
    struct buffer {
        void *start;
        size_t length;
    } *buffers;
    unsigned int nBuffers;
    QTimer *captureTimer;
    bool running;
};
#endif // CAMERACAPTURE_H
