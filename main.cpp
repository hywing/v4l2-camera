#if 0
#include <QApplication>
#include <QLabel>
#include <QImage>
#include <QTimer>
#include <QDebug>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <poll.h>
#include <linux/videodev2.h>

class V4L2Camera : public QObject {
    Q_OBJECT
public:
    explicit V4L2Camera(QObject *parent = nullptr) : QObject(parent), fd(-1) {}
    ~V4L2Camera() { stop(); }

    bool start(const QString &device, int width, int height) {
        // 打开设备
        fd = open(device.toUtf8().constData(), O_RDWR);
        if (fd < 0) {
            qWarning("Failed to open camera: %s", strerror(errno));
            return false;
        }

        // 设置多平面NV12格式
        struct v4l2_format fmt = {};
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        fmt.fmt.pix_mp.width = width;
        fmt.fmt.pix_mp.height = height;
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
        fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;
        fmt.fmt.pix_mp.num_planes = 1;  // NV12在V4L2中为单平面
        fmt.fmt.pix_mp.plane_fmt[0].bytesperline = width;
        fmt.fmt.pix_mp.plane_fmt[0].sizeimage = width * height * 3 / 2;  // NV12大小

        if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
            qWarning("Failed to set NV12 multiplanar format: %s", strerror(errno));
            close(fd);
            return false;
        }

        // 申请多平面缓冲区
        struct v4l2_requestbuffers req = {};
        req.count = 4;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        req.memory = V4L2_MEMORY_MMAP;

        if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
            qWarning("Failed to request buffers: %s", strerror(errno));
            close(fd);
            return false;
        }

        // 内存映射
        for (int i = 0; i < req.count; i++) {
            struct v4l2_buffer buf = {};
            struct v4l2_plane planes[VIDEO_MAX_PLANES] = {};
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            buf.length = 1;  // NV12单平面
            buf.m.planes = planes;

            if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
                qWarning("Failed to query buffer: %s", strerror(errno));
                close(fd);
                return false;
            }

            buffers[i] = static_cast<quint8*>(
                mmap(NULL, buf.m.planes[0].length, PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd, buf.m.planes[0].m.mem_offset));
            if (buffers[i] == MAP_FAILED) {
                qWarning("Failed to mmap buffer: %s", strerror(errno));
                close(fd);
                return false;
            }

            // 入队缓冲区
            if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
                qWarning("Failed to queue buffer: %s", strerror(errno));
                close(fd);
                return false;
            }
        }

        // 开始采集
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
            qWarning("Failed to start streaming: %s", strerror(errno));
            close(fd);
            return false;
        }

        frameSize = QSize(width, height);
        return true;
    }

    void stop() {
        if (fd >= 0) {
            enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            ioctl(fd, VIDIOC_STREAMOFF, &type);
            close(fd);
            fd = -1;
        }
    }

    // NV12转RGB（纯软件实现）
    QImage NV12ToRGB(const quint8* nv12, int width, int height) {
        QImage rgbImage(width, height, QImage::Format_RGB888);
        const quint8* yPlane = nv12;
        const quint8* uvPlane = nv12 + width * height;

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int yIdx = y * width + x;
                int uvIdx = ((y / 2) * (width / 2) + (x / 2)) * 2;

                // YUV转RGB公式
                int Y = yPlane[yIdx];
                int U = uvPlane[uvIdx] - 128;
                int V = uvPlane[uvIdx + 1] - 128;

                int R = qBound(0, (298 * Y + 409 * V + 128) >> 8, 255);
                int G = qBound(0, (298 * Y - 100 * U - 208 * V + 128) >> 8, 255);
                int B = qBound(0, (298 * Y + 516 * U + 128) >> 8, 255);

                rgbImage.setPixel(x, y, qRgb(R, G, B));
            }
        }
        return rgbImage;
    }

    QImage captureFrame() {
        if (fd < 0) return QImage();

        struct pollfd pfd = {fd, POLLIN, 0};
        if (poll(&pfd, 1, 1000) <= 0) {
            qWarning("Poll timeout");
            return QImage();
        }

        // 取出多平面缓冲区
        struct v4l2_buffer buf = {};
        struct v4l2_plane planes[VIDEO_MAX_PLANES] = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.length = 1;
        buf.m.planes = planes;

        if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
            qWarning("Failed to dequeue buffer: %s", strerror(errno));
            return QImage();
        }

        // 转换NV12为RGB
        QImage image = NV12ToRGB(buffers[buf.index], frameSize.width(), frameSize.height());

        // 重新入队
        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
            qWarning("Failed to requeue buffer: %s", strerror(errno));
        }

        return image;
    }

