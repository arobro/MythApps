#include "controls.h"
#include <libmyth/mythcontext.h>
#include <libmythbase/mythdirs.h>

/** \param m_username Kodi username
 *  \param m_password Kodi password
 *  \param m_ip Kodi ip
 * 	\param m_port Kodi port */
Controls::Controls(QString m_username, QString m_password, QString m_ip, QString m_port) {
    netRequest = new NetRequest(m_username, m_password, m_ip, m_port, false);
    eventClientIpAddress = CAddress(m_ip.toLocal8Bit().constData());
}

Controls::~Controls() {
    if (eventClientConnected) {
        CPacketBYE bye;
        bye.Send(sockfd, eventClientIpAddress); //disconnect Kodi EventClient
    }
    delete netRequest;
}

/** \brief connect to kodi event client (remote control interface) if not connected */
void Controls::checkEventClientConnected() {
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

/** \brief minimizes kodi */
void Controls::goMinimize() {
    LOG(VB_GENERAL, LOG_INFO, "controls->goMinimize()");

    checkEventClientConnected();
    CPacketACTION action("Minimize");
    action.Send(sockfd, eventClientIpAddress);
}

/** \brief toggle the player debug menu to show bitrate overlay 
 *  \param doubleclick require the function to be called twice within one second. (used for double button press).*/
void Controls::togglePlayerDebug(bool doubleclick) {
    static QTime time = QTime(0, 0,0, 0);

    if (!doubleclick || time > QTime::currentTime()) {
        LOG(VB_GENERAL, LOG_DEBUG, "togglePlayerDebug() -playerdebug");

        checkEventClientConnected();
        CPacketACTION action("playerdebug", ACTION_BUTTON);
        action.Send(sockfd, eventClientIpAddress);
    }
    time = QTime::currentTime().addSecs(1);
}

/** \brief helper function for netRequest->requestUrl() */
QString Controls::requestUrl(QJsonObject value) {
    if (getConnected() == 0) {
        return "Connection refused";
    }
    return netRequest->requestUrl(value);
}

/** \brief Start kodi if not running */
void Controls::startKodiIfNotRunning() {
#ifdef __ANDROID__
#elif _WIN32
    system("tasklist /nh /fi \"imagename eq kodi.exe\" | find /i \"kodi.exe\" > "
           "nul || (start kodi_start.cmd)");
#else
    QString kodiName = "kodi";
    if (system("command -v kodi >/dev/null 2>&1 || { exit 1; }") != 0) { // kodi found
        kodiName = "flatpak run tv.kodi.Kodi";                           // try flatpak kodi
    }
    if (gCoreContext->GetSetting("MythAppsInternalRemote").compare("1") == 0) {
        system("export LIRC_SOCKET_PATH=\"/\";if ! pgrep -x kodi > /dev/null; then " + kodiName.toLocal8Bit() + " & fi;");
    } else {
        system("if ! pgrep -x kodi > /dev/null; then " + kodiName.toLocal8Bit() + " & fi;");
    }
#endif
}

/** \brief is the setting equal to value in Kodi?
 *  \param SettingName the setting name.
 *  \param SettingValue  the value of the setting.
 *  \return does the SettingValue equal the corrosponding setting?  */
bool Controls::isKodiSetting(QString SettingName, QString SettingValue) {
    QJsonObject paramsObj;
    paramsObj["setting"] = SettingName;
    QString answer = fetchUrlJson("Settings.GetSettingValue", paramsObj);

    if (answer.compare(SettingValue) == 0) {
        return false;
    }

    QJsonDocument jsonDocument = QJsonDocument::fromJson(answer.toLocal8Bit().data());
    QJsonObject jObject = jsonDocument.object();
    QVariantMap mainMap = jObject.toVariantMap();
    QVariantMap map2 = mainMap["result"].toMap();

    if (map2["value"].toString().compare(SettingValue) == 0) {
        return false;
    }
    return true;
}

/** \brief set the setting in Kodi
 *  \param SettingName the setting name.
 *  \param SettingValue  the value of the setting.  */
template <typename T> void Controls::setKodiSetting(QString SettingName, T SettingValue) {
    QJsonObject obj;
    obj["setting"] = SettingName;
    obj["value"] = SettingValue;
    fetchUrlJson("Settings.SetSettingValue", obj);
}

/** \brief build and request the json string for kodi
 *  \param method method to send
 *  \param paramsObj the parameters to send. Optional parameter.
 *  \param property any properties to send. Optional parameter.
 *  \return json response from kodi */
QString Controls::fetchUrlJson(QString method, QJsonObject paramsObj, QJsonArray property) {
    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = method;

    if (paramsObj.size() > 0) {
        if (property.size() > 0) {
            paramsObj["properties"] = property;
        }
        jsonObj["params"] = paramsObj;
    }

    return requestUrl(jsonObj);
}

/** \brief are any addons installed? */
bool Controls::areAddonsInstalled() {
    QString answer = getAddons();

    if (answer.contains("total:0")) {
        return false;
    }
    return true;
}

/** \brief get json with a list of addons */
QString Controls::getAddons() {
    QJsonObject paramsObj;
    paramsObj["type"] = "xbmc.addon.video";
    paramsObj["enabled"] = true;
    return fetchUrlJson("Addons.GetAddons", paramsObj);
}

/** \brief  Checks if input adative addon installed. This is used by Kodi to play DRM video
 *  \return Is the input adative addon installed? */
bool Controls::isInputAdaptive() {
    QJsonObject paramsObj;
    paramsObj["type"] = "kodi.inputstream";

    QString answer = fetchUrlJson("Addons.GetAddons", paramsObj);

    if (answer.contains("inputstream.adaptive")) {
        return true;
    }
    return false;
}

/** \brief seek to a set position
 *  \param hours - time to seek
 *  \param minutes - time to seek
 *  \param seconds  - time to seek */
void Controls::seek(int hours, int minutes, int seconds) {
    int time = (hours * 60 * 60) + (minutes * 60) + seconds;
    LOG(VB_GENERAL, LOG_DEBUG, "controls_seek()" + QString::number(time));

    QJsonObject obj2;
    obj2["seconds"] = time;

    QJsonObject paramsObj;
    paramsObj["playerid"] = getActivePlayer();
    paramsObj["value"] = obj2;

    fetchUrlJson("Player.Seek", paramsObj);
}

/** \brief seek to a set position */
void Controls::seek(QString seekAmount) {
    QStringList seekTime = seekAmount.split(":");
    if (seekTime.size() >= 2) {
        seek(seekTime.at(0).toInt(), seekTime.at(1).toInt(), seekTime.at(2).toInt()); // seek to timestamp
    }
}

/** \brief active a window in Kodi
 *  \param window- name of window to active */
void Controls::activateWindow(QString window) {
    LOG(VB_GENERAL, LOG_DEBUG, "controls_activateSplash()");

    QJsonObject paramsObj;
    paramsObj["window"] = window;
    fetchUrlJson("GUI.ActivateWindow", paramsObj);
}

/** \brief show the on screen display */
void Controls::showOSD() { fetchUrlJson("Input.ShowOSD"); }

/** \brief show video player info */
void Controls::showPlayerProcessInfo() { fetchUrlJson("Input.ShowPlayerProcessInfo"); }

/** \brief show video info in Kodi */
void Controls::showInfo() { fetchUrlJson("Input.Info"); }

/** \brief get the volume
 *  \return volume level */
int Controls::getVol() {
    QJsonArray array;
    array.push_back("volume");

    QJsonObject paramsObj;
    paramsObj["properties"] = array;

    fetchUrlJson("Application.SetVolume", paramsObj, array).toInt();
}

/** \brief close dialog if one pops up. e.g. for a subscription */
bool Controls::isVirtualKeyboardOpen() {
    LOG(VB_GENERAL, LOG_DEBUG, "isVirtualKeyboardOpen()");

    QJsonArray array;
    array.push_back("System.CurrentWindow");
    QJsonObject paramsObj;
    paramsObj["labels"] = array;

    QString answer = fetchUrlJson("XBMC.GetInfoLabels", paramsObj);

    QJsonDocument jsonDocument = QJsonDocument::fromJson(answer.toLocal8Bit().data());
    QJsonObject jObject = jsonDocument.object();
    QVariantMap mainMap = jObject.toVariantMap();
    QVariantMap map2 = mainMap["result"].toMap();

    if (map2["System.CurrentWindow"].toString().compare("Virtual keyboard") == 0) {
        return true;
    }
    return false;
}

/** \brief Files.GetDirectory
 * \param url directory url
 * \return get plot, thumbnail, file etc */
QJsonObject Controls::getDirectoryObject(QString url) {
    LOG(VB_GENERAL, LOG_DEBUG, "getDirectoryObject() " + url);

    QJsonObject jsonObj;
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "Files.GetDirectory";

    QJsonArray array;
    array.push_back("plot");
    array.push_back("thumbnail");
    array.push_back("file");

    QJsonObject paramsObj;
    paramsObj["directory"] = url;
    paramsObj["media"] = "video";
    paramsObj["properties"] = array;

    jsonObj["params"] = paramsObj;
    jsonObj["id"] = "1";

    return jsonObj;
}

/** \brief press the back button in kodi */
void Controls::inputBack() { fetchUrlJson("Input.Back"); }

/** \brief mute the playing media */
void Controls::setMute() {
    if (!gCoreContext->GetSetting("MythAppsInternalMute").compare("1") == 0) {
        return; // internal mute disabled
    }

    QJsonObject paramsObj;
    paramsObj["mute"] = "toggle";

    fetchUrlJson("Application.SetMute", paramsObj);
}

/** \brief increase the volume if enabled */
void Controls::increaseVol() {
    if (!gCoreContext->GetSetting("MythAppsInternalVol").compare("1") == 0) {
        return; // internal volume disabled
    }

    QJsonObject paramsObj;
    paramsObj["volume"] = getVol() + 5;
    fetchUrlJson("Application.SetVolume", paramsObj);
}

/** \brief decrease the volume if enabled */
void Controls::decreaseVol() {
    if (!gCoreContext->GetSetting("MythAppsInternalVol").compare("1") == 0) {
        return; // internal volume disabled
    }

    QJsonObject paramsObj;
    paramsObj["volume"] = getVol() - 5;
    fetchUrlJson("Application.SetVolume", paramsObj);
}

/** \brief is the video paused?
 *  \param playerid Kodi player id*/
bool Controls::isPaused(int playerid) {
    QJsonArray array;
    array.push_back("speed");

    QJsonObject paramsObj;
    paramsObj["playerid"] = playerid;

    QString answer = fetchUrlJson("Player.GetProperties", paramsObj, array);

    // parse json
    QJsonDocument jsonDocument = QJsonDocument::fromJson(answer.toLocal8Bit().data());
    QJsonObject jObject = jsonDocument.object();
    QVariantMap mainMap = jObject.toVariantMap();

    QVariantMap map2 = mainMap["result"].toMap();
    if (map2["speed"].toString().toInt() == 0) {
        return true;
    }
    return false;
}

/** \brief kodi can have multible active video players.
 * \return the first active player id. zero indicates an error */
int Controls::getActivePlayer() {
    LOG(VB_GENERAL, LOG_DEBUG, "getActivePlayer()");
    delayMilli(100);

    QString answer = fetchUrlJson("Player.GetActivePlayers");

    // error handling
    if (answer.contains("error") or not answer.contains("playerid")) {
        delayMilli(500);
        LOG(VB_GENERAL, LOG_ERR, "no getActivePlayer");
        return 0;
    }

    // parse the json response
    QJsonDocument jsonDocument = QJsonDocument::fromJson(answer.toLocal8Bit().data());
    QJsonObject jObject = jsonDocument.object();
    QVariantMap mainMap = jObject.toVariantMap();

    QList list3 = mainMap["result"].toList();

    foreach (QVariant T, list3) {
        QVariantMap map4 = T.toMap();
        return map4["playerid"].toString().toInt();
    }
    return 0;
}

/** \brief stop playing the video */
int Controls::stopPlayBack() {
    int activePlayer = getActivePlayer();

    QJsonObject paramsObj;
    paramsObj["playerid"] = activePlayer;

    fetchUrlJson("Player.Stop", paramsObj);
    return activePlayer;
}

/** \brief is kodi fullscreen */
bool Controls::isFullscreenBool() {
    if (isFullscreen().compare("1")) {
        return true;
    }
    return false;
}
/** \brief is kodi fullscreen
 * \return 1 - yes, 0 no or 'Connection refused' */
QString Controls::isFullscreen() {
    QJsonObject paramsObj;
    paramsObj["setting"] = "videoscreen.screen";

    QString windowMode = fetchUrlJson("Settings.GetSettingValue", paramsObj);

    if (windowMode.compare("Connection refused") == 0) {
        return "Connection refused";
    }

    QJsonDocument jsonDocument = QJsonDocument::fromJson(windowMode.toLocal8Bit().data());
    QJsonObject jObject = jsonDocument.object();
    QVariantMap mainMap = jObject.toVariantMap();
    QVariantMap map2 = mainMap["result"].toMap();
    QString status = map2["value"].toString().replace("-", "");

    if (status.compare("") == 0) {
        return "0";
    }

    return status;
}

/** \brief check if the username and password is correct when connecting to kodi
 */
bool Controls::isUserNamePasswordCorrect() { return isKodiSetting("videoscreen.screen", "Host requires authentication"); }

/** \brief get the media currently playing time?
 *  \param playerid Kodi player id
 *  \return playback/total time*/
QVariantMap Controls::getPlayBackTime(int playerid) {
    QJsonArray array;
    array.push_back("time");
    array.push_back("totaltime");
    array.push_back("percentage");

    QJsonObject paramsObj;
    paramsObj["playerid"] = playerid;

    QString answer = fetchUrlJson("Player.GetProperties", paramsObj, array);

    if (answer.contains("error")) {
        delayMilli(500);
        LOG(VB_GENERAL, LOG_ERR, "getPlayBackTime() error: " + answer);
    }

    QJsonDocument jsonDocument = QJsonDocument::fromJson(answer.toLocal8Bit().data());
    QJsonObject jObject = jsonDocument.object();
    QVariantMap mainMap = jObject.toVariantMap();

    QVariantMap map2 = mainMap["result"].toMap();

    setTotalTime(map2["totaltime"].toMap());
    return map2;
}
/** \brief update the media total playing time into the globalDuration varible
 */
void Controls::setTotalTime(QVariantMap map) {
    QString minutes = map["minutes"].toString();
    QString seconds = map["seconds"].toString();

    if (minutes.size() == 1) {
        minutes = "0" + minutes;
    }
    if (seconds.size() == 1) {
        seconds = "0" + seconds;
    }

    globalDuration = minutes.rightJustified(2, '0') + ":" + seconds.rightJustified(2, '0');
}

QString Controls::getGlobalDuration() { return globalDuration; }

/** \brief close open dialogs in kodi */
QString Controls::handleDialogs() {
    QJsonArray array;
    array.push_back("System.CurrentWindow");
    QJsonObject paramsObj;
    paramsObj["labels"] = array;

    QString answer = fetchUrlJson("XBMC.GetInfoLabels", paramsObj);

    QJsonDocument jsonDocument = QJsonDocument::fromJson(answer.toLocal8Bit().data());
    QJsonObject jObject = jsonDocument.object();
    QVariantMap mainMap = jObject.toVariantMap();
    QVariantMap map2 = mainMap["result"].toMap();

    if (map2["System.CurrentWindow"].toString().compare("OK dialog") == 0) {
        inputBack();
    } else if (map2["System.CurrentWindow"].toString().compare("Virtual keyboard") == 0) {
        LOG(VB_GENERAL, LOG_ERR, "error - Virtual keyboard still open");
        inputBack();
    }

    return map2["System.CurrentWindow"].toString();
}

/** \brief play the media in Kodi */
void Controls::play(QString mediaLocation) {
    FFspeed = 1;
    QJsonObject obj;
    obj["file"] = mediaLocation.trimmed();
    QJsonObject paramsObj;
    paramsObj["item"] = obj;

    fetchUrlJson("Player.Open", paramsObj);
}

/** \brief toggle pause the playing media */
void Controls::pauseToggle(int activePlayerStatus) {
    QJsonObject paramsObj;
    paramsObj["playerid"] = activePlayerStatus;

    fetchUrlJson("Player.PlayPause", paramsObj);
}

/** \brief used to determine if media is playing */
QString Controls::isPlaying() { return fetchUrlJson("Player.GetActivePlayers"); }

/** \brief get videos on the file system in Kodi */
QVariantMap Controls::getVideos() {
    QJsonObject paramsObj;
    paramsObj["media"] = "video";

    QString answer = fetchUrlJson("Files.GetSources", paramsObj);

    QJsonDocument jsonDocument = QJsonDocument::fromJson(answer.toLocal8Bit().data());
    QJsonObject jObject = jsonDocument.object();
    QVariantMap mainMap = jObject.toVariantMap();

    QVariantMap map2 = mainMap["result"].toMap();
    return map2;
}

/** \brief sent text */
void Controls::inputSendText(QString text) {
    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "Input.SendText";
    QJsonArray array;
    array.push_back(text);
    jsonObj["params"] = array;

    requestUrl(jsonObj);
}

/** \brief get stream details helper function */
QString Controls::getStreamDetails(int playerId) {
    QJsonArray array;
    array.push_back("dynpath");
    array.push_back("streamdetails");

    QJsonObject paramsObj;
    paramsObj["playerid"] = playerId;
    paramsObj["properties"] = array;

    return fetchUrlJson("Player.GetItem", paramsObj);
}

/** \brief format stream details to a string. Codec, Resolution etc */
QString Controls::getStreamDetailsAll(int playerId) {
    LOG(VB_GENERAL, LOG_DEBUG, "getStreamDetails()");
    QString answer = getStreamDetails(playerId);

    if (answer.contains("error") || answer.contains("invaild")) {
        return "";
    }

    QJsonDocument jsonDocument = QJsonDocument::fromJson(answer.toLocal8Bit().data());
    QJsonObject jObject = jsonDocument.object();
    QVariantMap mainMap = jObject.toVariantMap();

    QVariantMap map2 = mainMap["result"].toMap();
    QVariantMap map3 = map2["item"].toMap();
    QString dynpath = map3["dynpath"].toString();

    QVariantMap map4 = map3["streamdetails"].toMap();

    QList list5 = map4["audio"].toList();

    QString channels;
    QString aCodec;

    foreach (QVariant T, list5) {
        QVariantMap map6 = T.toMap();
        channels = map6["channels"].toString();
        aCodec = map6["codec"].toString();
    }

    QString duration;
    QString vCodec;
    QString height;
    QString width;

    QList list7 = map4["video"].toList();
    foreach (QVariant T, list7) {
        QVariantMap map8 = T.toMap();
        duration = map8["duration"].toString();
        vCodec = map8["codec"].toString();
        height = map8["height"].toString();
        width = map8["width"].toString();
    }

    QString details = "";

    if (!duration.compare("") == 0) {
        details = details + ", Duration: " + (duration);
    }
    if (!vCodec.compare("") == 0) {
        details = details + ", Codecs: " + vCodec + "/" + aCodec + ", Channels: " + channels;
    }
    if (!height.compare("") == 0) {
        details = details + ", Resolution: " + height + "x" + width;
    }
    details = details + "\n";

    QString kodiDir = GetConfDir().replace(".mythtv", "");
    QFile KodiLog(kodiDir + ".kodi/temp/kodi.log");

    if (KodiLog.open(QIODevice::ReadOnly)) {
        QTextStream in(&KodiLog);
        QString videoInfo = "";
        QString audioInfo = "";
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.contains("CDVDVideoCodecFFmpeg::Open() Using codec:")) {
                videoInfo = line;
            }
            if (line.contains("Creating audio stream (codec id:")) {
                audioInfo = line;
            }
        }
        QStringList videoInfoList = videoInfo.split("codec:");
        if (videoInfoList.size() > 1) {
            details = details + "Video Details:" + videoInfoList.at(1) + "\n";
        }

        QStringList audioInfoList = audioInfo.split("Creating audio stream");
        if (audioInfoList.size() > 1) {
            details = details + "Audio Details:" + audioInfoList.at(1) + "\n";
        }
        KodiLog.close();
    }

    if (!dynpath.compare("") == 0) {
        details = details + "\nDynpath: " + dynpath;
    }

    return details;
}

