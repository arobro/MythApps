#include "ytCustom.h"

// QT headers
#include <QDebug>
#include <QFileInfo>
#include <QLocale>
#include <QPainter>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

// MythTV headers
#include "libmyth/mythcontext.h"
#include "libmythui/mythmainwindow.h"

// MythApps headers
#include "SafeDelete.h"
#include "plugin_api.h"
#include "programData.h"
#include "shared.h"

// Globals
const QString ytAppPathName = "app://ytCustom/";

ytCustom::ytCustom() : pluginName("ytCustom"), pluginIcon("ytCustom.png") {
    ma_popular_icon = createImageCachePath("ma_popular.png");
    ma_search_icon = createImageCachePath("ma_search.png");

    netRequest = new NetRequest("", "", "", "", false);
}

ytCustom::~ytCustom() {
    // SafeDelete(dateAfter);
    // SafeDelete(dateBefore);
}

QString ytCustom::getPluginName() const { return pluginName; }

QString ytCustom::getPluginDisplayName() { return "YouTube"; }

QString ytCustom::logoUrl() const { return "https://upload.wikimedia.org/wikipedia/commons/thumb/2/20/YouTube_2024.svg/1024px-YouTube_2024.svg.png"; }

QString ytCustom::getKodiYTPlayUrl() { return "plugin://" + getKodiYTPluginDomain() + "/?action=play_video&videoid="; }

QString ytCustom::getKodiYTPluginDomain() { return "plugin.video.youtube"; }

QString ytCustom::getAPIBaseUrl() { return "https://www.googleapis.com/youtube/v3/"; }

QString ytCustom::hidePlugin() { return getKodiYTPluginDomain(); }

bool ytCustom::useBasicMenu() { return true; }

QString ytCustom::getPluginIcon() const {
    QString iconPath = getGlobalPathPrefix() + "/" + pluginIcon;

    if (!QFile::exists(iconPath)) {
        QByteArray ytCustomBytes = netRequest->downloadImage(logoUrl(), true);

        if (!ytCustomBytes.isEmpty()) {
            processAndSaveImage(ytCustomBytes, iconPath);
        }
    }

    return pluginIcon;
}

bool ytCustom::getPluginStartPos() const { return false; }

void ytCustom::load(const QString label, const QString data) {
    LOGS(0, "", "label", label, "data", data);
    m_toggleSearchVisibleCallback(true);

    if (data.length() < 2) {
        loadYTCustom();
    } else {
        updateMediaListCallback(label, data);
    }
}

void ytCustom::displayHomeScreenItems() { return; }

void ytCustom::loadYTCustom() {
    setWidgetVisibility(uiCtx->searchSettingsGroup, true);
    initializeSettingsDialog();

    ytApi = gCoreContext->GetSetting("MythAppsYTapi");
    ytID = gCoreContext->GetSetting("MythAppsYTID");

    if (ytApi.length() < 10 || ytID.length() < 10) {
        LOG(VB_GENERAL, LOG_DEBUG, "No API key");
        dialog->createAutoClosingBusyDialog(tr("Please enter in your API key under MythApps->Settings"), 3);
        return;
    }

    m_loadProgramCallback(tr("Search Settings"), ytAppPathName + "settings&refreshGrid=false", ma_search_icon, nullptr);
    m_loadProgramCallback(tr("Popular Right Now"), createProgramData(ytAppPathName + "popular", tr("Browse the most popular content"), ma_popular_icon, false, ""), ma_popular_icon, nullptr);
    m_loadProgramCallback(tr("Wrapped App"), createProgramData("plugin://" + getKodiYTPluginDomain(), tr("Wrapped App"), ma_popular_icon, false, ""), ma_popular_icon, nullptr);
}

