#ifndef CAMERA_VIEW_H
#define CAMERA_VIEW_H

#include <QLabel>
#include <QImage>
#include <QTimer>
#include <QWidget>
#include <QElapsedTimer>
#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QMessageBox>
#include <QStackedLayout>

#include "image_converter.h"
#include "v4l2_camera.h"


// 摄像头显示视图
class CameraView : public QLabel
{
    Q_OBJECT
public:
    explicit CameraView(int index, QWidget *parent = nullptr);

    void setInfoText(const QString &text);

    void setFullscreen(bool fullscreen);

    bool isFullScreen() const { return isFullscreen; }

protected:
    void mousePressEvent(QMouseEvent *event) override;

    void paintEvent(QPaintEvent *event) override;

    bool eventFilter(QObject *obj, QEvent *event) override;

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

#endif // CAMERA_VIEW_H