/** \brief get Search Directory/ subfolder. Used by 'new search' button in kodi apps that display search in a sub folder
 * \param url search url to retrieve
 * \param  searchTextValue search text to look for.
 * \return fileurl / path of the directory as specified by searchTextValue  */
QString Controls::getSearchDirectory(QString url, QString searchTextValue) {
    QString answer = requestUrl(getDirectoryObject(url));

    QByteArray br = answer.toUtf8();
    QJsonDocument doc = QJsonDocument::fromJson(br);
    QJsonObject addons = doc.object();
    QJsonValue addons2 = addons["result"];
    QJsonObject o = addons2.toObject();

    for (auto oIt = o.constBegin(); oIt != o.constEnd(); ++oIt) {
        QJsonArray agentsArray = oIt.value().toArray();

        foreach (const QJsonValue &v, agentsArray) {
            QString label = removeBBCode(v.toObject().value("label").toString());
            QString file = v.toObject().value("filetype").toString();
            QString fileUrl = v.toObject().value("file").toString();

            if (label.compare(searchTextValue) == 0 and file.compare("directory") == 0) {
                return fileUrl;
            }
        }
    }
    return QString("");
}

/** \brief speed to fast foward at.
 * \param speed. Can be +- 1-32 times speed of the video */
void Controls::setSpeed(int speed) {
    QJsonObject paramsObj;
    paramsObj["playerid"] = getActivePlayer();
    paramsObj["speed"] = speed;

    fetchUrlJson("Player.SetSpeed", paramsObj);
    delayMilli(100);
}

