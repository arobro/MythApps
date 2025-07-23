#ifndef IMAGETHREAD_H
#define IMAGETHREAD_H

// QT headers
#include <QAtomicInt>
#include <QObject>
#include <QRunnable>
#include <QString>

// MythTV headers
#include <libmythui/mythuibuttonlist.h>

class NetRequest;
class MythUIButtonList;

struct Params {
    int position;
    QString thumbnailUrl;
    QString localImagePath;
    QString appIcon;
    QString username;
    QString password;
    QString ip;
    QString port;
    MythUIButtonList *fileList;
};

class ImageThread : public QObject, public QRunnable {
    Q_OBJECT

  public:
    explicit ImageThread(const Params &p);
    void run() override;
    void requestAbort();

  signals:
    void finished(int position, const QString thumbnailUrl, MythUIButtonList *fileList);

  private:
    Params params;
    QAtomicInt abortRequested{0}; // 0 = running, 1 = aborted

    void downloadAppIcon(NetRequest &nr, const QString &fn);
    void processImage(NetRequest &nr, const QString &fn);
    bool isAppIconEnabled(const QString &fn);
};

#endif // IMAGETHREAD_H
