#ifndef MYTHREAD_H
#define MYTHREAD_H

// QT headers
#include <QFile>
#include <QObject>
#include <QString>
#include <QThread>

// MythTV headers
#include <libmythui/mythuibuttonlist.h>

// MythApps headers
#include "netRequest.h"

/** \class ImageThread
 *  \brief Downloads and processes an image thumbnail into an image button. This can be multithreaded*/
class ImageThread : public QObject {
    Q_OBJECT

  public:
    explicit ImageThread(int buttonPosition, QString _thumbnailPath, QString _fileName, QString _appIcon, QString _username, QString _password, QString _ip, QString _port,
                         MythUIButtonList *_fileListType);

  signals:
    void renderImage(int, MythUIButtonList *);

  public slots:
    void startRead();

  private:
    int buttonPosition;
    QString thumbnailPath;
    QString fileName;
    QString appIcon;
    QString username;
    QString password;
    QString ip;
    QString port;
    MythUIButtonList *fileListType;

    void downloadAppIconImage(NetRequest &nr);
    void proccessImage(NetRequest &nr);
};

#endif // MYTHREAD_H