/** \brief fast foward */
void Controls::setFFWD() {
    if (FFspeed == 1) {
        FFspeed = 2;
    } else if (FFspeed < 1) {
        FFspeed = 1;
    } else if (FFspeed < 32) {
        FFspeed = FFspeed * 2;
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
        FFspeed = FFspeed * 2;
    }
    setSpeed(FFspeed);
}

/** \brief seek foward 30 seconds * the number of times the button is pressed.
 */
void Controls::seekFoward() {
    LOG(VB_GENERAL, LOG_DEBUG, "controls_seekFoward()");
    static int count = 1;
    static QTime dieTime = QTime::currentTime().addSecs(3);
    if (QTime::currentTime() > dieTime) {
        count = 1;
    }

    seek(0, 0, 30 * count);
    if (count < 8) {
        count++;
    }
}

/** \brief seek backwards -10 seconds * the number of times the button is
 * pressed. */
void Controls::seekBack() {
    LOG(VB_GENERAL, LOG_DEBUG, "controls_seekBack()");
    static int count = 1;
    static QTime dieTime = QTime::currentTime().addSecs(3);
    if (QTime::currentTime() > dieTime) {
        count = 1;
    }

    seek(0, 0, -10 * count);
    if (count < 8) {
        count++;
    }
}

void Controls::inputActionHelper(QString action) {
    QJsonObject obj;
    obj["action"] = action;
    fetchUrlJson("Input.ExecuteAction", obj);
}

