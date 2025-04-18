//#include "camera_capture.h"
//#include <fcntl.h>
//#include <unistd.h>
//#include <sys/ioctl.h>
//#include <sys/mman.h>
//#include <linux/videodev2.h>
//#include <QTimer>
//#include <QDebug>

//#define WIDTH 1280
//#define HEIGHT 720

//CameraCapture::CameraCapture(QObject *parent)
//    : QObject(parent), fd(-1)
//{
//    captureTimer = new QTimer(this);
//    connect(captureTimer, &QTimer::timeout, this, &CameraCapture::captureFrame);
//}

//CameraCapture::~CameraCapture()
//{
//    stop();
//}

//bool CameraCapture::start(const QString &device, int width, int height)
//{
//    // 打开设备
//    fd = open(device.toUtf8().constData(), O_RDWR);
//    if (fd < 0) {
//        qWarning("Failed to open camera: %s", strerror(errno));
//        return false;
//    }

//    // 设置多平面NV12格式
//    struct v4l2_format fmt = {};
//    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
//    fmt.fmt.pix_mp.width = width;
//    fmt.fmt.pix_mp.height = height;
//    fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
//    fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;
//    fmt.fmt.pix_mp.num_planes = 1;  // NV12在V4L2中为单平面
//    fmt.fmt.pix_mp.plane_fmt[0].bytesperline = width;
//    fmt.fmt.pix_mp.plane_fmt[0].sizeimage = width * height * 3 / 2;  // NV12大小

//    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
//        qWarning("Failed to set NV12 multiplanar format: %s", strerror(errno));
//        close(fd);
//        return false;
//    }

//    // 申请多平面缓冲区
//    struct v4l2_requestbuffers req = {};
//    req.count = 4;
//    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
//    req.memory = V4L2_MEMORY_MMAP;

//    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
//        qWarning("Failed to request buffers: %s", strerror(errno));
//        close(fd);
//        return false;
//    }

//    // 内存映射
//    for (int i = 0; i < req.count; i++) {
//        struct v4l2_buffer buf = {};
//        struct v4l2_plane planes[VIDEO_MAX_PLANES] = {};
//        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
//        buf.memory = V4L2_MEMORY_MMAP;
//        buf.index = i;
//        buf.length = 1;  // NV12单平面
//        buf.m.planes = planes;

//        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
//            qWarning("Failed to query buffer: %s", strerror(errno));
//            close(fd);
//            return false;
//        }

//        buffers[i] = static_cast<quint8*>(
//            mmap(NULL, buf.m.planes[0].length, PROT_READ | PROT_WRITE,
//                 MAP_SHARED, fd, buf.m.planes[0].m.mem_offset));
//        if (buffers[i] == MAP_FAILED) {
//            qWarning("Failed to mmap buffer: %s", strerror(errno));
//            close(fd);
//            return false;
//        }

//        // 入队缓冲区
//        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
//            qWarning("Failed to queue buffer: %s", strerror(errno));
//            close(fd);
//            return false;
//        }
//    }

//    // 开始采集
//    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
//    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
//        qWarning("Failed to start streaming: %s", strerror(errno));
//        close(fd);
//        return false;
//    }

//    frameSize = QSize(width, height);
//    return true;
//}

//void CameraCapture::stop()
//{
//    if (fd >= 0) {
//        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
//        ioctl(fd, VIDIOC_STREAMOFF, &type);
//        close(fd);
//        fd = -1;
//    }
//}

//bool CameraCapture::isRunning() const
//{
//    return running;
//}

//bool CameraCapture::initDevice()
//{
//    fd = open(deviceName.toLocal8Bit().constData(), O_RDWR | O_NONBLOCK, 0);
//    if (fd == -1) {
//        qWarning() << "Cannot open" << deviceName;
//        return false;
//    }

//    // Check capabilities
//    struct v4l2_capability cap;
//    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
//        qWarning() << "Failed to query capabilities";
//        return false;
//    }

//    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
//        qWarning() << "Device does not support video capture";
//        return false;
//    }

//    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
//        qWarning() << "Device does not support streaming";
//        return false;
//    }