private:
    int fd;
    quint8* buffers[4];  // 映射的缓冲区
    QSize frameSize;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // 创建显示窗口
    QLabel label;
    label.resize(1280, 720);
    label.show();

    // 初始化摄像头
    V4L2Camera camera;
    if (!camera.start("/dev/video14", 1280, 720)) {
        qCritical("Camera initialization failed!");
        return -1;
    }

    // 定时捕获帧
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        QImage frame = camera.captureFrame();
        if (!frame.isNull()) {
            label.setPixmap(QPixmap::fromImage(frame.scaled(label.size(), Qt::KeepAspectRatio)));
        }
    });
    timer.start(33);  // ~30 FPS

    return app.exec();
}

#include "main.moc"  // 确保信号槽元对象编译
#else
#include <QApplication>
#include <QLabel>
#include <QImage>
#include <QTimer>
#include <QGridLayout>
#include <QWidget>
#include <QElapsedTimer>
#include <QDebug>
#include <QMouseEvent>
#include <QThreadPool>
#include <QRunnable>
#include <QPainter>
#include <QMessageBox>
#include <QStackedLayout>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <poll.h>
#include <linux/videodev2.h>

// 图像转换线程
class ImageConverter : public QRunnable {
public:
    ImageConverter(const quint8* nv12, QImage* rgbImage, int width, int height, int startY, int endY)
        : nv12(nv12), rgbImage(rgbImage), width(width), height(height), startY(startY), endY(endY) {}

    void run() override {
        const quint8* yPlane = nv12;
        const quint8* uvPlane = nv12 + width * height;

        for (int y = startY; y < endY; y++) {
            for (int x = 0; x < width; x++) {
                int yIdx = y * width + x;
                int uvIdx = ((y / 2) * (width / 2) + (x / 2)) * 2;

                int Y = yPlane[yIdx];
                int U = uvPlane[uvIdx] - 128;
                int V = uvPlane[uvIdx + 1] - 128;

                int R = qBound(0, (298 * Y + 409 * V + 128) >> 8, 255);
                int G = qBound(0, (298 * Y - 100 * U - 208 * V + 128) >> 8, 255);
                int B = qBound(0, (298 * Y + 516 * U + 128) >> 8, 255);

                rgbImage->setPixel(x, y, qRgb(R, G, B));
            }
        }
    }

private:
    const quint8* nv12;
    QImage* rgbImage;
    int width;
    int height;
    int startY;
    int endY;
};

// 摄像头控制类
class V4L2Camera : public QObject {
    Q_OBJECT
public:
    explicit V4L2Camera(QObject *parent = nullptr) : QObject(parent), fd(-1), fps(0), frameCount(0) {
        fpsTimer.start();
    }
    ~V4L2Camera() { stop(); }

