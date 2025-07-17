#ifndef NetRequest_h
#define NetRequest_h

// QT headers
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QNetworkAccessManager>
#include <QString>
#include <QStringList>
#include <QtWebSockets/QWebSocket>

/** \class NetRequest
 *  \brief sent/receive json requests and images */
class NetRequest {
  public:
    NetRequest(QString m_username, QString m_password, QString m_ip, QString m_port, bool _longWait);
    ~NetRequest();

    QString requestUrlPublic(QString url, QString authorization);

    QByteArray downloadImage(QString imageUrl, bool tryDirectDownload);

    QString getFavIconUrl(QString websiteUrl);

  private:
    QString username;
    QString password;
    QString ip;
    QString port;
    QString globalDuration = "";

    QNetworkAccessManager *NRmgr;
};
#endif