//    // Set format
//    struct v4l2_format fmt = {};
//    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//    fmt.fmt.pix.width = WIDTH;
//    fmt.fmt.pix.height = HEIGHT;
//    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
//    fmt.fmt.pix.field = V4L2_FIELD_NONE;

//    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
//        qWarning() << "Failed to set format";
//        return false;
//    }

//    return true;
//}

//bool CameraCapture::initMMap()
//{
//    struct v4l2_requestbuffers req = {};
//    req.count = 4;
//    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//    req.memory = V4L2_MEMORY_MMAP;

//    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
//        qWarning() << "Failed to request buffers";
//        return false;
//    }

//    if (req.count < 2) {
//        qWarning() << "Insufficient buffer memory";
//        return false;
//    }

//    buffers = new buffer[req.count];
//    nBuffers = req.count;

//    for (unsigned int i = 0; i < req.count; ++i) {
//        struct v4l2_buffer buf = {};
//        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//        buf.memory = V4L2_MEMORY_MMAP;
//        buf.index = i;

//        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
//            qWarning() << "Failed to query buffer";
//            return false;
//        }

//        buffers[i].length = buf.length;
//        buffers[i].start = mmap(NULL, buf.length,
//                               PROT_READ | PROT_WRITE, MAP_SHARED,
//                               fd, buf.m.offset);

//        if (buffers[i].start == MAP_FAILED) {
//            qWarning() << "Failed to map buffer";
//            return false;
//        }

//        // Queue the buffer
//        if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
//            qWarning() << "Failed to queue buffer";
//            return false;
//        }
//    }

//    return true;
//}

//void CameraCapture::uninitDevice()
//{
//    if (buffers) {
//        for (unsigned int i = 0; i < nBuffers; ++i) {
//            if (buffers[i].start != MAP_FAILED) {
//                munmap(buffers[i].start, buffers[i].length);
//            }
//        }
//        delete[] buffers;
//        buffers = nullptr;
//        nBuffers = 0;
//    }

//    if (fd != -1) {
//        close(fd);
//        fd = -1;
//    }
//}

//void CameraCapture::captureFrame()
//{
//    if (fd < 0) return QImage();

//    struct pollfd pfd = {fd, POLLIN, 0};
//    if (poll(&pfd, 1, 1000) <= 0) {
//        qWarning("Poll timeout");
//        return QImage();
//    }

//    // 取出多平面缓冲区
//    struct v4l2_buffer buf = {};
//    struct v4l2_plane planes[VIDEO_MAX_PLANES] = {};
//    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
//    buf.memory = V4L2_MEMORY_MMAP;
//    buf.length = 1;
//    buf.m.planes = planes;

//    if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
//        qWarning("Failed to dequeue buffer: %s", strerror(errno));
//        return QImage();
//    }

//    // 转换NV12为RGB
//    QImage image = NV12ToRGB(buffers[buf.index], frameSize.width(), frameSize.height());

//    emit this->newFrame(image);

//    // 重新入队
//    if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
//        qWarning("Failed to requeue buffer: %s", strerror(errno));
//    }
//}


//QImage CameraCapture::NV12ToRGB(const quint8 *nv12, int width, int height)
//{
//    QImage rgbImage(width, height, QImage::Format_RGB888);
//    const quint8* yPlane = nv12;
//    const quint8* uvPlane = nv12 + width * height;

//    for (int y = 0; y < height; y++) {
//        for (int x = 0; x < width; x++) {
//            int yIdx = y * width + x;
//            int uvIdx = ((y / 2) * (width / 2) + (x / 2)) * 2;

//            // YUV转RGB公式
//            int Y = yPlane[yIdx];
//            int U = uvPlane[uvIdx] - 128;
//            int V = uvPlane[uvIdx + 1] - 128;

//            int R = qBound(0, (298 * Y + 409 * V + 128) >> 8, 255);
//            int G = qBound(0, (298 * Y - 100 * U - 208 * V + 128) >> 8, 255);
//            int B = qBound(0, (298 * Y + 516 * U + 128) >> 8, 255);

//            rgbImage.setPixel(x, y, qRgb(R, G, B));
//        }
//    }
//    return rgbImage;
//}
