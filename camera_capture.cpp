#if defined(__aarch64__) || defined(__linux__)
#include "camera_capture.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <QDebug>

#include <jpeglib.h>
#include <QDebug>

MJpegDecoder::MJpegDecoder(QObject *parent) : QObject(parent) {}

QImage MJpegDecoder::decode(const QByteArray &mjpegData) {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    jpeg_mem_src(&cinfo, reinterpret_cast<const unsigned char*>(mjpegData.constData()), mjpegData.size());

    if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
        emit errorOccurred("JPEG头解析失败");
        jpeg_destroy_decompress(&cinfo);
        return QImage();
    }

    if (jpeg_start_decompress(&cinfo) != TRUE) {
        emit errorOccurred("JPEG解码启动失败");
        jpeg_destroy_decompress(&cinfo);
        return QImage();
    }

    QImage image(cinfo.output_width, cinfo.output_height, QImage::Format_RGB888);

    JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, cinfo.output_width * cinfo.output_components, 1);

    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, buffer, 1);
        uchar *dest = image.scanLine(cinfo.output_scanline - 1);
        memcpy(dest, buffer[0], cinfo.output_width * cinfo.output_components);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    return image;
}

CameraCapture::CameraCapture(const QString &device, QObject *parent)
    : QObject(parent), deviceName(device), fd(-1), buffers(nullptr), nBuffers(0), running(false)
{
    captureTimer = new QTimer(this);
    // 初始化解码器
    decoder = new MJpegDecoder(this);
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
    fmt.fmt.pix.width = 1280;
    fmt.fmt.pix.height = 720;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
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
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    struct timeval tv = {};
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    int r = select(fd + 1, &fds, nullptr, nullptr, &tv);
    if (r == -1) {
        emit errorOccurred("select错误");
    } else if (r == 0) {
        emit errorOccurred("采集超时");
    }

    struct v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
        emit errorOccurred("获取帧失败");
    }

    QByteArray frameData(static_cast<char*>(buffers[buf.index].start), buf.bytesused);

    // 重新将缓冲区放入队列
    if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
        emit errorOccurred("缓冲区重新入队失败");
    }

    if (!frameData.isEmpty()) {
        QImage image = decoder->decode(frameData);
        if (!image.isNull()) {
            emit newFrame(image);
        }
    }
}

#endif
