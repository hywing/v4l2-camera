//#include "main_window.h"
//#include "camera_capture.h"
//#include <fcntl.h>
//#include <unistd.h>
//#include <sys/ioctl.h>
//#include <sys/mman.h>
//#include <linux/videodev2.h>
//#include <QVBoxLayout>
//#include <QHBoxLayout>
//#include <QDir>
//#include <QDebug>
//#include <QLabel>
//#include <QPushButton>
//#include <QComboBox>

//MainWindow::MainWindow(QWidget *parent)
//    : QMainWindow(parent), cameraCapture(nullptr)
//{
//    setupUI();
//    enumerateDevices();
//}

//MainWindow::~MainWindow()
//{
//    if (cameraCapture) {
//        cameraCapture->stop();
//        delete cameraCapture;
//    }
//}

//void MainWindow::setupUI()
//{
//    QWidget *centralWidget = new QWidget(this);
//    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

//    imageLabel = new QLabel(this);
//    imageLabel->setAlignment(Qt::AlignCenter);
//    imageLabel->setMinimumSize(640, 480);

//    QHBoxLayout *controlLayout = new QHBoxLayout();
//    deviceCombo = new QComboBox(this);
//    startButton = new QPushButton("Start", this);
//    stopButton = new QPushButton("Stop", this);
//    stopButton->setEnabled(false);

//    controlLayout->addWidget(deviceCombo);
//    controlLayout->addWidget(startButton);
//    controlLayout->addWidget(stopButton);

//    mainLayout->addWidget(imageLabel);
//    mainLayout->addLayout(controlLayout);

//    setCentralWidget(centralWidget);

//    connect(startButton, &QPushButton::clicked, this, &MainWindow::startCapture);
//    connect(stopButton, &QPushButton::clicked, this, &MainWindow::stopCapture);
//    connect(deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
//            this, &MainWindow::deviceChanged);
//}

//int is_video_capture_device(const char *device_path) {
//    int fd = open(device_path, O_RDWR);
//    if (fd == -1) {
//        perror("Failed to open device");
//        return 0;
//    }

//    struct v4l2_capability cap = {0};
//    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
//        perror("Failed to query capabilities");
//        close(fd);
//        return 0;
//    }

//    close(fd);

//    // 检查是否支持视频捕获
//    if (cap.capabilities & V4L2_CAP_VIDEO_OUTPUT) {
//        printf("%s 是一个视频捕获设备（如摄像头）\n", device_path);
//        return 1;
//    } else {
//        printf("%s 不是视频捕获设备\n", device_path);
//        return 0;
//    }
//}

//void MainWindow::enumerateDevices()
//{
//    QDir dir("/dev");
//    QStringList filters;
//    filters << "video*";
//    dir.setNameFilters(filters);

//    videoDevices = dir.entryList(QDir::System, QDir::Name);
//    deviceCombo->clear();

//    foreach (const QString &device, videoDevices) {
//        auto str = "/dev/" + device;
//        bool flag = is_video_capture_device(str.toLocal8Bit());
//        if(flag) {
//            deviceCombo->addItem(device, "/dev/" + device);
//        }
//        qDebug() << device << " : " << is_video_capture_device(str.toLocal8Bit());
//    }
//}

//void MainWindow::startCapture()
//{
//    if (cameraCapture) {
//        cameraCapture->stop();
//        delete cameraCapture;
//    }

//    QString device = deviceCombo->currentData().toString();
//    cameraCapture = new CameraCapture(device, this);
//    connect(cameraCapture, &CameraCapture::newFrame, this, &MainWindow::updateFrame);

//    if (cameraCapture->start()) {
//        startButton->setEnabled(false);
//        stopButton->setEnabled(true);
//    }
//}

//void MainWindow::stopCapture()
//{
//    if (cameraCapture) {
//        cameraCapture->stop();
//        startButton->setEnabled(true);
//        stopButton->setEnabled(false);
//    }
//}

//void MainWindow::updateFrame(const QImage &frame)
//{
//    imageLabel->setPixmap(QPixmap::fromImage(frame).scaled(
//        imageLabel->width(), imageLabel->height(), Qt::KeepAspectRatio));
//}

//void MainWindow::deviceChanged(int index)
//{
//    Q_UNUSED(index);
//    stopCapture();
//}
