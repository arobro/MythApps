#ifndef MYTHKODI_H
#define MYTHKODI_H

// QT headers
#include <QApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

// MythTV headers
#include <libmythui/mythscreentype.h>
#include <libmythui/mythuibutton.h>
#include <libmythui/mythuibuttonlist.h>
#include <libmythui/mythuiprogressbar.h>
#include <libmythui/mythuishape.h>

// MythApps headers
#include "browser.h"
#include "container.h"
#include "controls.h"
#include "fileBrowserHistory.h"
#include "imageThread.h"
#include "plugins/plugin_manager.h"
#include "programData.h"
#include "programLink.h"
#include "ytCustomApp.h"

class MythDialogBox;

/** \class MythApps
 *  \brief Main class for mythapp.
 *  Supports Linux, Windows (gcc cross compile), Android and Android TV (clang) */
class MythApps : public MythScreenType {
    Q_OBJECT

  public:
    MythApps(MythScreenStack *parent, QString name);
    ~MythApps();
    bool Create() override;
    bool keyPressEvent(QKeyEvent *) override;
    void customEvent(QEvent *event) override;

  private:
    QWebSocket m_webSocket; /*!< used for messages recieved from Kodi via the websocket. */
    MythUIText *m_filepath;
    MythUIText *m_plot;
    MythUIText *m_streamDetails;
    MythUIShape *m_streamDetailsbackground;
    MythUIText *m_title;
    MythUITextEdit *m_SearchTextEdit;
    MythUIText *m_SearchTextEditBackgroundText;
    MythUIImage *m_screenshotMainMythImage; /*!< used to create thumbnail for pause menu */
    QImage m_screenshotMainQimage;          /*!< used to create thumbnail for pause menu */
    MythUIImage *m_loaderImage;
    MythUIType *m_searchButtonListGroup;
    MythUIType *m_searchSettingsGroup;
    MythUIType *m_help;
    MythUIButton *m_androidMenuBtn;

    ProgramData *currentselectionDetails; /*!<  current selection in the file browser */
    ProgramData *lastPlayedDetails;       /*!< last media played. used by the watch list feature */

    QTimer *exitToMainMenuSleepTimer; /*!< return to main menu after inactivty for power save. */
    QTimer *searchTimer;
    QTimer *minimizeTimer;        /*!< minimizes Kodi to the desktop every few seconds */
    QTimer *playbackTimer;        /*!< update the playback status in the music app */
    QTimer *hintTimer;            /*!< stops any hung searchs */
    QTimer *musicBarOnStopTimer;  /*!< update the music bar status */
    QTimer *searchSuggestTimer;   /*!< micro singleshot timer to delay the search suggestions after keypress. */
    QTimer *searchFocusTimer;     /*!< micro singleshot timer to update the focus. */
    QTimer *isKodiConnectedTimer; /*!< pings Kodi to check connection is still alive */
    QTimer *nextPageTimer;        /*!< micro singleshot timer to load any next pages. Stops race conditions with multithreaded code. */
    QTimer *openSettingTimer;     /*!< micro singleshot timer to delay opening setting. */

    ProgramLink *previouslyPlayedLink; /*!< load previously played videos	 */
    ProgramLink *favLink;              /*!< load Favourites */
    ProgramLink *watchedLink;          /*!< load Watched List */
    ProgramLink *searchListLink;       /*!< load Search Sources */

    QString username;              /*!< username for Kodi */
    QString password;              /*!< password for Kodi */
    QString ip;                    /*!< ip for Kodi */
    QString port;                  /*!< port for Kodi */
    QString currentSearchUrl = ""; /*!< the current search source url to append the search text. Used by the search box. */

    QList<QThread *> imageThreadList; /*!< thread pool */
    QString getPlayBackTimeString(bool adjustEnd);

    QString searchText = ""; /*!< text entered into the search box */
    QStringList searchSubFoldersList;
    QString streamDetails; /*!< stream details from the previosuly played video */

    QString searchStartLeter = "";
    QStringList showsAZfolderNames;

    bool stopScroll = false; /*!< used by next page. freezes the filebrowser scrolling */
    bool searching = false;  /*!< are we currently waiting for search results after a search?  */
    bool isHome = true;      /*!< is the file browser on the home screen? */
    bool kodiPlayerOpen = false;
    bool musicOpen = false;        /*!< is the music player open? */
    bool ytNativeOpen = false;     /*!< is ytNative open? */
    bool allowAutoMinimize = true; /*!< is auto minimizing kodi enabled? */
    bool isPreviouslyPlayed = false;
    bool allShowsFolderFound = false;
    bool enableHiddenFolders = false;
    QStringList azShowOnUrl;

    int nextPagePosm_apps = 0;
    int nextPagePosm_fileListGrid = 0;
    int globalActivePlayer = 0; /*!< the id of the active player in kodi */

