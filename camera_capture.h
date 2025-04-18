//#ifndef CAMERACAPTURE_H
//#define CAMERACAPTURE_H

//#include <QObject>
//#include <QImage>
//#include <QTimer>

//class CameraCapture : public QObject
//{
//    Q_OBJECT

//public:
//    explicit CameraCapture(QObject *parent = nullptr);

//    ~CameraCapture();

//    bool start();
//    void stop();

//signals:
//    void newFrame(const QImage &frame);
//    void error(const QString &message);

//private slots:
//    void captureFrame();

//private:
//    bool initDevice();
//    bool initMMap();
//    void uninitDevice();
//    QImage NV12ToRGB(const quint8* nv12, int width, int height);

//    int fd;
//    quint8* buffers[4];  // 映射的缓冲区
//    QSize frameSize;
//    QTimer *captureTimer;
//};
//#endif // CAMERACAPTURE_H
