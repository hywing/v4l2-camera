#ifndef IMAGECONVERTER_H
#define IMAGECONVERTER_H

#include <QRunnable>
#include <QImage>

class ImageConverter : public QRunnable {
public:
    ImageConverter(const quint8* nv12, QImage* rgbImage, int width, int height, int startY, int endY);

protected:
    void run() override;

private:
    const quint8* nv12;
    QImage* rgbImage;
    int width;
    int height;
    int startY;
    int endY;
};

#endif // IMAGECONVERTER_H
