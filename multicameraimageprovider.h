#ifndef MULTICAMERAIMAGEPROVIDER_H
#define MULTICAMERAIMAGEPROVIDER_H

// multicameraimageprovider.h
#include <QQuickImageProvider>
#include "multicamerahandler.h"

class MultiCameraImageProvider : public QQuickImageProvider
{
public:
    MultiCameraImageProvider(MultiCameraHandler *handler)
        : QQuickImageProvider(QQuickImageProvider::Image),
          m_handler(handler) {}

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    MultiCameraHandler *m_handler;
};

#endif // MULTICAMERAIMAGEPROVIDER_H
