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
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTime>
#include <QTimer>
#include <QUrlQuery>

// MythApps headers
#include "netSocketRequest.h"
#include "shared.h"

// kodi events client
#include "libs/xbmcclient.h"
#include <sys/socket.h>

/** \class Controls
 *  \brief Wraper between Myth Apps and the Kodi json api to control Kodi - https://kodi.wiki/view/JSON-RPC_API
 */
class Controls : public QObject {
    Q_OBJECT

  public:
    explicit Controls(const QString &ip, const QString &port, QObject *parent = nullptr);
    ~Controls();

    void startKodiIfNotRunning();
    bool isKodiSetting(const QString &SettingName, const QString &SettingValue);
    template <typename T> void setKodiSetting(const QString &SettingName, const T &SettingValue);
    void seek(int hours, int minutes, int seconds);

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
    bool ping();

    bool isFullscreen();

    bool isInputAdaptive();
    bool areAddonsInstalled();
    QString getAddons(bool forceRefresh = false);
    void loadAddons();

    bool isVirtualKeyboardOpen();
    QJsonObject getDirectoryObject(QString url);
    QVariantMap getPlayBackTime(int playerid);

    QString handleDialogs();
    void play(const QString &mediaLocation, const QString &seekAmount);

    void pauseToggle(int activePlayerStatus);
    bool isPlaying();
    QVariantMap getVideos();

    void inputSendText(const QString &text);
    QString getStreamDetails(int playerId);

    QString getStreamDetailsAll(int playerId);

    void seekFoward();
    void seekBack();
    void setSpeed(int speed);
    void setFFWD();
    void setRWND();
    void inputActionHelper(QString action);
    bool androidAppSwitch(QString app);
    void toggleFullscreen();

    void setConnected(int connectStatus);
    int getConnected();
    bool isVideoNearEnd();
    void activateFullscreenVideo();
    void waitUntilKodiPingable();
    void initializeWebSocket();

    QString getLocationFromUrlAddress(QString urlAddress);
    QScopedPointer<NetSocketRequest> netSocketRequest;

    QJsonValue callJsonRpc(const QString &method, const QJsonObject &params = QJsonObject(), const QJsonArray &props = QJsonArray());
    QString callJsonRpcString(const QString &method, const QJsonObject &params = QJsonObject(), const QJsonArray &props = QJsonArray());

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
    void ensureEventClient();
    void sendEventAction(const QString &action);
    void queueSeek(int seconds);
    bool isInternalVolEnabled();

    QString globalDuration;
    int FFspeed = 1;
    int connected = 0; /*!< is kodi connected? 0 = not connected, 1 = connected, 2 = connected and authenticated*/
    QString ip, port;

    CAddress eventClientIpAddress;
    int sockfd;
    bool eventClientConnected = false;
    bool videoNearEnd = false;
    QMap<QString, QString> urlToThumbnailMap;

    QTimer seekTimer;
    int pendingSeekSeconds = 0;

  signals:
    void loadProgramSignal(QString name, QString setdata, QString thumbnailPath);
};
#endif
