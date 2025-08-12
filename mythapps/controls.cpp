#include "controls.h"

// MythTV headers
#include <libmyth/mythcontext.h>
#include <libmythbase/mythdirs.h>

/** \param m_username Kodi username
 *  \param m_password Kodi password
 *  \param m_ip Kodi ip
 * 	\param m_port Kodi port */
Controls::Controls(const QString &m_ip, const QString &m_port, QObject *parent) : QObject(parent), ip(m_ip), port(m_port) {
    ip = m_ip;
    port = m_port;
    eventClientIpAddress = CAddress(m_ip.toLocal8Bit().constData());
    seekTimer.setSingleShot(true);
}

Controls::~Controls() {
    if (eventClientConnected) {
        CPacketBYE bye;
        bye.Send(sockfd, eventClientIpAddress);
    }
}

/** \brief connect to kodi event client (remote control interface) if not connected */
void Controls::ensureEventClient() {
    if (!eventClientConnected) {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            LOG(VB_GENERAL, LOG_ERR, "Error creating eventClient socket");
            return;
        }
        eventClientIpAddress.Bind(sockfd);
    }
    eventClientConnected = true;
}

/** send any remote-control action **/
void Controls::sendEventAction(const QString &action) {
    ensureEventClient();
    CPacketACTION pkt = CPacketACTION(action.toUtf8().constData());
    pkt.Send(sockfd, eventClientIpAddress);
}

/** window-minimize **/
void Controls::goMinimize() { sendEventAction("Minimize"); }

/** quit kodi **/
void Controls::quitKodi() { callJsonRpc("Application.Quit"); }

void Controls::initializeWebSocket() { netSocketRequest.reset(new NetSocketRequest(QString("ws://%1:9090").arg(ip))); }

/** high-level JSON-RPC caller; returns the "result" object **/
QJsonValue Controls::callJsonRpc(const QString &method, const QJsonObject &params, const QJsonArray &props) { return netSocketRequest->call(method, params, props); }