    bool start(const QString &device, int width, int height) {
        if (fd >= 0 && currentWidth == width && currentHeight == height) {
            return true;
        }

        stop(); // 确保先停止当前设备

        currentWidth = width;
        currentHeight = height;

        // 尝试打开设备（带重试）
        for (int retry = 0; retry < 3; retry++) {
            fd = open(device.toUtf8().constData(), O_RDWR | O_NONBLOCK);
            if (fd >= 0) break;
            usleep(200000); // 等待200ms
        }

        if (fd < 0) {
            qWarning("Failed to open camera %s: %s", qPrintable(device), strerror(errno));
            return false;
        }

        // 设置格式（带重试）
        struct v4l2_format fmt = {};
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        fmt.fmt.pix_mp.width = width;
        fmt.fmt.pix_mp.height = height;
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
        fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;
        fmt.fmt.pix_mp.num_planes = 1;
        fmt.fmt.pix_mp.plane_fmt[0].bytesperline = width;
        fmt.fmt.pix_mp.plane_fmt[0].sizeimage = width * height * 3 / 2;

        for (int retry = 0; retry < 3; retry++) {
            if (ioctl(fd, VIDIOC_S_FMT, &fmt) == 0) break;
            if (retry == 2) {
                qWarning("Failed to set format: %s", strerror(errno));
                close(fd);
                fd = -1;
                return false;
            }
            usleep(200000); // 等待200ms
        }

        // 申请缓冲区
        struct v4l2_requestbuffers req = {};
        req.count = 4;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        req.memory = V4L2_MEMORY_MMAP;

        if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
            qWarning("Failed to request buffers: %s", strerror(errno));
            close(fd);
            fd = -1;
            return false;
        }

        // 内存映射
        for (int i = 0; i < req.count; i++) {
            struct v4l2_buffer buf = {};
            struct v4l2_plane planes[VIDEO_MAX_PLANES] = {};
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            buf.length = 1;
            buf.m.planes = planes;

            if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
                qWarning("Failed to query buffer: %s", strerror(errno));
                close(fd);
                fd = -1;
                return false;
            }

            buffers[i] = static_cast<quint8*>(
                mmap(NULL, buf.m.planes[0].length, PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd, buf.m.planes[0].m.mem_offset));
            if (buffers[i] == MAP_FAILED) {
                qWarning("Failed to mmap buffer: %s", strerror(errno));
                close(fd);
                fd = -1;
                return false;
            }

            if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
                qWarning("Failed to queue buffer: %s", strerror(errno));
                close(fd);
                fd = -1;
                return false;
            }
        }

        // 开始采集
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
            qWarning("Failed to start streaming: %s", strerror(errno));
            close(fd);
            fd = -1;
            return false;
        }

        deviceName = device;
        qDebug("Camera %s started at %dx%d", qPrintable(deviceName), currentWidth, currentHeight);
        return true;
    }

    bool changeResolution(int width, int height) {
        if (width == currentWidth && height == currentHeight) {
            return true;
        }
        return start(deviceName, width, height);
    }

    void stop() {
        if (fd >= 0) {
            // 停止采集
            enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            ioctl(fd, VIDIOC_STREAMOFF, &type);

            // 取消内存映射
            for (int i = 0; i < 4; i++) {
                if (buffers[i]) {
                    munmap(buffers[i], currentWidth * currentHeight * 3 / 2);
                    buffers[i] = nullptr;
                }
            }

            // 关闭设备
            close(fd);
            fd = -1;

            // 确保资源完全释放
            usleep(100000); // 等待100ms
            qDebug("Camera %s stopped", qPrintable(deviceName));
        }
    }

    QImage captureFrame() {
        if (fd < 0) return QImage();

        struct pollfd pfd = {fd, POLLIN, 0};
        if (poll(&pfd, 1, 33) <= 0) { // 33ms超时 (~30fps)
            return QImage();
        }

        struct v4l2_buffer buf = {};
        struct v4l2_plane planes[VIDEO_MAX_PLANES] = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.length = 1;
        buf.m.planes = planes;

        if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
            return QImage();
        }

        QImage image = NV12ToRGB(buffers[buf.index], currentWidth, currentHeight);

        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
            return QImage();
        }

        // 计算FPS
        frameCount++;
        if (fpsTimer.elapsed() >= 1000) {
            fps = frameCount * 1000 / fpsTimer.elapsed();
            frameCount = 0;
            fpsTimer.restart();
            emit fpsUpdated(fps);
        }

        return image;
    }

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

