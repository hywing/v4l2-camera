#include "image_converter.h"

ImageConverter::ImageConverter(const quint8* nv12, QImage* rgbImage, int width, int height, int startY, int endY)
    : nv12(nv12), rgbImage(rgbImage), width(width), height(height), startY(startY), endY(endY)
{

}

void ImageConverter::run()
{
    const quint8* yPlane = nv12;
    const quint8* uvPlane = nv12 + width * height;

    for (int y = startY; y < endY; y++) {
        for (int x = 0; x < width; x++) {
            int yIdx = y * width + x;
            int uvIdx = ((y / 2) * (width / 2) + (x / 2)) * 2;

            int Y = yPlane[yIdx];
            int U = uvPlane[uvIdx] - 128;
            int V = uvPlane[uvIdx + 1] - 128;

            int R = qBound(0, (298 * Y + 409 * V + 128) >> 8, 255);
            int G = qBound(0, (298 * Y - 100 * U - 208 * V + 128) >> 8, 255);
            int B = qBound(0, (298 * Y + 516 * U + 128) >> 8, 255);

            rgbImage->setPixel(x, y, qRgb(R, G, B));
        }
    }
}