QString Controls::callJsonRpcString(const QString &method, const QJsonObject &params, const QJsonArray &props) {
    QJsonValue result = callJsonRpc(method, params, props);

    QJsonDocument doc;
    if (result.isObject()) {
        doc = QJsonDocument(result.toObject());
    } else if (result.isArray()) {
        doc = QJsonDocument(result.toArray());
    } else {
        QJsonObject wrapper;
        wrapper["value"] = result;
        doc = QJsonDocument(wrapper);
    }

    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

/** \brief toggle the player debug menu to show bitrate overlay
 *  \param doubleclick require the function to be called twice within one second. (used for double button press).*/
void Controls::togglePlayerDebug(bool doubleclick) {
    static QTime time = QTime(0, 0, 0, 0);

    if (!doubleclick || time > QTime::currentTime()) {
        LOG(VB_GENERAL, LOG_DEBUG, "togglePlayerDebug() -playerdebug");
        inputActionHelper("playerdebug");
    }
    time = QTime::currentTime().addSecs(1);
}

/** \brief Start kodi if not running */
void Controls::startKodiIfNotRunning() {
#ifdef __ANDROID__
#elif _WIN32
    system("tasklist /nh /fi \"imagename eq kodi.exe\" | find /i \"kodi.exe\" > "
           "nul || (start kodi_start.cmd)");
#else
    QString kodiName = "kodi";
    if (checkIfProgramInstalled("kodi-start")) { // used for testing
        kodiName = "kodi-start";
    } else if (!checkIfProgramInstalled(kodiName)) {
        kodiName = "flatpak run tv.kodi.Kodi"; // try flatpak kodi
    }
    if (gCoreContext->GetSetting("MythAppsInternalRemote").compare("1") == 0) {
        system("export LIRC_SOCKET_PATH=\"/\";if ! pgrep -x kodi > /dev/null; then " + kodiName.toLocal8Bit() + " & fi;");
    } else {
        system("if ! pgrep -x kodi > /dev/null; then " + kodiName.toLocal8Bit() + " & fi;");
    }
#endif
}

/** \brief Checks if the Kodi setting equals the given value.
 *  \param SettingName The name of the setting.
 *  \param SettingValue The expected value of the setting.
 *  \return true if the setting equals the value, false otherwise.
 */
bool Controls::isKodiSetting(const QString &SettingName, const QString &SettingValue) {
    QJsonObject params;
    params["setting"] = SettingName;

    QJsonObject result = callJsonRpc("Settings.GetSettingValue", params).toObject();

    if (!result.contains("value")) {
        return false;
    }

    QString currentValue = result["value"].toVariant().toString();
    return currentValue == SettingValue;
}

/** \brief set the setting in Kodi
 *  \param SettingName the setting name.
 *  \param SettingValue  the value of the setting.  */
template <typename T> void Controls::setKodiSetting(const QString &SettingName, const T &SettingValue) {
    QJsonObject params;
    params["setting"] = SettingName;
    params["value"] = SettingValue;
    callJsonRpc("Settings.SetSettingValue", params);
}

/** \brief are any addons installed? */
bool Controls::areAddonsInstalled() {
    QString answer = getAddons();

    if (answer.contains("total:0")) {
        return false;
    }
    return true;
}

/** \brief Get JSON with a list of video addons */
QString Controls::getAddons(bool forceRefresh) {
    static QString cachedResult;

    if (!cachedResult.isEmpty() && !forceRefresh) {
        return cachedResult;
    }

    QJsonArray properties = {"name", "description", "thumbnail"};

    QJsonObject params;
    params["type"] = "xbmc.addon.video";
    params["enabled"] = true;

    QJsonObject fullResponse = callJsonRpc("Addons.GetAddons", params, properties).toObject();

    QJsonObject wrapped;
    wrapped["result"] = fullResponse;

    QJsonDocument doc(wrapped);
    cachedResult = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    return cachedResult;
}

/** \brief Load all addons */
void Controls::loadAddons(QStringList hiddenPluginList) {
    QString json = getAddons(true);
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());

    QJsonObject root = doc.object();
    QJsonObject result = root.value("result").toObject();
    QJsonArray addons = result.value("addons").toArray();

    urlToThumbnailMap.clear();

    for (const QJsonValue &addonVal : addons) {
        QJsonObject addon = addonVal.toObject();
        QString addonId = addon.value("addonid").toString();
        QString name = addon.value("name").toString();
        QString description = addon.value("description").toString();
        QString thumbnail = addon.value("thumbnail").toString();

        // creates a map to lookup the thumbnail based on the addonid.
        urlToThumbnailMap.insert(getWebSiteDomain(addonId), thumbnail);

        // load program if not in hidden list
        if (!hiddenPluginList.contains(addonId)) {
            emit loadProgramSignal(name, createProgramData(addonId, description, thumbnail, false, ""), thumbnail);
        }
    }
}

QString Controls::getLocationFromUrlAddress(QString urlAddress) { return urlToThumbnailMap.value(getWebSiteDomain(urlAddress)); }

/** \brief  Checks if input adative addon installed. This is used by Kodi to play DRM video
 *  \return Is the input adative addon installed? */
bool Controls::isInputAdaptive() {
    QJsonObject params;
    params["type"] = "kodi.inputstream";
    params["enabled"] = true;

    QString result = callJsonRpcString("Addons.GetAddons", params);

    if (result.contains("inputstream.adaptive")) {
        return true;
    }
    return false;
}

/** \brief seek to a set position
 *  \param hours - time to seek
 *  \param minutes - time to seek
 *  \param seconds  - time to seek */
void Controls::seek(int hours, int minutes, int seconds) {
    int time = (hours * 3600) + (minutes * 60) + seconds;
    LOG(VB_GENERAL, LOG_DEBUG, "controls_seek() " + QString::number(time));

    QJsonObject valueObj;
    valueObj["seconds"] = time;

    QJsonObject params;
    params["playerid"] = getActivePlayer();
    params["value"] = valueObj;

    callJsonRpc("Player.Seek", params);
}

