#include "camera_capture.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <QDebug>

CameraCapture::CameraCapture(const QString &device, QObject *parent)
    : QObject(parent), deviceName(device), fd(-1), buffers(nullptr), nBuffers(0), running(false)
{
    captureTimer = new QTimer(this);
    connect(captureTimer, &QTimer::timeout, this, &CameraCapture::captureFrame);
}

CameraCapture::~CameraCapture()
{
    stop();
}

bool CameraCapture::start()
{
    if (running) return true;

    if (!initDevice()) {
        emit error("Failed to initialize device");
        return false;
    }

    if (!initMMap()) {
        emit error("Failed to initialize memory mapping");
        uninitDevice();
        return false;
    }

    // Start capturing
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) == -1) {
        emit error("Failed to start streaming");
        uninitDevice();
        return false;
    }

    running = true;
    captureTimer->start(33); // ~30fps
    return true;
}

void CameraCapture::stop()
{
    if (!running) return;

    captureTimer->stop();

    // Stop capturing
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) == -1) {
        qWarning() << "Failed to stop streaming";
    }

    uninitDevice();
    running = false;
}

bool CameraCapture::isRunning() const
{
    return running;
}

bool CameraCapture::initDevice()
{
    fd = open(deviceName.toLocal8Bit().constData(), O_RDWR | O_NONBLOCK, 0);
    if (fd == -1) {
        qWarning() << "Cannot open" << deviceName;
        return false;
    }

    // Check capabilities
    struct v4l2_capability cap;
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
        qWarning() << "Failed to query capabilities";
        return false;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        qWarning() << "Device does not support video capture";
        return false;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        qWarning() << "Device does not support streaming";
        return false;
    }

    // Set format
    struct v4l2_format fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = 640;
    fmt.fmt.pix.height = 480;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
        qWarning() << "Failed to set format";
        return false;
    }

    return true;
}

bool CameraCapture::initMMap()
{
    struct v4l2_requestbuffers req = {};
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
        qWarning() << "Failed to request buffers";
        return false;
    }

    if (req.count < 2) {
        qWarning() << "Insufficient buffer memory";
        return false;
    }

    buffers = new buffer[req.count];
    nBuffers = req.count;

    for (unsigned int i = 0; i < req.count; ++i) {
        struct v4l2_buffer buf = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
            qWarning() << "Failed to query buffer";
            return false;
        }

        buffers[i].length = buf.length;
        buffers[i].start = mmap(NULL, buf.length,
                               PROT_READ | PROT_WRITE, MAP_SHARED,
                               fd, buf.m.offset);

        if (buffers[i].start == MAP_FAILED) {
            qWarning() << "Failed to map buffer";
            return false;
        }

        // Queue the buffer
        if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
            qWarning() << "Failed to queue buffer";
            return false;
        }
    }

    return true;
}

void CameraCapture::uninitDevice()
{
    if (buffers) {
        for (unsigned int i = 0; i < nBuffers; ++i) {
            if (buffers[i].start != MAP_FAILED) {
                munmap(buffers[i].start, buffers[i].length);
            }
        }
        delete[] buffers;
        buffers = nullptr;
        nBuffers = 0;
    }

    if (fd != -1) {
        close(fd);
        fd = -1;
    }
}

void CameraCapture::captureFrame()
{
    struct v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
        emit error("Failed to dequeue buffer");
        return;
    }

    // Process the frame
    QImage image = convertYUYVToRGB(static_cast<unsigned char*>(buffers[buf.index].start),
           640, 480);
    emit newFrame(image);

    // Requeue the buffer
    if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
        emit error("Failed to requeue buffer");
    }
}

QImage CameraCapture::convertYUYVToRGB(const unsigned char *data, int width, int height)
{
    QImage image(width, height, QImage::Format_RGB888);

    for (int y = 0; y < height; ++y) {
        const unsigned char *src = data + y * width * 2;
        unsigned char *dst = image.scanLine(y);

        for (int x = 0; x < width; x += 2) {
            // YUYV to RGB conversion
            int y0 = src[0];
            int u = src[1];
            int y1 = src[2];
            int v = src[3];

            // First pixel (Y0 U V)
            int c = y0 - 16;
            int d = u - 128;
            int e = v - 128;

            dst[0] = qBound(0, (298 * c + 409 * e + 128) >> 8, 255); // R
            dst[1] = qBound(0, (298 * c - 100 * d - 208 * e + 128) >> 8, 255); // G
            dst[2] = qBound(0, (298 * c + 516 * d + 128) >> 8, 255); // B

            // Second pixel (Y1 U V)
            c = y1 - 16;
            dst[3] = qBound(0, (298 * c + 409 * e + 128) >> 8, 255); // R
            dst[4] = qBound(0, (298 * c - 100 * d - 208 * e + 128) >> 8, 255); // G
            dst[5] = qBound(0, (298 * c + 516 * d + 128) >> 8, 255); // B

            src += 4;
            dst += 6;
        }
    }

    return image;
}
