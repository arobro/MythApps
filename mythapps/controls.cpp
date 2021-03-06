#include "controls.h"
#include <mythcontext.h>
#include <mythdirs.h>

/** \param m_username Kodi username
 *  \param m_password Kodi password
 *  \param m_ip Kodi ip
 * 	\param m_port Kodi port */
Controls::Controls(QString m_username, QString m_password, QString m_ip, QString m_port) { netRequest = new NetRequest(m_username, m_password, m_ip, m_port, false); }
Controls::~Controls() { delete netRequest; }

/** \brief helper function for netRequest->requestUrl() */
QString Controls::requestUrl(QJsonObject value) { return netRequest->requestUrl(value); }

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
    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "Addons.GetAddons";

    QJsonObject paramsObj;
    paramsObj["type"] = "xbmc.addon.video";
    paramsObj["enabled"] = true;
    jsonObj["params"] = paramsObj;

    return requestUrl(jsonObj);
}

/** \brief  Checks if input adative addon installed. This is used by Kodi to
 * play DRM video \return Is the input adative addon installed?*/
bool Controls::isInputAdaptive() {
    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "Addons.GetAddons";

    QJsonObject paramsObj;
    paramsObj["type"] = "kodi.inputstream";
    jsonObj["params"] = paramsObj;

    QString answer = requestUrl(jsonObj);

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

    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "Player.Seek";

    QJsonObject obj3;
    obj3["seconds"] = time;

    QJsonObject paramsObj;
    paramsObj["playerid"] = getActivePlayer();
    paramsObj["value"] = obj3;

    jsonObj["params"] = paramsObj;
    requestUrl(jsonObj);
}

/** \brief active a window in Kodi
 *  \param window- name of window to active */
void Controls::activateWindow(QString window) {
    LOG(VB_GENERAL, LOG_DEBUG, "controls_activateSplash()");
    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "GUI.ActivateWindow";

    QJsonObject paramsObj;
    paramsObj["window"] = window;

    jsonObj["params"] = paramsObj;
    requestUrl(jsonObj);
}

/** \brief show the on screen display */
void Controls::showOSD() {
    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "Input.ShowOSD";
    requestUrl(jsonObj);
}

/** \brief show video player info */
void Controls::showPlayerProcessInfo() {
    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "Input.ShowPlayerProcessInfo";
    requestUrl(jsonObj);
}

/** \brief show video info in Kodi */
void Controls::showInfo() {
    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "Input.Info";
    requestUrl(jsonObj);
}

/** \brief get the volume
 *  \return volume level */
int Controls::getVol() {
    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "Application.SetVolume";

    QJsonArray array;
    array.push_back("volume");

    QJsonObject paramsObj;
    paramsObj["properties"] = array;
    jsonObj["params"] = paramsObj;

    return requestUrl(jsonObj).toInt();
}