/** \brief active a window in Kodi
 *  \param window- name of window to active */
void Controls::activateWindow(QString window) {
    LOG(VB_GENERAL, LOG_DEBUG, "controls_activateSplash()");

    QJsonObject params;
    params["window"] = window;
    callJsonRpc("GUI.ActivateWindow", params);
}

/** \brief show the on screen display */
void Controls::showOSD() { callJsonRpc("Input.ShowOSD"); }

/** \brief show video player info */
void Controls::showPlayerProcessInfo() { callJsonRpc("Input.ShowPlayerProcessInfo"); }

/** \brief show video info in Kodi */
void Controls::showInfo() { callJsonRpc("Input.Info"); }

/** \brief Get the current volume level
 *  \return Volume as an integer (0–100) */
int Controls::getVol() {
    QJsonArray props;
    props.append("volume");

    QJsonObject result = callJsonRpc("Application.GetProperties", QJsonObject(), props).toObject();
    return result.value("volume").toInt();
}

/** \brief close dialog if one pops up. e.g. for a subscription
 *  \return True if the virtual keyboard is open, false otherwise */
bool Controls::isVirtualKeyboardOpen() {
    LOG(VB_GENERAL, LOG_DEBUG, "isVirtualKeyboardOpen()");

    QJsonObject params;
    QJsonArray labels;
    labels.append("System.CurrentWindow");
    params["labels"] = labels;

    QJsonObject result = callJsonRpc("XBMC.GetInfoLabels", params).toObject();

    QString currentWindow = result.value("System.CurrentWindow").toString();
    return currentWindow.compare("Virtual keyboard", Qt::CaseInsensitive) == 0;
}

/** \brief Retrieve a directory listing
 * \param url Path to the directory
 * \return get plot, thumbnail, file etc */
QJsonObject Controls::getDirectoryObject(QString url) {
    LOG(VB_GENERAL, LOG_DEBUG, "getDirectoryObject() " + url);

    QJsonObject params;
    params["directory"] = url;
    params["media"] = "video";
    params["properties"] = QJsonArray{"plot", "thumbnail", "file"};

    QJsonObject request;
    request["jsonrpc"] = "2.0";
    request["method"] = "Files.GetDirectory";
    request["params"] = params;
    request["id"] = 1;

    return request;
}

/** \brief press the back button in kodi */
void Controls::inputBack() { callJsonRpc("Input.Back"); }

/** \brief mute the playing media */
void Controls::setMute() {
    if (!isInternalVolEnabled()) {
        return;
    }

    QJsonObject params;
    params["mute"] = "toggle";

    callJsonRpc("Application.SetMute", params);
}

/** \brief increase the volume if enabled */
void Controls::increaseVol() {
    if (!isInternalVolEnabled()) {
        return;
    }

    QJsonObject params;
    params["volume"] = getVol() + 5;
    callJsonRpc("Application.SetVolume", params);
}

/** \brief decrease the volume if enabled */
void Controls::decreaseVol() {
    if (!isInternalVolEnabled()) {
        return;
    }

    QJsonObject params;
    params["volume"] = getVol() - 5;
    callJsonRpc("Application.SetVolume", params);
}

bool Controls::isInternalVolEnabled() { return gCoreContext->GetSetting("MythAppsInternalVol") == "1"; }

/** \brief is the video paused?
 *  \param playerid Kodi player id*/
bool Controls::isPaused() {
    QJsonObject params;
    params["playerid"] = getActivePlayer();

    QJsonArray props;
    props.append("speed");

    QJsonObject result = callJsonRpc("Player.GetProperties", params, props).toObject();
    return result.value("speed").toInt() == 0;
}

void Controls::setActivePlayer(int player) { globalActivePlayer = player; }

/** \brief kodi can have multible active video players.
 * \return the first active player id. zero indicates an error */
