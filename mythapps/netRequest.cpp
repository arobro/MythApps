#include "netRequest.h"

// MythTV headers
#include <libmyth/mythcontext.h>

// MythApps headers
#include "shared.h"

/** \param m_username Kodi username
 *  \param m_password Kodi password
 *  \param m_ip Kodi ip
 * 	\param m_port Kodi port
 *  \param _longWait should the requestUrl function wait longer before timing out*/
NetRequest::NetRequest(QString m_username, QString m_password, QString m_ip, QString m_port, bool _longWait) {
    username = m_username;
    password = m_password;
    ip = m_ip;
    port = m_port;
    longWait = _longWait;

    NRmgr = new QNetworkAccessManager();
}
NetRequest::~NetRequest() {
    NRmgr->disconnect();
    delete NRmgr;
}

/** \brief helper function to send and receive any public urls without opional authentication
 *  \param url - external url to send the request to */
QString NetRequest::requestUrlPublic(QString url, QString authorization) {
    QUrl qurl(url);
    QNetworkRequest request;

    if (!authorization.isEmpty()) {
        request.setRawHeader("Authorization", authorization.toLocal8Bit().toBase64());
    }

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setUrl(qurl);

    QNetworkReply *RUreply = NRmgr->get(request);
    return requestUrlHelper(RUreply, qurl);
}

/** \brief helper function to send and receive json to Kodi
 * 	\param QJsonObject json object*/
QString NetRequest::requestUrl(QJsonObject value) {
    QUrl qurl("http://" + ip + ":" + port + "/jsonrpc");
    qurl.setUserName(username);
    qurl.setPassword(password);

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setUrl(qurl);

    QJsonDocument doc(value);
    QByteArray data = doc.toJson();
    QNetworkReply *RUreply = NRmgr->post(request, data);
    return requestUrlHelper(RUreply, qurl);
}

/** \brief helper function to send and receive json.
 * 	\param RUreply json QNetworkReplyt
 *  \param qurl url to send the request to */
QString NetRequest::requestUrlHelper(QNetworkReply *RUreply, QUrl qurl) {
    contentsGlobal = "";
    QNetworkReply *RUreply2;

    QObject::connect(RUreply, &QNetworkReply::finished, [=]() {
        if (RUreply->error() == QNetworkReply::NoError) {
            contentsGlobal = QString::fromUtf8(RUreply->readAll());
            if (contentsGlobal.contains("error")) {
                LOG(VB_GENERAL, LOG_ERR, "NetRequest->requestUrlHelper() Kodi Error: " + contentsGlobal + " -" + qurl.toString());
            }
        } else { // err
            contentsGlobal = RUreply->errorString();
            LOG(VB_GENERAL, LOG_DEBUG, "NetRequest->requestUrlHelper() Error: " + contentsGlobal + " -" + qurl.toString());
        }
        RUreply->disconnect();
        RUreply->deleteLater();
    });

    QNetworkRequest l_req(qurl); // wait if the requests has not yet returned.
    RUreply2 = NRmgr->get(l_req);
    QEventLoop l_event_loop;
    QObject::connect(RUreply2, SIGNAL(finished()), &l_event_loop, SLOT(quit()));
    l_event_loop.exec();

    RUreply2->disconnect();
    delete RUreply2;

    int wait = 4;
    if (longWait) { // longer timeout
        wait = 60;
    }

    for (int i = 0; i < wait; i++) { // addional check to confirm the requests has returned
        if (contentsGlobal.compare("") == 0) {
            delayMilli(100);
        }
    }

    return contentsGlobal;
}

/** \brief download an image.
 * 	\param imageUrl image url to download
 *  \param tryDirectDownload Download directly (faster) instead of via Kodi
 *  \return binary image object */
