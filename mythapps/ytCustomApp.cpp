// MythTV headers
#include <libmyth/mythcontext.h>

// QT headers
#include <QDateTime>

// MythApps headers
#include "programData.h"
#include "ytCustomApp.h"

/** \brief Creates a new ytCustomApp to provide native performance.
/** \param m_username Kodi username
 *  \param m_password Kodi password
 *  \param m_ip Kodi ip
 * 	\param m_port Kodi port
 *  \param m_searchSettingsButtonList Setting UI Widget
 *  \param m_searchSettingsGroup Setting Group UI Widget
 *  \param m_browser Browser Widget*/
ytCustomApp::ytCustomApp(QString m_username, QString m_password, QString m_ip, QString m_port, MythUIButtonList *m_searchSettingsButtonList, MythUIType *m_searchSettingsGroup, Browser *m_browser) {
    netRequest = new NetRequest(m_username, m_password, m_ip, m_port, false);
    searchSettingsGroup = m_searchSettingsGroup;
    browser = m_browser;

    auto *item1 = new MythUIButtonListItem(m_searchSettingsButtonList, "Sort By:");
    sortBy = "relevance";
    item1->SetText("Relevance", "buttontext2");
    item1->SetData(QStringList({"Relevance", "Date"}));

    auto *item2 = new MythUIButtonListItem(m_searchSettingsButtonList, "Duration:");
    videodDuration = "any";
    item2->SetText("Any", "buttontext2");
    item2->SetData(QStringList({"Any", "Long"}));

    auto *item3 = new MythUIButtonListItem(m_searchSettingsButtonList, "Media Type:");
    type = "";
    eventType = "";
    item3->SetText("Any", "buttontext2");
    item3->SetData(QStringList({"Any", "Video", "Channel", "Live", "Live (Upcoming)", "Playlist"}));

    useBrowserPlayer = true;

    masterDateListdateBetween = QStringList({"Now", "1 Month Ago", "6 Months Ago", "1 Year Ago", "2 Years  Ago", "5 Years  Ago", "10 Years  Ago", "15 Years Ago"});
    masterDateListPublishedBefore = QStringList({"1 Month Ago", "6 Months Ago", "1 Year Ago", "2 Years  Ago", "5 Years  Ago", "10 Years  Ago", "15 Years Ago", "All Time"});

    dateBetween = new MythUIButtonListItem(m_searchSettingsButtonList, "Published Between:");
    dateBetween->SetText("Now", "buttontext2");
    dateBetween->SetData(masterDateListdateBetween);

    datePublishedBefore = new MythUIButtonListItem(m_searchSettingsButtonList, "And Published Before:");
    datePublishedBefore->SetText("All Time", "buttontext2");
    datePublishedBefore->SetData(masterDateListPublishedBefore);

    dateBetweenUrlBuilder = "";
    datePublishedBeforeUrlBuilder = "";
}

/** \brief set the visibilty of tall the widgets in the settings group
 *  \param visible should the search setting widgets be visible? */
void ytCustomApp::setSearchSettingsGroupVisibilty(bool visible) {
    searchSettingsGroup->SetVisible(visible);
    searchSettingsGroup->SetEnabled(visible);
}

/** \brief get json list of videoes
 *  \param searchText what to search for
 *  \param directory what directory path to load */
QList<QVariant> ytCustomApp::getVideos(QString searchText, QString directory) {
    LOG(VB_GENERAL, LOG_DEBUG, "getVideos()- " + searchText + "- " + directory);

    QString query = getAPIBaseUrl() + "videos?part=snippet&maxResults=50&chart=mostPopular"; //&regionCode=US

    ytApi = gCoreContext->GetSetting("MythAppsYTapi");
    ytID = gCoreContext->GetSetting("MythAppsYTID");

    if (!searchText.isEmpty()) {
        query = getAPIBaseUrl() + "search?part=snippet&maxResults=50&type=" + type + eventType + datePublishedBeforeUrlBuilder + dateBetweenUrlBuilder + "&order=" + sortBy +
                "&videoDuration=" + videodDuration + "&q=" + searchText;
    }

    if (!directory.isEmpty()) {
        if (directory.startsWith("YTNative://channel/")) {
            directory.replace("YTNative://channel/", "");
            query = getAPIBaseUrl() + "search?part=snippet&maxResults=50&channelId=" + directory;
        } else if (directory.startsWith("YTNative://playlist/")) {
            directory.replace("YTNative://playlist/", "");
            query = getAPIBaseUrl() + "playlistItems?part=snippet&maxResults=50&playlistId=" + directory;
        }
    }

    QString answer = netRequest->requestUrlPublic(query + "&key=" + ytApi, ytID);

    if (answer.contains("\"Forbidden\"")) {
        LOG(VB_GENERAL, LOG_ERR, "excedded quota");
    }

    QJsonDocument jsonDocument = QJsonDocument::fromJson(answer.toLocal8Bit().data());

    QJsonObject jObject = jsonDocument.object();
    QVariantMap mainMap = jObject.toVariantMap();

    return mainMap["items"].toList();
}