void Controls::setActivePlayer() {
    LOG(VB_GENERAL, LOG_DEBUG, "getActivePlayer()");

    const int maxAttempts = 3;
    int attempt = 0;

    while (attempt < maxAttempts) {
        QJsonArray players = callJsonRpc("Player.GetActivePlayers").toArray();

        if (!players.isEmpty()) {
            QJsonObject player = players.first().toObject();
            globalActivePlayer = player.value("playerid").toInt();
            LOG(VB_GENERAL, LOG_DEBUG, QString("Active player found: %1").arg(globalActivePlayer));
            return;
        }

        LOG(VB_GENERAL, LOG_WARNING, "No active player found, retrying...Attempt #" + (attempt + 1));
        delayMilli(500);
        attempt++;
    }

    LOG(VB_GENERAL, LOG_ERR, "Failed to get active player after multiple attempts");
}

int Controls::getActivePlayer() {
    QMutexLocker locker(&mutex);

    if (globalActivePlayer != -1)
        return globalActivePlayer;

    if (!settingInProgress) {
        settingInProgress = true;
        locker.unlock();
        setActivePlayer();
        locker.relock();

        settingInProgress = false;
        condition.wakeAll();
    } else {
        while (settingInProgress) {
            condition.wait(&mutex);
        }
    }

    return globalActivePlayer;
}

void Controls::resetActivePlayer() { globalActivePlayer = -1; }

/** \brief stop playing the video */
void Controls::stopPlayBack() {
    QJsonObject params;
    params["playerid"] = getActivePlayer();

    callJsonRpc("Player.Stop", params);
}

/** \brief is kodi fullscreen */
bool Controls::isFullscreen() {
    QJsonObject params;
    params["setting"] = QString("videoscreen.screen");

    QJsonObject result = callJsonRpc("Settings.GetSettingValue", params, QJsonArray()).toObject();
    int screenValue = result.value("value").toInt();

    return screenValue >= 0;
}

/** \brief check if the username and password is correct when connecting to kodi */
bool Controls::isUserNamePasswordCorrect() { return ping(); }

bool Controls::ping() {
    QJsonValue result = callJsonRpc("JSONRPC.Ping");
    return result.toString() == "pong";
}

/** \brief get the media currently playing time?
 *  \param playerid Kodi player id
 *  \return playback/total time*/
QVariantMap Controls::getPlayBackTime() {
    QJsonArray properties = {"time", "totaltime", "percentage"};
    QJsonObject params;
    params["playerid"] = getActivePlayer();

    QJsonObject playbackData = callJsonRpc("Player.GetProperties", params, properties).toObject();
    QVariantMap playbackMap = playbackData.toVariantMap();

    QVariantMap totalTimeMap = playbackMap["totaltime"].toMap();
    QTime totalTime;
    totalTime.setHMS(totalTimeMap["hours"].toInt(), totalTimeMap["minutes"].toInt(), totalTimeMap["seconds"].toInt());
    playbackMap["duration"] = totalTime.toString("hh:mm:ss");

    QVariantMap currentTimeMap = playbackMap["time"].toMap();
    QTime currentTime;
    currentTime.setHMS(currentTimeMap["hours"].toInt(), currentTimeMap["minutes"].toInt(), currentTimeMap["seconds"].toInt());

    return playbackMap;
}

/** \brief close open dialogs in kodi */
QString Controls::handleDialogs() {
    QJsonArray labels = {"System.CurrentWindow"};
    QJsonObject params;
    params["labels"] = labels;

    QJsonObject result = callJsonRpc("XBMC.GetInfoLabels", params).toObject();
    QString currentWindow = result.value("System.CurrentWindow").toString();

    if (currentWindow.compare("OK dialog") == 0) {
        inputBack();
    } else if (currentWindow.compare("Virtual keyboard") == 0) {
        LOG(VB_GENERAL, LOG_ERR, "Error – Virtual keyboard still open");
        inputBack();
    }

    return currentWindow;
}