QByteArray NetRequest::downloadImage(QString imageUrl, bool tryDirectDownload) {
    QUrl l_url;

    if (tryDirectDownload && imageUrl.contains("http")) {
        l_url = urlDecode(removeTrailingChar(imageUrl, '/').replace("image://", ""));
    } else {
        l_url = "http://" + ip + ":" + port + "/image/" + imageUrl;
        l_url.setUserName(username);
        l_url.setPassword(password);
    }

    QNetworkRequest l_req(l_url);
    l_req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
    QNetworkReply *reply2 = NRmgr->get(l_req);

    QEventLoop l_event_loop; // wait if the requests has not yet returned.
    QObject::connect(reply2, SIGNAL(finished()), &l_event_loop, SLOT(quit()));
    l_event_loop.exec();
    QByteArray imageData = reply2->readAll();

    reply2->disconnect();
    delete reply2;

    // download image via Kodi if failed to download directly.
    if (tryDirectDownload && imageData.size() < 300) {
        return downloadImage(urlEncode(imageUrl), false);
    } else {
        return imageData;
    }
}

/** \brief download favourite icon for a website.
 * 	\param websiteUrl to autodiscover the favourite icon for
 *  \return url of the highest quailty website icon */
QString NetRequest::getFavIconUrl(QString websiteUrl) {
    QUrl l_url(websiteUrl);
    QNetworkRequest l_req(l_url);
    l_req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);

    QNetworkReply *reply2 = NRmgr->get(l_req);
    QEventLoop loop; // wait if the requests has not yet returned.
    QObject::connect(reply2, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
    QString html = reply2->readAll();

    reply2->disconnect();
    delete reply2;

    QStringList lines = html.split('>');

    QString biggestIcon = "";
    foreach (QString line,
             lines) { // parse the html and find the high quality icon
        if (line.contains("<link rel=") and line.contains("icon") and line.contains("href=") and line.contains("png")) {
            QString desiredUrl = line.replace(line.mid(0, line.indexOf("href=\"")) + "href=\"", "");
            desiredUrl = desiredUrl.mid(0, desiredUrl.indexOf("\""));
            biggestIcon = desiredUrl.replace("\"", "");
        }
    }

    websiteUrl = removeTrailingChar(websiteUrl, '/');
    biggestIcon = removeTrailingChar(biggestIcon, '/');

    if (biggestIcon.compare("") == 0) { // if no url found
        return QString("");
    }

    QString domain = getWebSiteDomain(websiteUrl);
    LOG(VB_GENERAL, LOG_DEBUG, "getFavIconUrl() domain : " + domain + " biggestIcon: " + biggestIcon);

    if (biggestIcon.contains(domain)) { // fix up relative url if required
        return biggestIcon;
    } else {
        return websiteUrl + biggestIcon;
    }
}

/** \brief switch between Kodi and MythTV on Android
 * 	\param app name of app to switch to
 *  \return is the helper switching app (mythapp services) running? */
bool NetRequest::androidAppSwitch(QString app) {
    LOG(VB_GENERAL, LOG_INFO, "androidAppSwitch() " + app);

    if (!m_webSocketNR.isValid()) {
        m_webSocketNR.open(QUrl("ws://127.0.0.1:8088"));

        QObject::connect(&m_webSocketNR, &QWebSocket::connected, [=]() {
            QObject::connect(&m_webSocketNR, &QWebSocket::textMessageReceived, [=]() {
                LOG(VB_GENERAL, LOG_INFO, "NR->androidAppSwitch() mythAppsServiceconnected ");
                mythAppsServiceconnected = true;
            });
        });
    }

    for (int i = 0; i < 5; i++) {
        delayMilli(500);
        if (mythAppsServiceconnected) {
            break;
        }
    }

    if (!mythAppsServiceconnected) {
        LOG(VB_GENERAL, LOG_INFO, "NR->androidAppSwitch() MythApp Services apk not installed or running.");
        return false;
    }
    m_webSocketNR.sendTextMessage(app);
    return true;
}