    void toggleSearchVisible(bool visible);
    void setSearchButtonListVisible(bool visible);
    void exitToMainMenuSleepTimerReset();
    void makeSearchTextEditEmpty();
    void showSearchTextEditBackgroundText();
    QStringList searchNoDuplicateCheck;

    void showOptionsMenu();
    void goBack();
    void createAutoClosingBusyDialog(QString dialogText, int delaySeconds, bool wait = true);
    void refreshPage(bool enableDialog);

    void loadWatched(bool unwatched);
    void loadVideos();
    void loadYTNative(QString searchString, QString directory);
    void loadShowsAZ();
    void loadBackButtonIfRequired(bool m_loadBackButton);
    bool loadAZSearch(QString hash);

    void loadApps();
    void loadMusic();
    void loadArtists(bool listview);
    void loadAlbums(bool listview);
    void loadGenres();
    void loadSongs(QString album);
    void loadSongsMain(QString value, QString type);

    int stopPlayBack();
    void pauseToggle();

    void openOSD(QString screenType);
    bool takeScreenshot();
    void resetScreenshot();
    void inputSendText(QString text);
    void toggleHiddenFolders();
    void toggleAutoMinimize();
    void reload();
    void refreshFileListGridSelection();
    void addToUnWatchedList(bool menu);

    void toggleFullscreen();
    void goFullscreen();
    void goMinimize(bool fullscreenCheck);
    int isPlaying(int retryNo);

    void loadBackButton();
    void goSearch(QString overrideCurrentSearchUrl);
    void delayWhileStopScroll(int maxDelay);

    void loadProgram(QString name, QString setdata, QString thumbnailPath);
    void loadProgram(QString name, QString setdata, QString thumbnailPath, MythUIButtonList *mythUIButtonList);
    void play_Kodi(QString mediaLocation, QString seekAmount);
    void play(QString mediaLocation, QString seekAmount = "");

    void ReplyFinishedFileBrowser(QNetworkReply *reply);
    void requestFileBrowser(QString url, QStringList previousSearches, bool loadBackButton, QString itemData);
    bool appsCallbackPlugins(ProgramData *programData, QString label, QString data);
    void appsCallback(QString label, QString data, bool allowBack = true);
    void waitforRequests();
    void downloadImage(QString thumbnailPath);
    void displayImage(MythUIButtonListItem *item, MythUIButtonList *m_fileList);
    void displayFileBrowser(QString answer, QStringList previousSearchTerms, bool m_loadBackButton);
    void handleDialogs(bool forceFullScreenVideo);
    void handleSettingsDialogs(QNetworkReply *reply);

    void discoverSearchURLs(QString label, QString file);

    void fetchSearch(QString searchUrl);
    void niceClose(bool forceClose);
    int getThreadCount();
    void waitForThreads(int maxThreadsRunning);
    bool folderAllowed(QString label, QStringList previousSearchTerms);
    QString getLabel(MythUIButtonListItem *item);

    QString getNewSearch(QString url);

    void loadImage(MythUIButtonList *mythUIButtonList, QString name, QString setdata, QString thumbnailPath);
    void createPlayBackMenu();
    void displayInputBox(QString jsonMessage);
    QVariantMap playBackTimeMap;
    QString getStandarizedImagePath(QString imagePath);

    MythDialogBox *m_menuPopup{nullptr};
    MythUIBusyDialog *m_busyPopup{nullptr};
    MythUIButtonListItem *nextPageitem;

    QScreen *screen;
    Controls *controls;

    QString mm_albums_icon;    /*!< stores physical image location for the corresponding button */
    QString mm_alltracks_icon; /*!< stores physical image location for the corresponding button */
    QString mm_artists_icon;   /*!< stores physical image location for the corresponding button */
    QString mm_genres_icon;    /*!< stores physical image location for the corresponding button */
    QString mm_playlist_icon;  /*!< stores physical image location for the corresponding button */
    QString recent_icon;       /*!< stores physical image location for the corresponding button */
    QString videos_icon;       /*!< stores physical image location for the corresponding button */
    QString music_icon;        /*!< stores physical image location for the corresponding button */
    QString back_icon;         /*!< stores physical image location for the corresponding button */
    QString ma_tv_icon;        /*!< stores physical image location for the corresponding button */
    QString ma_popular_icon;   /*!< stores physical image location for the corresponding button */
    QString ma_search_icon;    /*!< stores physical image location for the corresponding button */

    void confirmDialog(QString description, QString type);
    void returnFocus();
    void setButtonWatched(bool watched);
    void addToPreviouslyPlayed();

    Browser *browser;
    FileBrowserHistory *fileBrowserHistory;

    // music app
    int m_currentMusicButton = 0;

    MythUIType *m_musicDetailsUIGroup;