/**  \brief Open media in Kodi and optionally resume playback */
void Controls::play(const QString &mediaLocation, const QString &seekAmount) {
    FFspeed = 1;

    QJsonObject item;
    item["file"] = mediaLocation.trimmed();

    QJsonObject params;
    params["item"] = item;

    QString trimmedSeek = seekAmount.trimmed();
    if (!trimmedSeek.isEmpty()) {
        QStringList parts = trimmedSeek.split(":");
        if (parts.size() == 3) {
            QJsonObject resume;
            resume["hours"] = parts.at(0).toInt();
            resume["minutes"] = parts.at(1).toInt();
            resume["seconds"] = parts.at(2).toInt();

            QJsonObject options;
            options["resume"] = resume;
            params["options"] = options;
        }
    }

    callJsonRpc("Player.Open", params);
}

/** \brief toggle pause the playing media */
void Controls::pauseToggle() {
    QJsonObject params;
    params["playerid"] = getActivePlayer();

    callJsonRpc("Player.PlayPause", params);
}

/** \brief Determines if media is currently playing */
bool Controls::isPlaying() {
    QString response = callJsonRpcString("Player.GetActivePlayers");

    QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8());
    QJsonArray players = doc.array();
    return !players.isEmpty();
}

/** \brief get videos on the file system in Kodi */
QVariantMap Controls::getVideos() {
    QJsonObject params;
    params["media"] = "video";

    QJsonObject result = callJsonRpc("Files.GetSources", params).toObject();

    QVariantMap videoSources;
    videoSources["sources"] = result["sources"].toVariant();

    return videoSources;
}

/** \brief sent text */
void Controls::inputSendText(const QString &text) {
    QJsonObject params;
    params["text"] = text;
    params["done"] = true;

    callJsonRpc("Input.SendText", params);
}

/** \brief get stream details helper function */
QString Controls::getStreamDetails() {
    QJsonArray properties = {"dynpath", "streamdetails"};
    QJsonObject params;
    params["playerid"] = getActivePlayer();

    return callJsonRpcString("Player.GetItem", params, properties);
}

/** \brief Format stream details to a string: codec, resolution, etc. */
QString Controls::getStreamDetailsAll() {
    LOG(VB_GENERAL, LOG_DEBUG, "getStreamDetailsAll()");
    QString response = getStreamDetails();
    if (response.contains("error") || response.contains("invalid"))
        return "";

    QJsonDocument doc = QJsonDocument::fromJson(response.toUtf8());
    QVariantMap item = doc.object().toVariantMap()["item"].toMap();
    QVariantMap detailsMap = item["streamdetails"].toMap();

    QString dynPath = item["dynpath"].toString();
    QString audioCodec, audioChannels = "unknown", videoCodec, durationStr, resolution;

    if (auto audio = detailsMap["audio"].toList().value(0).toMap(); !audio.isEmpty()) {
        int ch = audio["channels"].toInt();
        audioChannels = ch > 0 ? QString::number(ch) : "unknown";
        audioCodec = audio["codec"].toString();
    }

    if (auto video = detailsMap["video"].toList().value(0).toMap(); !video.isEmpty()) {
        int durationSec = video["duration"].toInt();
        durationStr = QTime::fromMSecsSinceStartOfDay(durationSec * 1000).toString("hh:mm:ss");
        videoCodec = video["codec"].toString();
        resolution = QString("%1x%2").arg(video["height"].toInt()).arg(video["width"].toInt());
    }

    QStringList parts;
    if (!durationStr.isEmpty())
        parts << "Duration: " + durationStr;
    if (!videoCodec.isEmpty() || !audioCodec.isEmpty())
        parts << "Codecs: " + videoCodec + "/" + audioCodec + ", Channels: " + audioChannels;
    if (!resolution.isEmpty())
        parts << "Resolution: " + resolution;

    QString details = parts.join(", ") + "\n";

    QFile log(getKodiLogPath());
    if (log.open(QIODevice::ReadOnly)) {
        QTextStream in(&log);
        QString videoInfo, audioInfo;
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.contains("CDVDVideoCodecFFmpeg::Open() Using codec:"))
                videoInfo = line;
            if (line.contains("Creating audio stream (codec id:"))
                audioInfo = line;
        }
        if (videoInfo.contains("codec:"))
            details += "Video Details: " + videoInfo.split("codec:").value(1).trimmed() + "\n";
        if (audioInfo.contains("Creating audio stream"))
            details += "Audio Details: " + audioInfo.split("Creating audio stream").value(1).trimmed() + "\n";
        log.close();
    }

    if (!dynPath.isEmpty())
        details += "\nDynpath: " + dynPath;
    return details;
}