/** \brief switch between Kodi and MythTV on Android
 * 	\param app name of app to switch to
 *  \return is the helper switching app (mythapp services) running? */
bool Controls::androidAppSwitch(QString app) { return netRequest->androidAppSwitch(app); }

/** \brief set the connection status
 * 	\param connectStatus the new connection status */
void Controls::setConnected(int connectStatus) { connected = connectStatus; }

/** \brief get the connection status
 *  \return 0 = not connected, 1 = connected, 2 = connected and authenticated */
int Controls::getConnected() { return connected; }

// music

/** \brief set the number of seconds for music crossfade */
void Controls::setCrossFade(int seconds) { setKodiSetting("musicplayer.crossfade", seconds); }

/** \brief is crossfade enabled? */
bool Controls::getCrossFade() { return isKodiSetting("musicplayer.crossfade", "0"); }

/** \brief set Audio Library Scan */
void Controls::setAudioLibraryScan() { fetchUrlJson("AudioLibrary.Scan"); }

/** \brief set ProjectM visualization */
void Controls::setProjectM() { setKodiSetting("musicplayer.visualisation", "visualization.projectm"); }

void Controls::playListClear() { playListClear(1); }

void Controls::playListClear(int playerid) {
    LOG(VB_GENERAL, LOG_DEBUG, "playListClear()");
    if (playerid == 0) {
        return;
    }
    QJsonObject paramsObj;
    paramsObj["playlistid"] = playerid;
    fetchUrlJson("Playlist.Clear", paramsObj);
}

