#if defined(__aarch64__) || defined(__linux__)
#ifndef CAMERACAPTURE_H
#define CAMERACAPTURE_H

#include <QObject>
#include <QImage>
#include <QTimer>
#include <QImage>

class MJpegDecoder : public QObject
{
    Q_OBJECT
public:
    explicit MJpegDecoder(QObject *parent = nullptr);
    QImage decode(const QByteArray &mjpegData);

signals:
    void errorOccurred(const QString &error);
};

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
    void errorOccurred(const QString &error);
    void newFrame(const QImage &frame);
    void error(const QString &message);

private slots:
    void captureFrame();

private:
    bool initDevice();
    bool initMMap();
    void uninitDevice();

    QString deviceName;
    int fd;
    struct buffer {
        void *start;
        size_t length;
    } *buffers;
    unsigned int nBuffers;
    QTimer *captureTimer;
    bool running;
    MJpegDecoder *decoder;
};
#endif // CAMERACAPTURE_H
#endif