void ytCustom::updateMediaListCallback(const QString &label, const QString &data) {
    LOGS(0, "", "label", label, "data", data);

    setWidgetVisibility(uiCtx->searchSettingsGroup, false);

    ProgramData programData(label, data);
    QString path = programData.getFilePathParam();

    if (path.compare("settings&refreshGrid=false") == 0) {
        LOG(VB_GENERAL, LOG_DEBUG, "ytCustom::updateMediaListCallback() -settings");
        setWidgetVisibility(uiCtx->searchSettingsGroup, true);
        SetFocusWidget(uiCtx->searchSettingsButtonList);
        return;
    }

    loadProgramList("", path);
}

void ytCustom::search(const QString &searchText) {
    LOGS(0, "", "searchText", searchText);
    setWidgetVisibility(uiCtx->searchSettingsGroup, false);

    loadProgramList(searchText, "");
}

/** \brief get json list of videoes
 *  \param searchText what to search for
 *  \param directory what directory path to load */
QList<QVariant> ytCustom::getVideos(QString searchText, QString directory) {
    LOGS(0, "", "searchText", searchText, "directory", directory);
    QString query = getAPIBaseUrl() + "videos?part=snippet&maxResults=50&chart=mostPopular";

    if (!searchText.isEmpty()) {
        query = getAPIBaseUrl() + "search?part=snippet&maxResults=35";

        if (!type.isEmpty())
            query += "&type=" + type;

        if (!eventType.isEmpty())
            query += "&eventType=" + eventType;

        if (!dateBeforeUrlBuilder.isEmpty())
            query += "&publishedBefore=" + dateBeforeUrlBuilder;

        if (!dateAfterUrlBuilder.isEmpty())
            query += "&publishedAfter=" + dateAfterUrlBuilder;

        query += "&order=" + sortBy;
        query += "&videoDuration=" + videoDuration;
        query += "&q=" + QUrl::toPercentEncoding(searchText);
    }

    if (!directory.isEmpty()) {
        if (directory.startsWith(ytAppPathName + "channel/")) {
            directory.replace(ytAppPathName + "channel/", "");
            query = getAPIBaseUrl() + "search?part=snippet&maxResults=50&channelId=" + directory;
        } else if (directory.startsWith(ytAppPathName + "playlist/")) {
            directory.replace(ytAppPathName + "playlist/", "");
            query = getAPIBaseUrl() + "playlistItems?part=snippet&maxResults=50&playlistId=" + directory;
        }
    }

    query += "&regionCode=" + getRegionCodeFromOS();

    LOG(VB_GENERAL, LOG_DEBUG, "getVideos() query- " + query);

    QString answer = netRequest->requestUrlPublic(query + "&key=" + ytApi, ytID);

    if (answer.contains("\"Forbidden\"")) {
        LOG(VB_GENERAL, LOG_ERR, "Forbidden. exceeded quota?");
    }

    QJsonDocument jsonDocument = QJsonDocument::fromJson(answer.toUtf8());
    QJsonObject jObject = jsonDocument.object();
    QVariantMap mainMap = jObject.toVariantMap();

    return mainMap["items"].toList();
}