void Controls::playListOpen(int position) {
    LOG(VB_GENERAL, LOG_DEBUG, "playListOpen()");

    QJsonObject obj2;
    obj2["playlistid"] = 1;
    obj2["position"] = position;

    QJsonObject paramsObj;
    paramsObj["item"] = obj2;

    fetchUrlJson("Player.Open", paramsObj);
}

void Controls::playListAdd(QString file) {
    LOG(VB_GENERAL, LOG_DEBUG, "playListAdd(): " + file);

    QJsonObject obj2;
    obj2["file"] = file;

    QJsonObject paramsObj;
    paramsObj["playlistid"] = 1;
    paramsObj["item"] = obj2;

    fetchUrlJson("Playlist.Add", paramsObj);
}

/** \brief set part mode for music */
void Controls::setPartyMode() {
    QJsonObject obj2;
    obj2["partymode"] = "music";

    QJsonObject paramsObj;
    paramsObj["item"] = obj2;

    fetchUrlJson("Player.Open", paramsObj);
}

QString Controls::playerGetItem(int playerid) {
    QJsonArray array;
    array.push_back("album");
    array.push_back("artist");
    array.push_back("thumbnail");

    QJsonObject paramsObj;
    paramsObj["playerid"] = playerid;
    paramsObj["properties"] = array;

    return fetchUrlJson("Player.GetItem", paramsObj);
}

void Controls::removeFromPlaylist(int inPlaylistPos) {
    QJsonObject paramsObj;
    paramsObj["playlistid"] = 1;
    paramsObj["position"] = inPlaylistPos;

    fetchUrlJson("Playlist.Remove", paramsObj);
}