// 摄像头显示视图
class CameraView : public QLabel {
    Q_OBJECT
public:
    explicit CameraView(int index, QWidget *parent = nullptr)
        : QLabel(parent), cameraIndex(index), isFullscreen(false) {
        setAlignment(Qt::AlignCenter);
        setStyleSheet("background-color: black; color: white;");
        QFont font;
        font.setPointSize(12);
        setFont(font);
        setMinimumSize(320, 180);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    void setInfoText(const QString &text) {
        infoText = text;
        update();
    }

    void setFullscreen(bool fullscreen) {
        if (isFullscreen == fullscreen) return;

        isFullscreen = fullscreen;
        if (fullscreen) {
            // 保存原始父窗口和几何信息
            originalParent = parentWidget();
            originalGeometry = geometry();

            // 创建全屏窗口
            fullscreenWindow = new QWidget(nullptr, Qt::Window | Qt::FramelessWindowHint);
            fullscreenWindow->setStyleSheet("background-color: black;");
            fullscreenWindow->showFullScreen();

            // 将当前QLabel移动到全屏窗口
            setParent(fullscreenWindow);
            setGeometry(fullscreenWindow->rect());
            show();

            // 安装事件过滤器以处理退出全屏
            fullscreenWindow->installEventFilter(this);
        } else {
            if (fullscreenWindow) {
                // 恢复原始父窗口和几何信息
                setParent(originalParent);
                setGeometry(originalGeometry);
                show();

                // 删除全屏窗口
                fullscreenWindow->deleteLater();
                fullscreenWindow = nullptr;
            }
        }
    }

    bool isFullScreen() const { return isFullscreen; }

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            emit clicked(cameraIndex);
        }
        QLabel::mousePressEvent(event);
    }

    void paintEvent(QPaintEvent *event) override {
        QLabel::paintEvent(event);

        if (!pixmap() && !infoText.isEmpty()) {
            QPainter painter(this);
            painter.setPen(Qt::white);
            painter.drawText(rect(), Qt::AlignCenter, infoText);
        }
    }

    bool eventFilter(QObject *obj, QEvent *event) override {
        if (isFullscreen && obj == fullscreenWindow) {
            if (event->type() == QEvent::MouseButtonPress) {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                if (mouseEvent->button() == Qt::LeftButton) {
                    emit clicked(cameraIndex);
                    return true;
                }
            }
            else if (event->type() == QEvent::KeyPress) {
                QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
                if (keyEvent->key() == Qt::Key_Escape) {
                    emit clicked(cameraIndex);
                    return true;
                }
            }
        }
        return QLabel::eventFilter(obj, event);
    }

signals:
    void clicked(int index);

private:
    int cameraIndex;
    bool isFullscreen;
    QString infoText;
    QWidget *fullscreenWindow = nullptr;
    QWidget *originalParent = nullptr;
    QRect originalGeometry;
};

// 主窗口
class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr) : QWidget(parent), fullscreenIndex(-1) {
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

    ~MainWindow() {
        for (int i = 0; i < 4; i++) {
            cameras[i].stop();
        }
    }

private slots:
    void updateFrames() {
        for (int i = 0; i < 4; i++) {
            QImage frame = cameras[i].captureFrame();
            if (!frame.isNull()) {
                QPixmap pixmap = QPixmap::fromImage(frame).scaled(
                    views[i]->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                views[i]->setPixmap(pixmap);
            }
        }
    }

    void toggleFullscreen(int index) {
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

private:
    bool safeResolutionChange(int index, int width, int height) {
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

    QGridLayout *mainLayout;
    CameraView *views[4];
    V4L2Camera cameras[4];
    QTimer *frameTimer;
    int fullscreenIndex;
    const QString cameraDevices[4] = {"/dev/video11", "/dev/video12", "/dev/video13", "/dev/video14"};
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}

#include "main.moc"
#endif
