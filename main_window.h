#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>

//QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QComboBox;
//QT_END_NAMESPACE

class CameraCapture;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void startCapture();
    void stopCapture();
    void updateFrame(const QImage &frame);
    void deviceChanged(int index);

private:
    void enumerateDevices();
    void setupUI();

    CameraCapture *cameraCapture;
    QLabel *imageLabel;
    QPushButton *startButton;
    QPushButton *stopButton;
    QComboBox *deviceCombo;
    QList<QString> videoDevices;
};
#endif // MAINWINDOW_H
