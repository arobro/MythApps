#ifndef MYTHKODI_H
#define MYTHKODI_H

// QT headers
#include <QApplication>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScopedPointer>

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
#include "dialog.h"
#include "fileBrowserHistory.h"
#include "imageThread.h"
#include "plugins/plugin_manager.h"
#include "programData.h"
#include "programLink.h"
#include "uiContext.h"

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
    QImage m_screenshot; /*!< used to create thumbnail for pause menu */

    ProgramData *currentSelectionDetails{nullptr}; /*!<  current selection in the file browser */
    ProgramData *lastPlayedDetails{nullptr};       /*!< last media played. used by the watch list feature */

    QTimer *exitToMainMenuSleepTimer; /*!< return to main menu after inactivty for power save. */
    QTimer *searchTimer;
    QTimer *minimizeTimer;        /*!< minimizes Kodi to the desktop every few seconds */
    QTimer *searchSuggestTimer;   /*!< micro singleshot timer to delay the search suggestions after keypress. */
    QTimer *searchFocusTimer;     /*!< micro singleshot timer to update the focus. */
    QTimer *isKodiConnectedTimer; /*!< pings Kodi to check connection is still alive */
    QTimer *nextPageTimer;        /*!< micro singleshot timer to load any next pages. Stops race conditions with multithreaded code. */
    QTimer *openSettingTimer;     /*!< micro singleshot timer to delay opening setting. */

    ProgramLink *previouslyPlayedLink{nullptr}; /*!< load previously played videos	 */
    ProgramLink *searchListLink{nullptr};       /*!< load Search Sources */

    QString username;              /*!< username for Kodi */
    QString password;              /*!< password for Kodi */
    QString ip;                    /*!< ip for Kodi */
    QString port;                  /*!< port for Kodi */
    QString currentSearchUrl = ""; /*!< the current search source url to append the search text. Used by the search box. */

    QString getPlayBackTimeString(bool adjustEnd, bool removeHoursIfNone);

    QString searchText = ""; /*!< text entered into the search box */
    QStringList searchSubFoldersList;
    QString streamDetails; /*!< stream details from the previosuly played video */

    QString searchStartLeter = "";
    QStringList showsAZfolderNames;

    bool stopScroll = false; /*!< used by next page. freezes the filebrowser scrolling */
    bool searching = false;  /*!< are we currently waiting for search results after a search?  */
    bool isHome = true;      /*!< is the file browser on the home screen? */
    bool kodiPlayerOpen = false;
    bool allowAutoMinimize = true; /*!< is auto minimizing kodi enabled? */
    bool allShowsFolderFound = false;
    bool enableHiddenFolders = false;
    QStringList azShowOnUrl;

    int nextPagePosm_apps = 0;
    int nextPagePosm_fileListGrid = 0;
    static QAtomicInteger<quint64> currentLoadId;

    void toggleSearchVisible(bool visible);
    void setSearchButtonListVisible(bool visible);
    void exitToMainMenuSleepTimerReset();
    void makeSearchTextEditEmpty();
    void showSearchTextEditBackgroundText();
    QStringList searchNoDuplicateCheck;

    void showOptionsMenu();
    void goBack();
    void refreshPage();

    void loadShowsAZ();
    void loadBackButtonIfRequired(bool m_loadBackButton);
    bool loadAZSearch(QString hash);

    void loadApps();
    void loadArtists(bool listview);
    void loadAlbums(bool listview);
    void loadGenres();
    void loadSongs(QString album);
    void loadSongsMain(QString value, QString type);

    void stopPlayBack();
    void pauseToggle();
    void toggleStreamDetails();

    void openOSD(QString screenType);
    void takeScreenshot();
    void resetScreenshot();
    void inputSendText(QString text);
    void toggleHiddenFolders();
    void toggleAutoMinimize();
    void reload();
    void refreshFileListGridSelection();

    void goFullscreen();
    void goMinimize(bool fullscreenCheck);

    void loadBackButton();
    void goSearch(QString overrideCurrentSearchUrl);
    void delayWhileStopScroll(int maxDelay);

    void loadProgram(QString name, QString setdata, QString thumbnailUrl);
    void loadProgram(QString name, QString setdata, QString thumbnailUrl, MythUIButtonList *mythUIButtonList);
    void play_Kodi(QString mediaLocation, QString seekAmount);
    void play(QString mediaLocation, QString seekAmount = "");

    void ReplyFinishedFileBrowser(QNetworkReply *reply);
    void requestFileBrowser(QString url, QStringList previousSearches, bool loadBackButton, QString itemData);
    bool appsCallbackPlugins(QScopedPointer<ProgramData> &programData, QString label, QString data);
    void appsCallback(QString label, QString data, bool allowBack = true);
    void waitforRequests();
    void downloadImage(QString thumbnailUrl);
    void displayImage(MythUIButtonListItem *item, MythUIButtonList *m_fileList);
    void displayFileBrowser(QString answer, QStringList previousSearchTerms, bool m_loadBackButton);
    void handleDialogs(bool forceFullScreenVideo);
    void handleSettingsDialogs(QNetworkReply *reply);

    void discoverSearchURLs(QString label, QString file);

    void fetchSearch(QString searchUrl);
    void niceClose(bool forceClose);
    bool folderAllowed(const QString &label, const QStringList &previousSearchTerms);
    QString getLabel(MythUIButtonListItem *item);

    QString getNewSearch(QString url);

    void loadImage(MythUIButtonList *mythUIButtonList, QString name, QString setdata, QString thumbnailUrl);
    void createPlayBackMenu();
    void displayInputBox(QString jsonMessage);
    QVariantMap playBackTimeMap;
    QString getStandardizedImagePath(QString imagePath);
    void loadPlugins(bool start);
    void loadPluginManager();

    MythDialogBox *m_menuPopup{nullptr};
    MythUIButtonListItem *nextPageitem{nullptr};

    QScreen *screen{nullptr};
    Controls *controls{nullptr};

    QString back_icon;      /*!< stores physical image location for the corresponding button */
    QString ma_tv_icon;     /*!< stores physical image location for the corresponding button */
    QString ma_search_icon; /*!< stores physical image location for the corresponding button */

    void setButtonWatched(bool watched);
    void addToPreviouslyPlayed();
    void coolDown();

    Browser *browser;
    FileBrowserHistory *fileBrowserHistory;
    Dialog *dialog;

    void handlePlaybackEvent(const QString &method, const QString &message);
    PlaybackTime getKodiPlaybackTimeMs();
    qint64 getCurrentPlaybackTimeMs() const;
    int getPlaybackPercentage() const;

    qint64 playbackStartMs_ = -1;
    qint64 manualAccumulatedMs_ = 0;
    qint64 lastKnownPlaybackMs_ = 0;

    UIContext *uiCtx;
    PlaybackInfo getPlaybackInfo();

    void CleanupResources();

  Q_SIGNALS:
    void closedWS();

  private Q_SLOTS:
    void onTextMessageReceived(const QString &method, const QString &message);

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

    void exitToMainMenuSleepTimerClose();
    void minimizeTimerSlot();
    void searchTimerSlot();
    void searchSuggestTimerSlot();
    void searchFocusTimerSlot();
    void isKodiConnectedSlot();
    void nextPageTimerSlot();

    void loadProgramSlot(QString name, QString setdata, QString thumbnailUrl);

  public slots:
    void handleImageSlot(int, const QString, MythUIButtonList *);
    void setFocusWidgetSlot(QString);

  private:
    QNetworkAccessManager *managerFileBrowser;
    QNetworkRequest request;

    MythUIImage *m_thumbnailImage{nullptr};
    MythUIButtonList *m_searchSettingsButtonList{nullptr};

    QString firstDirectoryName;

    static PluginManager *pluginManager;
};

#endif /* MYTHKODI_H */