void ytCustom::loadProgramList(QString searchText, QString directory) {
    LOGS(1, "", "searchText", searchText, "directory", directory);

    dialog->getLoader()->SetVisible(true);

    QList<QStringList> programList;
    QList<QVariant> list = getVideos(searchText, directory);

    int count = 0;
    for (const QVariant &item : list) {
        QVariantMap map = item.toMap();

        QString url, desc, imgurl, prefix, title;
        bool autoPlay = false;

        QString kind = map["kind"].toString();

        // Determine URL and prefix based on kind
        if (kind == "youtube#video") {
            url = map["id"].toString();
            prefix = getKodiYTPlayUrl();
            autoPlay = true;
        } else if (kind == "youtube#searchResult") {
            QVariantMap idMap = map["id"].toMap();
            QString subKind = idMap["kind"].toString();

            if (subKind.contains("#video")) {
                url = idMap["videoId"].toString();
                prefix = getKodiYTPlayUrl();
                autoPlay = true;
            } else if (subKind.contains("#channel")) {
                url = idMap["channelId"].toString();
                prefix = ytAppPathName + "channel/";
            } else if (subKind.contains("#playlist")) {
                url = idMap["playlistId"].toString();
                prefix = ytAppPathName + "playlist/";
            }
        } else if (kind.contains("#playlistItem")) {
            QVariantMap snippetMap = map["snippet"].toMap();
            QVariantMap resourceMap = snippetMap["resourceId"].toMap();

            if (resourceMap["kind"].toString().contains("#video")) {
                url = resourceMap["videoId"].toString();
                prefix = getKodiYTPlayUrl();
                autoPlay = true;
            }
        }

        QVariantMap snippet = map["snippet"].toMap();
        title = snippet["title"].toString();
        desc = snippet["description"].toString();

        QVariantMap thumbnails = snippet["thumbnails"].toMap();
        QVariantMap mediumThumb = thumbnails["medium"].toMap();
        if (mediumThumb.isEmpty())
            mediumThumb = thumbnails["default"].toMap();

        imgurl = mediumThumb["url"].toString();

        programList.append({title, createProgramData(prefix + url, desc, imgurl, autoPlay, ""), imgurl});

        if (++count % 4 == 0)
            QCoreApplication::processEvents();
    }

    for (const QStringList &entry : programList) {
        m_loadProgramCallback(entry.at(0), entry.at(1), entry.at(2), nullptr);
    }
    dialog->getLoader()->SetVisible(false);
}

void ytCustom::searchSettingsClicked(MythUIButtonListItem *item) {
    LOGS(0, "");
    QStringList options = item->GetData().toStringList();
    int pos = (options.indexOf(item->GetText("buttontext2")) + 1) % options.size();
    QString itemNewText = options.at(pos);
    item->SetText(itemNewText, "buttontext2");

    QString header = item->GetText();
    if (header == "Sort By:") {
        sortBy = itemNewText.toLower();
    } else if (header == "Duration:") {
        videoDuration = itemNewText.toLower();
    } else if (header == "Media Type:") {
        if (itemNewText == "Any") {
            type.clear();
            eventType.clear();
        } else if (itemNewText == "Video") {
            type = "video";
            eventType.clear();
        } else if (itemNewText == "Channel") {
            type = "channel";
            eventType.clear();
        } else if (itemNewText == "Playlist") {
            type = "playlist";
            eventType.clear();
        } else if (itemNewText == "Live") {
            type = "video";
            eventType = "live";
        } else if (itemNewText == "Live (Upcoming)") {
            type = "video";
            eventType = "upcoming";
        }
    } else if (header == "Published Between:") { // Now sets the lower bound → publishedAfter
        if (itemNewText == "All Time") {
            dateAfterUrlBuilder.clear();
        } else {
            dateAfterUrlBuilder = translateWordtoRFC3339Date(itemNewText);
        }
    } else if (header == "And:") {
        if (itemNewText == "All Time") {
            dateBeforeUrlBuilder.clear();
        } else {
            dateBeforeUrlBuilder = translateWordtoRFC3339Date(itemNewText);
        }
    }
}

/** \brief convert word date to RFC3339 Date */
QString ytCustom::translateWordtoRFC3339Date(const QString &date) {
    LOGS(0, "", "date", date);
    static const QMap<QString, int> offsets = [] {
        QMap<QString, int> map;
        map.insert("1 Month Ago", 31);
        map.insert("6 Months Ago", 182);
        map.insert("1 Year Ago", 365);
        map.insert("2 Years Ago", 730);
        map.insert("5 Years Ago", 1825);
        map.insert("10 Years Ago", 3650);
        map.insert("15 Years Ago", 5475);
        return map;
    }();

    QDateTime dt = QDateTime::currentDateTime();
    dt.setTime(QTime(0, 0));
    return dt.addDays(-offsets.value(date, 0)).toUTC().toString(Qt::ISODate);
}

/** \brief get a list of dates that can be used in the date range ui */
QStringList ytCustom::getlist(int currentPos, bool forward) {
    LOGS(0, "", "currentPos", currentPos, "forward", forward);
    QStringList dateRange;
    if (forward) {
        dateRange = dateListBefore;
    } else { // backwards
        dateRange = dateListAfter;
    }

    if (forward) {
        for (int i = 0; i < currentPos; i++) {
            dateRange.removeFirst();
        }
    } else { // backwards
        for (int i = currentPos + 1; i < dateListBefore.size(); i++) {
            dateRange.removeLast();
        }
    }
    return dateRange;
}