/** \brief get Search Directory/ subfolder. Used by 'new search' button in kodi apps that display search in a sub folder
 * \param url search url to retrieve
 * \param  searchTextValue search text to look for.
 * \return fileurl / path of the directory as specified by searchTextValue  */
QString Controls::getSearchDirectory(QString url, QString searchTextValue) {
    QString trimmedUrl = url.trimmed();
    QJsonObject params;
    params["directory"] = trimmedUrl;
    params["media"] = "video";

    QJsonArray props{"plot", "thumbnail", "file"};

    QJsonObject response = callJsonRpc("Files.GetDirectory", params, props).toObject();
    QJsonArray entries = response["files"].toArray();

    for (const QJsonValue &entry : entries) { // Search for matching directory
        QJsonObject entryObj = entry.toObject();
        QString label = removeBBCode(entryObj.value("label").toString());
        QString fileType = entryObj.value("filetype").toString();
        QString fileUrl = entryObj.value("file").toString();

        if (label == searchTextValue && fileType == "directory") {
            LOG(VB_GENERAL, LOG_DEBUG, "getSearchDirectory - match: " + fileUrl);
            return fileUrl;
        }
    }
    return QString("");
}

/** \brief speed to fast foward at.
 * \param speed. Can be +- 1-32 times speed of the video */
void Controls::setSpeed(int speed) {
    QJsonObject params;
    params["playerid"] = getActivePlayer();
    params["speed"] = speed;

    callJsonRpc("Player.SetSpeed", params);
}

/** \brief fast foward */
void Controls::setFFWD() {
    if (FFspeed == 1) {
        FFspeed = 2;
    } else if (FFspeed < 1) {
        FFspeed = 1;
    } else if (FFspeed < 32) {
        FFspeed *= 2;
    }
    setSpeed(FFspeed);
}

/** \brief fast rewind */
void Controls::setRWND() {
    if (FFspeed == 1) {
        FFspeed = -2;
    } else if (FFspeed > 1) {
        FFspeed = 1;
    } else if (FFspeed > -8) {
        FFspeed *= 2;
    }
    setSpeed(FFspeed);
}

/** \brief seek foward 30 seconds * the number of times the button is pressed. */
void Controls::seekFoward() { queueSeek(30); }

/** \brief Seek backward 10 seconds × number of rapid presses (max 8x). */
void Controls::seekBack() { queueSeek(-10); }

void Controls::queueSeek(int seconds) {
    LOG(VB_GENERAL, LOG_DEBUG, "queueSeek()");
    pendingSeekSeconds += seconds;

    if (!seekTimer.isActive()) {
        seekTimer.singleShot(400, this, [this]() {
            if (pendingSeekSeconds != 0) {
                seek(0, 0, pendingSeekSeconds);
                pendingSeekSeconds = 0;
            }
        });
    }
}

void Controls::inputActionHelper(QString action) {
    QJsonObject params;
    params["action"] = action;
    callJsonRpc("Input.ExecuteAction", params);
}

void Controls::activateFullscreenVideo() {
    QJsonObject params;
    params["window"] = "fullscreenvideo";

    callJsonRpc("GUI.ActivateWindow", params);
}

/** \brief toggle kodi fullscreen */
void Controls::toggleFullscreen() {
#ifdef __ANDROID__
    return;
#endif
    inputActionHelper("togglefullscreen");
}

/** \brief switch between Kodi and MythTV on Android
 * 	\param app name of app to switch to
 *  \return is the helper switching app (mythapp services) running? */
bool Controls::androidAppSwitch(QString app) {
    return netSocketRequest->androidAppSwitch(app);
    return false;
}

/** \brief set the connection status
 * 	\param connectStatus the new connection status */