/** \brief load videoes
 *  \param searchText what to search for
 *  \param directory what directory path to load */
void ytCustomApp::loadProgramList(QString searchText, QString directory) {
    LOG(VB_GENERAL, LOG_DEBUG, "getLoadProgramList()");

    QList<QStringList> programList;
    QList<QVariant> list = getVideos(searchText, directory);

    int count = 0;
    foreach (QVariant T, list) {
        QVariantMap map4 = T.toMap();

        QString url;
        QString desc;
        QString imgurl;
        QString prefix;
        QString title;
        bool autoPlay = false;

        if (map4["kind"].toString().compare("youtube#video") == 0) {
            url = map4["id"].toString();
            autoPlay = true;
            prefix = getKodiYTPlayUrl();

        } else if (map4["kind"].toString().compare("youtube#searchResult") == 0) {
            QVariantMap map5 = map4["id"].toMap();
            QString kind = map5["kind"].toString();

            if (kind.contains("#video")) {
                url = map5["videoId"].toString();
                autoPlay = true;
                prefix = getKodiYTPlayUrl();
            } else if (kind.contains("#channel")) {
                url = map5["channelId"].toString();
                prefix = "YTNative://channel/";
            } else if (kind.contains("#playlist")) {
                url = map5["playlistId"].toString();
                prefix = "YTNative://playlist/";
            }

        } else if (map4["kind"].toString().contains("#playlistItem")) {
            QVariantMap map6 = map4["snippet"].toMap();
            QVariantMap map7b = map6["resourceId"].toMap();
            if (map7b["kind"].toString().contains("#video")) {
                url = map7b["videoId"].toString();
                autoPlay = true;
                prefix = getKodiYTPlayUrl();
            }
        }

        QVariantMap map6 = map4["snippet"].toMap();

        title = map6["title"].toString();
        desc = map6["description"].toString();

        QVariantMap map7 = map6["thumbnails"].toMap();
        QVariantMap map8 = map7["medium"].toMap();
        imgurl = map8["url"].toString();

        QStringList programListTemp;
        programListTemp.append(title);
        programListTemp.append(createProgramData(prefix + url, desc, imgurl, autoPlay, ""));
        programListTemp.append(imgurl);

        programList.append(programListTemp);

        count++;
        if (count % 4 == 0) {
            QCoreApplication::processEvents();
        }
    }

    foreach (QList T, programList) {
        emit loadProgramSignal(T.at(0), T.at(1), T.at(2));
    }
}

QString ytCustomApp::getKodiYTPluginAppName() { return "YouTube"; }

QString ytCustomApp::getKodiYTPlayUrl() { return "plugin://" + getKodiYTPluginDomain() + "/?action=play_video&videoid="; }

QString ytCustomApp::getKodiYTPluginDomain() { return kodiYTPluginDomain; }

QString ytCustomApp::converKodiYTURLtoBrowserUrl(QString url) { return "https://www." + getKodiYTPluginAppName() + ".com/watch?v=" + url.replace(getKodiYTPlayUrl(), ""); }

QString ytCustomApp::getAPIBaseUrl() { return "https://www.googleapis.com/youtube/v3/"; }

/** \brief open the video in the browser if prefered option
 * \param filePathParam url of the video*/
bool ytCustomApp::openInBrowserIfRequired(QString filePathParam) {
    // todo: put a setting in
    return false;

    if (filePathParam.contains(getKodiYTPlayUrl())) { // openInBrowserIfPossible()
        browser->openBrowser(converKodiYTURLtoBrowserUrl(filePathParam));
        return true;
    }
    return false;
}

/** \brief set the program data
 * \param pd program data string*/
void ytCustomApp::setKodiYTProgramData(QString pd) {
    ProgramData *ytPD = new ProgramData("", pd);
    kodiYTPluginDomain = ytPD->getFilePathParam();
    kodiYTPluginIcon = ytPD->getImageUrl();
    delete ytPD;

    kodiYTProgramData = pd;
}

