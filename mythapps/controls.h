#ifndef Controls_h
#define Controls_h

// QT headers
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QNetworkAccessManager>
#include <QString>
#include <QStringList>
#include <QTime>
#include <QUrlQuery>

// MythApps headers
#include "netRequest.h"
#include "shared.h"

// kodi events client
#include "libs/xbmcclient.h"
#include <sys/socket.h>

/** \class Controls
 *  \brief Wraper between Myth Apps and the Kodi json api to control Kodi - https://kodi.wiki/view/JSON-RPC_API
 */
class Controls {
  public:
    Controls(QString m_username, QString m_password, QString m_ip, QString m_port);
    ~Controls();

    void startKodiIfNotRunning();
    bool isKodiSetting(QString SettingName, QString SettingValue);
    template <typename T> void setKodiSetting(QString SettingName, T SettingValue);
    QString fetchUrlJson(QString method, QJsonObject paramsObj = QJsonObject(), QJsonArray property = QJsonArray());
    void seek(int hours, int minutes, int seconds);
    void seek(QString seekAmount);

    void showOSD();
    void togglePlayerDebug(bool doubleclick);
    void goMinimize();
    void quitKodi();

    void showPlayerProcessInfo();
    void showInfo();
    void activateWindow(QString window);
    QString getSearchDirectory(QString url, QString searchTextValue);

    void setMute();
    void increaseVol();
    void decreaseVol();
    int getVol();
    void inputBack();
    bool isPaused(int playerid);
    int stopPlayBack();

    int getActivePlayer();
    bool isUserNamePasswordCorrect();

    QString isFullscreen();
    bool isFullscreenBool();

    bool isInputAdaptive();
    bool areAddonsInstalled();
    QString getAddons();

    bool isVirtualKeyboardOpen();
    QJsonObject getDirectoryObject(QString url);

    QString requestUrl(QJsonObject value);
    QVariantMap getPlayBackTime(int playerid);

    QString handleDialogs();
    void play(QString mediaLocation);

    void pauseToggle(int activePlayerStatus);
    QString isPlaying();
    QVariantMap getVideos();

    void inputSendText(QString text);
    QString getStreamDetails(int playerId);

    QString getStreamDetailsAll(int playerId);

    void seekFoward();
    void seekBack();
    void setSpeed(int speed);
    void setFFWD();
    void setRWND();
    void inputActionHelper(QString action);
    bool androidAppSwitch(QString app);

    void setConnected(int connectStatus);
    int getConnected();
    bool isVideoNearEnd();

    // music
    void setCrossFade(int seconds);
    void setProjectM();
    bool getCrossFade();
    void setAudioLibraryScan();
    void playListClear();
    void playListClear(int playerid);
    void playListOpen(int position);
    void playListAdd(QString file);
    void setPartyMode();
    QString playerGetItem(int playerid);
    void removeFromPlaylist(int inPlaylistPos);

  private:
    void checkEventClientConnected();
    NetRequest *netRequest;
    QString globalDuration;
    int FFspeed = 1;
    int connected = 0; /*!< is kodi connected? 0 = not connected, 1 = connected, 2 = connected and authenticated*/

    CAddress eventClientIpAddress;
    int sockfd;
    bool eventClientConnected = false;
    bool videoNearEnd = false;
};
#endif
