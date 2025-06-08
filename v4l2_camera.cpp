#include "v4l2_camera.h"

V4L2Camera::V4L2Camera(QObject *parent) : QObject(parent), fd(-1), fps(0), frameCount(0)
{
    fpsTimer.start();
}

V4L2Camera::~V4L2Camera()
{
    stop();
}

bool V4L2Camera::start(const QString &device, int width, int height)
{
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

bool V4L2Camera::changeResolution(int width, int height)
{
    if (width == currentWidth && height == currentHeight) {
        return true;
    }
    return start(deviceName, width, height);
}

void V4L2Camera::stop()
{
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

QImage V4L2Camera::captureFrame()
{
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