void ytCustom::initializeSettingsDialog() {
    LOGS(0, "");
    if (uiCtx->searchSettingsButtonList->GetItemAt(0) != nullptr)
        return;

    QStringList dateListStart = {"All Time", "15 Years Ago", "10 Years Ago", "5 Years Ago", "2 Years Ago", "1 Year Ago", "6 Months Ago", "1 Month Ago"};
    QStringList dateListEnd = {"Now", "1 Month Ago", "6 Months Ago", "1 Year Ago", "2 Years Ago", "5 Years Ago", "10 Years Ago", "15 Years Ago", "All Time"};

    auto *item1 = new MythUIButtonListItem(uiCtx->searchSettingsButtonList, "Sort By:");
    sortBy = "relevance";
    item1->SetText("Relevance", "buttontext2");
    item1->SetData(QStringList({"Relevance", "Date"}));

    auto *item2 = new MythUIButtonListItem(uiCtx->searchSettingsButtonList, "Duration:");
    videoDuration = "any";
    item2->SetText("Any", "buttontext2");
    item2->SetData(QStringList({"Any", "Long"}));

    auto *item3 = new MythUIButtonListItem(uiCtx->searchSettingsButtonList, "Media Type:");
    type = "";
    eventType = "";
    item3->SetText("Any", "buttontext2");
    item3->SetData(QStringList({"Any", "Video", "Channel", "Live", "Live (Upcoming)", "Playlist"}));

    dateAfter = new MythUIButtonListItem(uiCtx->searchSettingsButtonList, "Published Between:");
    dateAfter->SetText("All Time", "buttontext2");
    dateAfter->SetData(dateListStart);

    dateBefore = new MythUIButtonListItem(uiCtx->searchSettingsButtonList, "And:");
    dateBefore->SetText("Now", "buttontext2");
    dateBefore->SetData(dateListEnd);

    dateAfterUrlBuilder = "";
    dateBeforeUrlBuilder = "";

    connect(uiCtx->searchSettingsButtonList, SIGNAL(itemClicked(MythUIButtonListItem *)), this, SLOT(searchSettingsClicked(MythUIButtonListItem *)));
}

void ytCustom::processAndSaveImage(const QByteArray &data, const QString &path) const {
    LOGS(0, "", "path", path);
    QImage img;
    if (!img.loadFromData(data))
        return;

    img = img.convertToFormat(QImage::Format_ARGB32);
    img = img.scaled(img.width() / 4, img.height() / 4, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    int size = std::max(img.width(), img.height());
    QImage canvas(size, size, QImage::Format_ARGB32);
    canvas.fill(Qt::white);

    QPainter p(&canvas);
    p.drawImage((size - img.width()) / 2, (size - img.height()) / 2, img);
    p.end();

    canvas.save(path);
}

/** \brief check if a video url is entered in the search box and open the video. */
bool ytCustom::handleSuggestion(const QString searchText) {
    LOGS(0, "", "searchText", searchText);
    if (searchText.contains(getPluginDisplayName(), Qt::CaseInsensitive) && searchText.contains("v=") && searchText.contains("http") && searchText.contains(".com")) {

        QUrl qu(searchText.trimmed());
        if (!qu.isValid())
            return false;

        QUrlQuery q(qu);
        QString vUrl = q.queryItemValue("v", QUrl::FullyDecoded);

        m_SetPlay_KodiCallback(getKodiYTPlayUrl() + vUrl);
        return true;
    }
    return false;
}

QString ytCustom::getRegionCodeFromOS() {
    LOGS(0, "");
    QLocale locale = QLocale::system();
    QStringList parts = locale.name().split('_');

    if (parts.size() == 2 && !parts.last().isEmpty()) {
        return parts.last().toUpper();
    }

    return "US";
}