/** \brief get the YTNative program data used to open the custom app*/
QString ytCustomApp::getYTnativeProgramData() {
    QString result = kodiYTProgramData;
    return result.replace(getKodiYTPluginDomain(), "YTNative");
}

/** \brief get the kodi YT program data used to open the wrapped app*/
QString ytCustomApp::getKodiYTProgramData() { return kodiYTProgramData; }

/** \brief get the kodi YT app icon*/
QString ytCustomApp::getAppIcon() { return kodiYTPluginIcon; }

/** \brief search settings. Used for sorting and date range search */
void ytCustomApp::searchSettingsClicked(MythUIButtonListItem *item) {
    LOG(VB_GENERAL, LOG_DEBUG, "searchSettingsClicked()");
    QStringList options = item->GetData().toStringList();

    int pos = options.indexOf(item->GetText("buttontext2"));
    pos++;
    if (pos == options.size()) {
        pos = 0;
    }
    QString itemNewText = options.at(pos);
    item->SetText(itemNewText, "buttontext2");

    QString itemText = item->GetText();

    if (itemText.compare("Sort By:") == 0) {
        sortBy = itemNewText.toLower();
    } else if (itemText.compare("Quailty:") == 0) {
        videodDuration = itemNewText.toLower();
    } else if (itemText.compare("Media Type:") == 0) {
        if (itemNewText.compare("Channel") == 0 || itemNewText.compare("Playlist") == 0 || itemNewText.compare("Video") == 0) {
            type = itemNewText.toLower();
            eventType = "";
        } else if (itemNewText.compare("Any") == 0) {
            type = "";
            eventType = "";
        } else if (itemNewText.compare("Live") == 0) {
            type = "video";
            eventType = "&eventType=live";
        } else if (itemNewText.compare("Live (Upcoming)") == 0) {
            type = "video";
            eventType = "&eventType=upcoming";
        }
    }

    if (itemText.compare("Published Between:") == 0) {
        datePublishedBefore->SetData(getlist(pos, true));

        if (itemNewText.compare("Any") == 0) {
            dateBetweenUrlBuilder = "";
        } else {
            dateBetweenUrlBuilder = "&publishedBefore=" + translateWordtoRFC3339Date(itemNewText);
        }
    } else if (itemText.compare("And Published Before:") == 0) {
        dateBetween->SetData(getlist(pos, false));

        if (itemNewText.compare("Any") == 0) {
            datePublishedBeforeUrlBuilder = "";
        } else {
            datePublishedBeforeUrlBuilder = "&publishedAfter=" + translateWordtoRFC3339Date(itemNewText);
        }
    }
}

/** \brief convert word date to RFC3339 Date */
QString ytCustomApp::translateWordtoRFC3339Date(QString date) {
    QDateTime qdatetime = QDateTime::currentDateTime();
    qdatetime.setTime(QTime(0, 0, 0, 0));

    if (date.compare("1 Month Ago") == 0) {
        qdatetime = qdatetime.addDays(-31);
    } else if (date.compare("6 Months Ago") == 0) {
        qdatetime = qdatetime.addDays(-182);
    } else if (date.compare("1 Year Ago") == 0) {
        qdatetime = qdatetime.addDays(-365);
    } else if (date.compare("2 Years  Ago") == 0) {
        qdatetime = qdatetime.addDays(-730);
    } else if (date.compare("5 Years Ago") == 0) {
        qdatetime = qdatetime.addDays(-1825);
    } else if (date.compare("10 Years Ago") == 0) {
        qdatetime = qdatetime.addDays(-3650);
    } else if (date.compare("15 Years Ago") == 0) {
        qdatetime = qdatetime.addDays(-5475);
    }
    return qdatetime.toString(Qt::ISODate) + "Z";
}

/** \brief get a list of dates that can be used in the date range ui */
QStringList ytCustomApp::getlist(int currentPos, bool foward) {
    QStringList dateRange;
    if (foward) {
        dateRange = masterDateListPublishedBefore;
    } else { // backwards
        dateRange = masterDateListdateBetween;
    }

    if (foward) {
        for (int i; i < currentPos; i++) {
            dateRange.removeFirst();
        }
    } else { // backwards
        for (int i = currentPos + 1; i < masterDateListPublishedBefore.size(); i++) {
            dateRange.removeLast();
        }
    }
    return dateRange;
}