/** \brief close dialog if one pops up. e.g. for a subscription */
bool Controls::isVirtualKeyboardOpen() {
    LOG(VB_GENERAL, LOG_DEBUG, "isVirtualKeyboardOpen()");
    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "XBMC.GetInfoLabels";

    QJsonArray array;
    array.push_back("System.CurrentWindow");
    QJsonObject paramsObj;
    paramsObj["labels"] = array;
    jsonObj["params"] = paramsObj;
    QString s = requestUrl(jsonObj);

    QJsonDocument jsonDocument = QJsonDocument::fromJson(s.toLocal8Bit().data());
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
void Controls::inputBack() {
    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "Input.Back";
    requestUrl(jsonObj);
}

/** \brief mute the playing media */
void Controls::setMute() {
    if (!gCoreContext->GetSetting("MythAppsInternalMute").compare("1") == 0) {
        return; // internal mute disabled
    }

    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "Application.SetMute";

    QJsonObject paramsObj;
    paramsObj["mute"] = "toggle";
    jsonObj["params"] = paramsObj;

    requestUrl(jsonObj);
}

/** \brief increase the volume if enabled */
void Controls::increaseVol() {
    if (!gCoreContext->GetSetting("MythAppsInternalVol").compare("1") == 0) {
        return; // internal volume disabled
    }

    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "Application.SetVolume";

    QJsonObject paramsObj;
    paramsObj["volume"] = getVol() + 5;
    jsonObj["params"] = paramsObj;

    requestUrl(jsonObj);
}

/** \brief decrease the volume if enabled */
void Controls::decreaseVol() {
    if (!gCoreContext->GetSetting("MythAppsInternalVol").compare("1") == 0) {
        return; // internal volume disabled
    }

    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "Application.SetVolume";

    QJsonObject paramsObj;
    paramsObj["volume"] = getVol() - 5;
    jsonObj["params"] = paramsObj;

    requestUrl(jsonObj);
}

/** \brief is the video paused?
 *  \param playerid Kodi player id*/
bool Controls::isPaused(int playerid) {
    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "Player.GetProperties";

    QJsonArray array;
    array.push_back("speed");

    QJsonObject paramsObj;
    paramsObj["playerid"] = playerid;
    paramsObj["properties"] = array;

    jsonObj["params"] = paramsObj;

    QString s = requestUrl(jsonObj);

    // parse json
    QJsonDocument jsonDocument = QJsonDocument::fromJson(s.toLocal8Bit().data());
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

    // fetch the json response
    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "Player.GetActivePlayers";
    jsonObj["id"] = "1";
    QString answer = requestUrl(jsonObj);

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

    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "Player.Stop";

    QJsonObject paramsObj;
    paramsObj["playerid"] = activePlayer;
    jsonObj["params"] = paramsObj;

    requestUrl(jsonObj);
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
    QJsonObject objB;
    objB["id"] = "1";
    objB["jsonrpc"] = "2.0";
    objB["method"] = "Settings.GetSettingValue";
    QJsonObject paramsObjB;
    paramsObjB["setting"] = "videoscreen.screen";
    objB["params"] = paramsObjB;
    QString windowMode = requestUrl(objB);

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
bool Controls::isUserNamePasswordCorrect() {
    QJsonObject objB;
    objB["id"] = "1";
    objB["jsonrpc"] = "2.0";
    objB["method"] = "Settings.GetSettingValue";
    QJsonObject paramsObjB;
    paramsObjB["setting"] = "videoscreen.screen";
    objB["params"] = paramsObjB;
    QString windowMode = requestUrl(objB);

    if (windowMode.compare("Host requires authentication") == 0) {
        return false;
    }
    return true;
}

/** \brief get the media currently playing time?
 *  \param playerid Kodi player id
 *  \return playback/total time*/
QVariantMap Controls::getPlayBackTime(int playerid) {
    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "Player.GetProperties";

    QJsonArray array;
    array.push_back("time");
    array.push_back("totaltime");
    array.push_back("percentage");

    QJsonObject paramsObj;
    paramsObj["playerid"] = playerid;
    paramsObj["properties"] = array;

    jsonObj["params"] = paramsObj;

    QString answer = requestUrl(jsonObj);

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
    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "XBMC.GetInfoLabels";

    QJsonArray array;
    array.push_back("System.CurrentWindow");
    QJsonObject paramsObj;
    paramsObj["labels"] = array;
    jsonObj["params"] = paramsObj;
    QString s = requestUrl(jsonObj);

    QJsonDocument jsonDocument = QJsonDocument::fromJson(s.toLocal8Bit().data());
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
    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "Player.Open";
    QJsonObject obj3;
    obj3["file"] = mediaLocation.trimmed();
    QJsonObject paramsObj;
    paramsObj["item"] = obj3;
    jsonObj["params"] = paramsObj;

    requestUrl(jsonObj);
}

/** \brief toggle pause the playing media */
void Controls::pauseToggle(int activePlayerStatus) {
    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "Player.PlayPause";

    QJsonObject paramsObj;
    paramsObj["playerid"] = activePlayerStatus;
    jsonObj["params"] = paramsObj;

    requestUrl(jsonObj);
}

/** \brief used to determine if media is playing */
QString Controls::isPlaying() {
    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "Player.GetActivePlayers";
    return requestUrl(jsonObj);
}

/** \brief get videos on the file system in Kodi */
QVariantMap Controls::getVideos() {
    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "Files.GetSources";

    QJsonObject paramsObj;
    paramsObj["media"] = "video";
    jsonObj["params"] = paramsObj;

    QString answer = requestUrl(jsonObj);

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
    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "Player.GetItem";

    QJsonArray array;
    array.push_back("dynpath");
    array.push_back("streamdetails");

    QJsonObject paramsObj;
    paramsObj["playerid"] = playerId;
    paramsObj["properties"] = array;

    jsonObj["params"] = paramsObj;

    return requestUrl(jsonObj);
}

/** \brief format stream details to a string. Codec, Resolution etc */
QString Controls::getStreamDetailsAll(int playerId) {
    LOG(VB_GENERAL, LOG_DEBUG, "getStreamDetails()");
    QString answer = getStreamDetails(playerId);

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
    QJsonObject jsonObj;
    jsonObj["id"] = "1";
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "Player.SetSpeed";

    QJsonObject paramsObj;
    paramsObj["playerid"] = getActivePlayer();
    paramsObj["speed"] = speed;

    jsonObj["params"] = paramsObj;
    requestUrl(jsonObj);
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
    obj["id"] = "1";
    obj["jsonrpc"] = "2.0";
    obj["method"] = "Input.ExecuteAction";
    QJsonObject obj2;
    obj2["action"] = action;
    obj["params"] = obj2;
    requestUrl(obj);
}

// music

/** \brief set the number of seconds for music crossfade */
void Controls::setCrossFade(int seconds) {
    QJsonObject obj;
    obj["id"] = "1";
    obj["jsonrpc"] = "2.0";
    obj["method"] = "Settings.SetSettingValue";

    QJsonObject obj2;
    obj2["setting"] = "musicplayer.crossfade";
    obj2["value"] = seconds;
    obj["params"] = obj2;
    requestUrl(obj);
}

/** \brief is crossfade enabled? */
bool Controls::getCrossFade() {
    QJsonObject obj;
    obj["id"] = "1";
    obj["jsonrpc"] = "2.0";
    obj["method"] = "Settings.GetSettingValue";

    QJsonObject obj2;
    obj2["setting"] = "musicplayer.crossfade";
    obj["params"] = obj2;
    QString s = requestUrl(obj);

    QJsonDocument jsonDocument = QJsonDocument::fromJson(s.toLocal8Bit().data());
    QJsonObject jObject = jsonDocument.object();
    QVariantMap mainMap = jObject.toVariantMap();
    QVariantMap map2 = mainMap["result"].toMap();

    if (map2["value"].toString().compare("0") == 0) {
        return false;
    }
    return true;
}

/** \brief set Audio Library Scan */
void Controls::setAudioLibraryScan() {
    QJsonObject obj;
    obj["id"] = "1";
    obj["jsonrpc"] = "2.0";
    obj["method"] = "AudioLibrary.Scan";
    requestUrl(obj);
}

/** \brief set ProjectM visualization */
void Controls::setProjectM() {
    QJsonObject obj;
    obj["id"] = "1";
    obj["jsonrpc"] = "2.0";
    obj["method"] = "Settings.SetSettingValue";

    QJsonObject obj2;
    obj2["setting"] = "musicplayer.visualisation";
    obj2["value"] = "visualization.projectm";
    obj["params"] = obj2;
    requestUrl(obj);
}

void Controls::playListClear() { playListClear(1); }

void Controls::playListClear(int playerid) {
    LOG(VB_GENERAL, LOG_DEBUG, "playListClear()");
    if (playerid == 0) {
        return;
    }
    QJsonObject obj;
    obj["id"] = "1";
    obj["jsonrpc"] = "2.0";
    obj["method"] = "Playlist.Clear";

    QJsonObject obj2;
    obj2["playlistid"] = playerid;
    obj["params"] = obj2;
    requestUrl(obj);
}

void Controls::playListOpen(int position) {
    LOG(VB_GENERAL, LOG_DEBUG, "playListOpen()");
    QJsonObject obj;
    obj["id"] = "1";
    obj["jsonrpc"] = "2.0";
    obj["method"] = "Player.Open";

    QJsonObject obj3;
    obj3["playlistid"] = 1;
    obj3["position"] = position;

    QJsonObject obj2;
    obj2["item"] = obj3;

    obj["params"] = obj2;
    requestUrl(obj);
}

void Controls::playListAdd(QString file) {
    LOG(VB_GENERAL, LOG_DEBUG, "playListAdd(): " + file);

    QJsonObject obj;
    obj["id"] = "1";
    obj["jsonrpc"] = "2.0";
    obj["method"] = "Playlist.Add";

    QJsonObject obj3;
    obj3["file"] = file;

    QJsonObject obj2;
    obj2["playlistid"] = 1;
    obj2["item"] = obj3;

    obj["params"] = obj2;
    requestUrl(obj);
}

/** \brief set part mode for music */
void Controls::setPartyMode() {
    QJsonObject obj;
    obj["id"] = "1";
    obj["jsonrpc"] = "2.0";
    obj["method"] = "Player.Open";

    QJsonObject obj3;
    obj3["partymode"] = "music";
    QJsonObject obj2;
    obj2["item"] = obj3;
    obj["params"] = obj2;
    requestUrl(obj);
}

QString Controls::playerGetItem(int playerid) {
    QJsonObject obj;
    obj["id"] = "1";
    obj["jsonrpc"] = "2.0";
    obj["method"] = "Player.GetItem";

    QJsonArray array;
    array.push_back("album");
    array.push_back("artist");
    array.push_back("thumbnail");

    QJsonObject obj2;
    obj2["playerid"] = playerid;
    obj2["properties"] = array;

    obj["params"] = obj2;

    return requestUrl(obj);
}

void Controls::removeFromPlaylist(int inPlaylistPos) {
    QJsonObject obj;
    obj["id"] = "1";
    obj["jsonrpc"] = "2.0";
    obj["method"] = "Playlist.Remove";
    QJsonObject obj2;
    obj2["playlistid"] = 1;
    obj2["position"] = inPlaylistPos;

    obj["params"] = obj2;
    requestUrl(obj);
}

/** \brief switch between Kodi and MythTV on Android
 * 	\param app name of app to switch to
 *  \return is the helper switching app (mythapp services) running? */
bool Controls::androidAppSwitch(QString app) { return netRequest->androidAppSwitch(app); }
