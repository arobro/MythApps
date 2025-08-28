#include "imageThread.h"

// QT headers
#include <QBrush>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QMetaObject>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QUrl>

// MythTV headers
#include <libmyth/mythcontext.h>

// MythApps headers
#include "netRequest.h"

static QString urlEncode(const QString &s) { return QUrl::toPercentEncoding(s); }

ImageThread::ImageThread(const Params &p) : params(p) { setAutoDelete(false); }

/** \brief download and proccess the image if required. Emit the completed result */
void ImageThread::run() {
    if (params.appIcon.isEmpty())
        LOG(VB_GENERAL, LOG_DEBUG, "ImageThread::run() thumbnailUrl: " + params.thumbnailUrl);

    QString fn = params.localImagePath;
    fn.replace("//", "/");

    NetRequest nr(params.username, params.password, params.ip, params.port, false);

    if (!QFileInfo::exists(fn + ".processed")) {
        downloadAppIcon(nr, fn);
        processImage(nr, fn);
    }

    if (!abortRequested.load())
        emit finished(params.position, params.thumbnailUrl, params.fileList);

    deleteLater();
}

void ImageThread::requestAbort() { abortRequested.store(1); }

/** \brief Download the app icon image using net request if required */
void ImageThread::downloadAppIcon(NetRequest &nr, const QString &fn) {
    if (fn == params.appIcon && !QFileInfo::exists(fn)) {
        QFile out(fn);
        if (out.open(QIODevice::WriteOnly)) {
            out.write(nr.downloadImage(urlEncode(params.thumbnailUrl)));
        }
        out.close();
    }
}

/** \brief checks whether the application icon should be considered "enabled" or valid */
bool ImageThread::isAppIconEnabled(const QString &fn) { return !params.appIcon.isEmpty() && params.appIcon != fn && !params.appIcon.contains("ma_mv_browse_nocover.png"); }

/** \brief convert the image from a thumbnail to a button */
void ImageThread::processImage(NetRequest &nr, const QString &fn) {
    QImage orig;
    if (QFileInfo::exists(fn)) {
        orig.load(fn);
    } else {
        orig.loadFromData(nr.downloadImage(params.thumbnailUrl));
    }
    if (orig.isNull())
        return;

    double ratio = (16.0 / 9.0);
    int mainH = orig.height();
    int mainW = int(mainH * ratio);
    int reflH = int(mainH * 0.20);
    int finalH = mainH + reflH;
    int xOff = (mainW - orig.width()) / 2;

    // Create padded image with background color and draw original image centered
    QImage padded(mainW, mainH, QImage::Format_ARGB32_Premultiplied);
    QColor bg(orig.pixel(2, 2));
    padded.fill(bg);
    QPainter pd(&padded);
    pd.setRenderHint(QPainter::SmoothPixmapTransform);
    pd.drawImage(xOff, 0, orig);
    pd.end();
    if (abortRequested.load())
        return;

    // Create reflection image from top slice of padded image and apply opacity mask
    QRect slice(0, 0, mainW, reflH);
    QImage reflection = padded.copy(slice).mirrored(false, true);
    QImage mask(reflection.size(), QImage::Format_ARGB32_Premultiplied);
    mask.fill(Qt::black);
    QPainter pr(&reflection);
    pr.setCompositionMode(QPainter::CompositionMode_SourceAtop);
    pr.setOpacity(0.85);
    pr.drawImage(0, 0, mask);
    pr.end();
    if (abortRequested.load())
        return;

    // Compose final image with rounded corners and clipped path
    QImage result(mainW, finalH, QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);
    QPainter p(&result);
    p.setRenderHints(QPainter::Antialiasing | QPainter::HighQualityAntialiasing | QPainter::SmoothPixmapTransform);
    QPainterPath path;
    path.addRoundedRect(result.rect(), 30, 30);
    p.setClipPath(path);

    p.drawImage(0, 0, reflection);
    p.drawImage(0, reflH, padded);

    // Optionally overlay app icon
    if (isAppIconEnabled(fn)) {
        QImage icon(params.appIcon);
        if (!icon.isNull()) {
            icon = icon.scaled(40, 40, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            QPoint pos(mainW - icon.width(), (finalH - icon.height()) / 2);
            p.drawImage(pos, icon);
        }
    }

    // Persist
    p.end();
    result.save(fn + ".processed", "PNG");
}
