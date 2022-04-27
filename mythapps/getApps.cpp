#include "getApps.h"

// QT headers
#include <QCoreApplication>
#include <QEventLoop>
#include <QString>
#include <QXmlStreamReader>
#include <QtNetwork/QNetworkReply>

#include "shared.h"
#include <mythcontext.h>
#include <regex>

/** \param m_username Kodi username
 *  \param m_password Kodi password
 *  \param m_ip Kodi ip
 * 	\param m_port Kodi port */
GetApps::GetApps(QString m_username, QString m_password, QString m_ip, QString m_port) {
    managerApps = new QNetworkAccessManager();
    QObject::connect(managerApps, &QNetworkAccessManager::finished, this, &GetApps::ReplyFinishedApps);

    managerAppsDetails = new QNetworkAccessManager();
    QObject::connect(managerAppsDetails, &QNetworkAccessManager::finished, this, &GetApps::ReplyFinishedAppsDetails);

    username = m_username;
    password = m_password;
    ip = m_ip;
    port = m_port;
}

GetApps::~GetApps() {
    managerApps->disconnect();
    delete managerApps;
    managerAppsDetails->disconnect();
    delete managerAppsDetails;
}
/** \brief load all apps from Kodi*/
void GetApps::loadAll() {
    QUrl qurl("http://" + ip + ":" + port +
              "/jsonrpc?request={%22jsonrpc%22:%222.0%22,%22id%22:1,%22method%22:"
              "%22Addons.GetAddons%22,%22params%22:{%22type%22:%22xbmc.addon."
              "video%22,%22enabled%22:true}}");
    qurl.setUserName(username);
    qurl.setPassword(password);

    request.setUrl(qurl);
    managerApps->get(request);
}

QString GetApps::getLocationFromUrlAddress(QString urlAddress) { return urlToThumbnailMap.value(getWebSiteDomain(urlAddress)); }

/** \brief  LoadApps() calls this function for each Kodi app. This function then
requests the apps addon details via a
 *          json request and the repsonse will be in
ReplyFinishedAppsDetails(QNetworkReply *reply)
 *  \param reply This is the name of the Kodi app
Net Request is not used as it was found to be slower due to the gui elments only
rendering after all requests were completed*/
void GetApps::ReplyFinishedApps(QNetworkReply *reply) {
    QString answer = reply->readAll();

    QByteArray br = answer.toUtf8();
    QJsonDocument doc = QJsonDocument::fromJson(br);
    QJsonObject addons = doc.object();
    QJsonValue addons2 = addons["result"];
    QJsonObject result = addons2.toObject();

    for (auto oIt = result.constBegin(); oIt != result.constEnd(); ++oIt) {
        QJsonArray agentsArray = oIt.value().toArray();

        foreach (const QJsonValue &v, agentsArray) {
            QString addonid = v.toObject().value("addonid").toString();
            appIDlist.append(addonid);

            QUrl qurl("http://" + ip + ":" + port +
                      "/jsonrpc?request={%22jsonrpc%22:%222.0%22,%22id%22:1,%"
                      "22method%22:%"
                      "22Addons.GetAddonDetails%22,%22params%22:{%22addonid%22:%22" +
                      addonid +
                      "%22,%22properties%22:[%22description%22,%22thumbnail%22,%20%"
                      "22name%22]}}");
            qurl.setUserName(username);
            qurl.setPassword(password);

            request.setUrl(qurl);
            managerAppsDetails->get(request);
        }
    }
    displayWebsiteBookmarks();
}

/** \brief  ReplyFinishedApps() calls this function. This is the json response
 * with the details of the app. \param reply contains the App name, App
 * thumbnail App Description etc  */
void GetApps::ReplyFinishedAppsDetails(QNetworkReply *reply) {
    QString answer = reply->readAll();
    LOG(VB_GENERAL, LOG_DEBUG, "ReplyFinishedAppsDetails()" + answer);

    QByteArray br = answer.toUtf8();
    QJsonDocument doc = QJsonDocument::fromJson(br);
    QJsonObject addons = doc.object();
    QJsonValue addons2 = addons["result"];
    QJsonObject o = addons2.toObject();

    QString name = "";
    QString thumbnail = "";
    QString addonid = "";
    QString description = "";

    for (auto oIt = o.constBegin(); oIt != o.constEnd(); ++oIt) {
        QJsonObject o2 = oIt.value().toObject();
        for (auto oIt2 = o2.constBegin(); oIt2 != o2.constEnd(); ++oIt2) {
            if (oIt2.key().compare("addonid") == 0) {
                addonid = oIt2.value().toString();
            }
            if (oIt2.key().compare("name") == 0) {
                name = oIt2.value().toString();
            }
            if (oIt2.key().compare("thumbnail") == 0) {
                thumbnail = oIt2.value().toString();
            }
            if (oIt2.key().compare("description") == 0) {
                description = oIt2.value().toString();
            }
        }
        // creates a map to lookup the thumbnail based on the addonid.
        urlToThumbnailMap.insert(getWebSiteDomain(addonid), thumbnail);

        // load program
        emit loadProgramSignal(name, createProgramData(addonid, description, thumbnail, false, ""), thumbnail, true);
    }
}

/** \brief get the app id (name) list.
 *  \return app id list */
QStringList GetApps::getAppIDlist() { return appIDlist; }

/** \brief retrieve and display the website bookmarks on the home screen*/
void GetApps::displayWebsiteBookmarks() {
    LOG(VB_GENERAL, LOG_DEBUG, "displayWebsiteBookmarks() start");
    QString kodiWeb = gCoreContext->GetSettingOnHost("MythAppsweb", gCoreContext->GetMasterHostName());

    if (kodiWeb.contains("~")) {
        QStringList kodiWebList = kodiWeb.split("~");

        foreach (QString website, kodiWebList) { displayBookmark(website); }
    }
    LOG(VB_GENERAL, LOG_DEBUG, "displayWebsiteBookmarks() end");
}

/** \brief helper function to display an individual bookmark on the home
 * screen*/
void GetApps::displayBookmark(QString kodiWeb) {
    if (kodiWeb.contains("|")) {
        QString thumbnail = kodiWeb.split("|").at(1);
        if (!thumbnail.isEmpty()) {
            thumbnail = "image://" + thumbnail;
        }

        QString kodiWebLabel = kodiWeb.split("|").at(0);
        QString kodiWebName = kodiWebLabel.replace("http://", "").replace("https://", "").replace("www.", "").replace("~", "");

        emit loadProgramSignal(kodiWebName, createProgramData(kodiWebLabel, "web", thumbnail, false, ""), thumbnail, true);
    }
}
