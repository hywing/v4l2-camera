#include "main_window.h"
#include "camera_capture.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDir>
#include <QDebug>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), cameraCapture(nullptr)
{
    setupUI();
    enumerateDevices();
}

MainWindow::~MainWindow()
{
    if (cameraCapture) {
        cameraCapture->stop();
        delete cameraCapture;
    }
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMinimumSize(640, 480);

    QHBoxLayout *controlLayout = new QHBoxLayout();
    deviceCombo = new QComboBox(this);
    startButton = new QPushButton("Start", this);
    stopButton = new QPushButton("Stop", this);
    stopButton->setEnabled(false);

    controlLayout->addWidget(deviceCombo);
    controlLayout->addWidget(startButton);
    controlLayout->addWidget(stopButton);

    mainLayout->addWidget(imageLabel);
    mainLayout->addLayout(controlLayout);

    setCentralWidget(centralWidget);

    connect(startButton, &QPushButton::clicked, this, &MainWindow::startCapture);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::stopCapture);
    connect(deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::deviceChanged);
}

void MainWindow::enumerateDevices()
{
    QDir dir("/dev");
    QStringList filters;
    filters << "video*";
    dir.setNameFilters(filters);

    videoDevices = dir.entryList(QDir::System, QDir::Name);
    deviceCombo->clear();

    foreach (const QString &device, videoDevices) {
        deviceCombo->addItem(device, "/dev/" + device);
    }
}

void MainWindow::startCapture()
{
    if (cameraCapture) {
        cameraCapture->stop();
        delete cameraCapture;
    }

    QString device = deviceCombo->currentData().toString();
    cameraCapture = new CameraCapture(device, this);
    connect(cameraCapture, &CameraCapture::newFrame, this, &MainWindow::updateFrame);

    if (cameraCapture->start()) {
        startButton->setEnabled(false);
        stopButton->setEnabled(true);
    }
}

void MainWindow::stopCapture()
{
    if (cameraCapture) {
        cameraCapture->stop();
        startButton->setEnabled(true);
        stopButton->setEnabled(false);
    }
}

void MainWindow::updateFrame(const QImage &frame)
{
    imageLabel->setPixmap(QPixmap::fromImage(frame).scaled(
        imageLabel->width(), imageLabel->height(), Qt::KeepAspectRatio));
}

void MainWindow::deviceChanged(int index)
{
    Q_UNUSED(index);
    stopCapture();
}
