#include "imageThread.h"
#include "shared.h"

// QT headers
#include <QApplication>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QTime>

/** \param _buttonPosition The postion of the button to update the image on.
 *  \param _thumbnailPath path of the thumbnail
 *  \param _encodedThumbnailPath encoded thumbnail path
 *  \param _fileName filename of the thumbnail
 *  \param _appIcon application icon
 *  \param m_username Kodi username
 *  \param m_password Kodi password
 *  \param m_ip Kodi ip
 * 	\param m_port Kodi port
 * 	\param _fileListType what MythUIButtonList instance to render the image on */

ImageThread::ImageThread(int _buttonPosition, QString _thumbnailPath, QString _fileName, QString _appIcon, QString _username, QString _password, QString _ip, QString _port,
                         MythUIButtonList *_fileListType)
    : buttonPosition(_buttonPosition), thumbnailPath(_thumbnailPath), fileName(_fileName), appIcon(_appIcon), username(_username), password(_password), ip(_ip), port(_port),
      fileListType(_fileListType) {}

/** \brief download and proccess the image if required. Emit the completed result */
void ImageThread::startRead() {
    this->fileName.replace("//", "/");

    nr = new NetRequest(username, password, ip, port, false);
    if (!QFileInfo::exists(this->fileName + ".processed")) {
        downloadAppIconImage();
        proccessImage();
    }
    delete nr;
    emit renderImage(buttonPosition, this->fileListType);
}

/** \brief Download the app icon image using net request if required */
void ImageThread::downloadAppIconImage() {
    if (this->fileName.compare(this->appIcon) == 0) { // if app icon, save it to overlay source on thumbnails
        if (!QFileInfo::exists(fileName)) {
            QFile file(fileName);
            file.open(QIODevice::WriteOnly);
            file.write(nr->downloadImage(urlEncode(this->thumbnailPath), false));
            file.close();
        }
    }
}

/** \brief convert the image from a thumbnail to a button */
void ImageThread::proccessImage() {
    QImage *originalImage = new QImage();

    if (QFileInfo::exists(this->fileName)) { // only for icons
        originalImage->load(this->fileName);
    } else { // all the thumbnails from Kodi
        originalImage->loadFromData(nr->downloadImage(this->thumbnailPath, true), "");
    }

    QColor originalImageColor(originalImage->pixel(2, 2)); // get image colour

    // get image ratio and scale
    int hRatio = originalImage->height() * ((16 / 9) * 1.75);
    originalImage->scaled(500, 500, Qt::KeepAspectRatio);
    int widthDiff = hRatio - originalImage->width();
    widthDiff = widthDiff / 2;

    QImage *destBackground = new QImage(hRatio, originalImage->height(), QImage::Format_ARGB32);
    destBackground->fill(originalImageColor);

    QPainter pI(&*destBackground);
    pI.setCompositionMode(QPainter::CompositionMode_SourceAtop);
    pI.drawImage(widthDiff, 0, *originalImage);
    pI.end();

    // refelction
    QImage reflectionImage = destBackground->mirrored();

    // crop
    QRect rectI(0, destBackground->height() / 1.25, destBackground->width(), destBackground->height());
    QImage reflectionCroppedImage = reflectionImage.copy(rectI);

    // black opacity
    QImage blackImage(reflectionCroppedImage.width(), reflectionCroppedImage.height(), QImage::Format_ARGB32);
    blackImage.fill(Qt::black);
    QPainter imagepainter(&reflectionCroppedImage);
    imagepainter.setOpacity(0.85);
    imagepainter.setCompositionMode(QPainter::CompositionMode_SourceOver); // Set the overlay effect
    imagepainter.drawImage(0, 0, blackImage);
    imagepainter.end();

    ////combine image and reflectionImage
    QImage result(destBackground->width(), destBackground->height() + destBackground->height() * .20,
                  QImage::Format_ARGB32); // image to hold the join of image 1 & 2
    QPainter painterII(&result);
    painterII.drawImage(0, 0, reflectionCroppedImage);
    painterII.drawImage(0, destBackground->height() * .20, *destBackground);
    painterII.end();

    // app icon
    QImage *appIconImage = new QImage(this->appIcon);
    *appIconImage = appIconImage->scaled(40, 40, Qt::IgnoreAspectRatio);

    // rounded corners
    QImage roundedCornersImage(result.width(), result.height(), QImage::Format_ARGB32);
    roundedCornersImage.fill(Qt::transparent);
    QBrush brush(result);
    QPen pen;
    pen.setColor(Qt::darkGray);
    pen.setJoinStyle(Qt::RoundJoin);
    QPainter painter(&roundedCornersImage);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::HighQualityAntialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    painter.setBrush(brush);
    painter.setPen(pen);
    painter.drawRoundedRect(0, 0, result.width(), result.height(), 30, 30);
    if (!appIcon.compare(this->fileName) == 0 and !this->appIcon.contains("ma_mv_browse_nocover.png")) {
        painter.drawImage(result.width() - 40, result.height() / 2, *appIconImage);
    }
    painter.end();

    roundedCornersImage.save(fileName + ".processed", "PNG");
    delete originalImage;
    delete destBackground;
    delete appIconImage;
}