    MythUIText *m_textSong;
    MythUIText *m_textArtist;
    MythUIText *m_textAlbum;
    MythUIText *m_musicDuration;
    MythUIText *m_hint; /*!< hint used in music app */
    MythUIShape *m_seekbar;

    MythUIText *m_musicTitle;
    MythUIProgressBar *m_trackProgress;
    MythUIImage *m_coverart;
    MythUIImage *m_blackhole_border;
    MythUIImage *m_playingOn;
    MythUIImage *m_next_buttonOn;
    MythUIImage *m_ff_buttonOn;
    MythUIImage *m_playingOff;
    MythUIImage *m_next_buttonOff;
    MythUIImage *m_ff_buttonOff;

    int musicMode; /*!< 1 for music, 2 for video in the music app */

    QMap<QString, QStringList> getMusicHelper(QString methodValue, QString type, QString filterKey, QString filterValue, QString operatorValue);
    QMap<QString, QStringList> getByAlbums(QString artist);
    QMap<QString, QStringList> getByAlbumsSearch(QString s);
    QMap<QString, QStringList> getByAlbumsWithGenre(QString genres);
    QMap<QString, QStringList> getByArtist(QString artist);
    QMap<QString, QStringList> getByGenres();
    QStringList addSpacingToList(QMap<QString, QStringList> map, bool listview);
    QMap<QString, QStringList> filterQMap(QMap<QString, QStringList> map, QString filterKey);

    void updateMusicPlayingBarStatus();
    void showMusicPlayingBar(bool show);
    void showMusicUI(bool show);

    void refreshGuiPlaylist();
    int isInPlaylist(QString label);
    void removeFromPlaylist(QString label);
    bool isMusicPlayingBarVisible();

    void previousTrack();
    void nextTrack();
    void setPartyMode();

    bool partyMode;
    bool initializeMusic();
    void clearAndStopPlaylist();
    void closeBusyDialog();

    void musicSearch(QString search);
    void loadMusicHelper(QString labelPrefix, QMap<QString, QStringList> loadMusicType, MythUIButtonList *m_fileList);
    void loadMusicSetup();
    void loadPlaylists();
    QVariantMap getPlayBackTime(int playerid); /*!< used in music app for the songs playing time */

  Q_SIGNALS:
    void closedWS();

  private Q_SLOTS:
    void onConnectedWS();
    void onTextMessageReceived(QString message);

  private slots:
    void appsCallback(MythUIButtonListItem *item);
    void selectAppsCallback(MythUIButtonListItem *item);
    void visibleAppsCallback(MythUIButtonListItem *item);
    void addToPlaylistClickedCallback(MythUIButtonListItem *item);
    void selectSearchList(MythUIButtonListItem *item);
    void clickedSearchList(MythUIButtonListItem *item);
    void searchSettingsClicked(MythUIButtonListItem *item);
    void searchTextEditValueChanged();
    void searchTextEditLosingFocus();
    void runMythSettingsSlot();
    void androidMenuBtnSlot();

    void fileListMusicGridClickedCallback(MythUIButtonListItem *item);
    void fileListMusicGridSelectedCallback(MythUIButtonListItem *item);
    void fileListMusicGridVisibleCallback(MythUIButtonListItem *item);
    void filterGridClickedCallback(MythUIButtonListItem *item);
    void filterGridSelectedCallback(MythUIButtonListItem *item);
    void filterOptionsListClickedCallback(MythUIButtonListItem *item);

    void exitToMainMenuSleepTimerClose();
    void minimizeTimerSlot();
    void searchTimerSlot();
    void playbackTimerSlot();
    void hintTimerSlot();
    void musicBarOnStopTimerSlot();
    void searchSuggestTimerSlot();
    void searchFocusTimerSlot();
    void isKodiConnectedSlot();
    void nextPageTimerSlot();

    void loadProgramSlot(QString name, QString setdata, QString thumbnailPath);

  public slots:
    void handleImageSlot(int, MythUIButtonList *);
    void setFocusWidgetSlot(QString);

  protected:
    void createBusyDialog(const QString &title);

  private:
    QNetworkAccessManager *managerFileBrowser;
    QNetworkRequest request;

    MythUIImage *m_thumbnailImage{nullptr};
    MythUIButtonList *m_fileListGrid;
    MythUIButtonList *m_searchButtonList;
    MythUIButtonList *m_searchSettingsButtonList;

    MythUIButtonList *m_fileListMusicGrid;
    MythUIButtonList *m_fileListSongs;
    MythUIButtonList *m_filterGrid;
    MythUIButtonList *m_filterOptionsList;
    MythUIButtonList *m_playlistVertical;

    ytCustomApp *ytNative;
    bool ytNativeEnabled = false;
    QString firstDirectoryName;

    static PluginManager pluginManager;
};

#endif /* MYTHKODI_H */
