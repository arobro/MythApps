#ifndef GetApps_h
#define GetApps_h

// QT headers
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QString>
#include <QStringList>

/** \class GetApps
 *  \brief Loads the apps onto the home screen and displays any bookmarks.  */
class GetApps : public QObject {
    Q_OBJECT

  public:
    GetApps(QString m_username, QString m_password, QString m_ip, QString m_port);
    ~GetApps();

    void loadAll();
    QString getLocationFromUrlAddress(QString urlAddress);
    QStringList getAppIDlist();

  private:
    QString username;
    QString password;
    QString ip;
    QString port;

    QStringList appIDlist;

    QNetworkAccessManager *managerApps;
    QNetworkAccessManager *managerAppsDetails;

    QNetworkRequest request;
    QMap<QString, QString> urlToThumbnailMap;

    void ReplyFinishedApps(QNetworkReply *reply);
    void ReplyFinishedAppsDetails(QNetworkReply *reply);
    void displayWebsiteBookmarks();
    void displayBookmark(QString kodiWeb);

  signals:
    void loadProgramSignal(QString name, QString setdata, QString thumbnailPath, bool appDir);
};
#endif
