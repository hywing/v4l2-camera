#include "camera_view.h"

CameraView::CameraView(int index, QWidget *parent)
    : QLabel(parent), cameraIndex(index), isFullscreen(false)
{
    setAlignment(Qt::AlignCenter);
    setStyleSheet("background-color: black; color: white;");
    QFont font;
    font.setPointSize(12);
    setFont(font);
    setMinimumSize(320, 180);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void CameraView::setInfoText(const QString &text)
{
    infoText = text;
    update();
}

void CameraView::setFullscreen(bool fullscreen)
{
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

void CameraView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(cameraIndex);
    }
    QLabel::mousePressEvent(event);
}

void CameraView::paintEvent(QPaintEvent *event)
{
    QLabel::paintEvent(event);

    if (!pixmap() && !infoText.isEmpty()) {
        QPainter painter(this);
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, infoText);
    }
}

bool CameraView::eventFilter(QObject *obj, QEvent *event)
{
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
