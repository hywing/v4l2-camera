// multicameraimageprovider.cpp
#include "multicameraimageprovider.h"

QImage MultiCameraImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    // ID格式应为 "cameraIndex" (如 "0", "1", "2", "3")
    bool ok;
    int cameraIndex = id.toInt(&ok);

    if (!ok || cameraIndex < 0 || cameraIndex >= m_handler->cameraCount()) {
        return QImage();
    }
//qDebug() << "dddddd time up!!!";
    auto &&image = m_handler->getCameraImage(cameraIndex);

    if (size) *size = image.size();

    if (requestedSize.width() > 0 && requestedSize.height() > 0) {
        return image.scaled(requestedSize, Qt::KeepAspectRatio);
    }

    return image;
}