void Controls::setConnected(int connectStatus) { connected = connectStatus; }

/** \brief get the connection status
 *  \return 0 = not connected, 1 = connected, 2 = connected and authenticated */
int Controls::getConnected() { return connected; }

void Controls::waitUntilKodiPingable() {
    QTime deadline = QTime::currentTime().addSecs(4);
    bool port9090Ok = false;
    bool eventPortOk = false;

    while (QTime::currentTime() < deadline) {
        port9090Ok = isKodiPingable(ip, "9090");
        eventPortOk = isKodiPingable(ip, port);

        if (port9090Ok && eventPortOk) {
            setConnected(1);
            return;
        }
        delayMilli(50);
    }

    if (!port9090Ok)
        LOG(VB_GENERAL, LOG_ERR, "Control port 9090 is not responding to ping");

    if (!eventPortOk)
        LOG(VB_GENERAL, LOG_ERR, "HTTP port is not responding to ping");
}

// music

/** \brief set the number of seconds for music crossfade */
void Controls::setCrossFade(int seconds) { setKodiSetting("musicplayer.crossfade", seconds); }

/** \brief is crossfade enabled? */
bool Controls::getCrossFade() { return isKodiSetting("musicplayer.crossfade", "0"); }

/** \brief set Audio Library Scan */
void Controls::setAudioLibraryScan() { callJsonRpc("AudioLibrary.Scan"); }

/** \brief set ProjectM visualization */
void Controls::setProjectM() { setKodiSetting("musicplayer.visualisation", "visualization.projectm"); }

void Controls::playListClear() {
    LOG(VB_GENERAL, LOG_DEBUG, "playListClear()");
    if (globalActivePlayer == 0) {
        return;
    }

    QJsonObject params;
    params["playlistid"] = getActivePlayer();

    callJsonRpc("Playlist.Clear", params);
}

void Controls::playListOpen(int position) {
    LOG(VB_GENERAL, LOG_DEBUG, "playListOpen()");

    QJsonObject itemObj;
    itemObj["playlistid"] = 1;
    itemObj["position"] = position;

    QJsonObject params;
    params["item"] = itemObj;

    callJsonRpc("Player.Open", params);
}

void Controls::playListAdd(QString file) {
    LOG(VB_GENERAL, LOG_DEBUG, "playListAdd(): " + file);

    QJsonObject itemObj;
    itemObj["file"] = file;

    QJsonObject params;
    params["playlistid"] = 1;
    params["item"] = itemObj;

    callJsonRpc("Playlist.Add", params);
}

/** \brief set part mode for music */
void Controls::setPartyMode() {
    QJsonObject obj2;
    obj2["partymode"] = "music";

    QJsonObject params;
    params["item"] = obj2;

    callJsonRpc("Player.Open", params);
}

QString Controls::playerGetItem() {
    QJsonArray array;
    array.push_back("album");
    array.push_back("artist");
    array.push_back("thumbnail");

    QJsonObject params;
    params["playerid"] = getActivePlayer();
    params["properties"] = array;

    return callJsonRpcString("Player.GetItem", params, array); // new
}

void Controls::removeFromPlaylist(int inPlaylistPos) {
    QJsonObject params;
    params["playlistid"] = 1;
    params["position"] = inPlaylistPos;

    callJsonRpc("Playlist.Remove", params);
}

qint64 Controls::getTimeFromSeekTimeMs(const QString &message) {
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    QJsonObject rootObj = doc.object();
    QJsonObject paramsObj = rootObj["params"].toObject();
    QJsonObject dataObj = paramsObj["data"].toObject();
    QJsonObject playerObj = dataObj["player"].toObject();
    QJsonObject timeObj = playerObj["time"].toObject();

    int hours = timeObj["hours"].toInt();
    int minutes = timeObj["minutes"].toInt();
    int seconds = timeObj["seconds"].toInt();
    int millis = timeObj["milliseconds"].toInt();

    QTime time(hours, minutes, seconds, millis);
    return QTime(0, 0).msecsTo(time);
}
