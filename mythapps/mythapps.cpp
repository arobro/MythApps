// MythTV headers
#include "mythuistatetracker.h"
#include <mythcontext.h>
#include <mythdialogbox.h>
#include <mythdirs.h>
#include <mythmainwindow.h>
#include <mythprogressdialog.h>
#include <mythuibutton.h>
#include <mythuiimage.h>
#include <mythuitext.h>

// QT headers
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QTextStream>
#include <QtWidgets>

// MythApps headers
#include "imageThread.h"
#include "music_functions.cpp"
#include "mythapps.h"
#include "mythinput.h"
#include "mythlogging.h"
#include "mythosd.h"
#include "mythsettings.h"
#include "searchSuggestions.h"
#include "shared.h"

#ifdef _WIN32
#include <iostream>
#include <windows.h>
#endif

/** \brief Creates a new MythApps Screen
 *  \param parent Pointer to the screen stack
 *  \param name The name of the window */
MythApps::MythApps(MythScreenStack *parent, QString name) : MythScreenType(parent, name) {
    managerFileBrowser = new QNetworkAccessManager();
    QObject::connect(managerFileBrowser, &QNetworkAccessManager::finished, this, &MythApps::ReplyFinishedFileBrowser);

    exitToMainMenuSleepTimer = new QTimer(this); // return to main menu after inactivty for power save.
    minimizeTimer = new QTimer(this);            // minimizes Kodi to the desktop every few seconds
    playbackTimer = new QTimer(this);            // update the playback status in the music app
    searchTimer = new QTimer(this);              // stops any hung searchs
    hintTimer = new QTimer(this);                // displays a hint message in the music app
    musicBarOnStopTimer = new QTimer(this);      // update the music bar status
    isKodiConnectedTimer = new QTimer(this);     // pings Kodi to check connection is still alive
    nextPageTimer = new QTimer(this);            // micro singleshot timer to load any next pages. Stops
                                                 // race conditions with multithreaded code.
    nextPageTimer->setSingleShot(true);
    searchSuggestTimer = new QTimer(this); // micro singleshot timer to delay the search suggestions after keypress.
    searchSuggestTimer->setSingleShot(true);
    openSettingTimer = new QTimer(this); // micro singleshot timer to delay opening setting
    openSettingTimer->setSingleShot(true);

    // slots
    connect(exitToMainMenuSleepTimer, SIGNAL(timeout()), this, SLOT(exitToMainMenuSleepTimerClose()));
    exitToMainMenuSleepTimer->start(15 * 60 * 1000);

    connect(minimizeTimer, SIGNAL(timeout()), this, SLOT(minimizeTimerSlot()));
    minimizeTimer->start(5 * 1000);

    connect(isKodiConnectedTimer, SIGNAL(timeout()), this, SLOT(isKodiConnectedSlot()));
    connect(playbackTimer, SIGNAL(timeout()), this, SLOT(playbackTimerSlot()));
    connect(searchTimer, SIGNAL(timeout()), this, SLOT(searchTimerSlot()));
    connect(hintTimer, SIGNAL(timeout()), this, SLOT(hintTimerSlot()));
    connect(musicBarOnStopTimer, SIGNAL(timeout()), this, SLOT(musicBarOnStopTimerSlot()));
    connect(searchSuggestTimer, SIGNAL(timeout()), this, SLOT(searchSuggestTimerSlot()));
    connect(nextPageTimer, SIGNAL(timeout()), this, SLOT(nextPageTimerSlot()));
    connect(openSettingTimer, SIGNAL(timeout()), this, SLOT(runMythSettingsSlot()));

    // Listens to Kodi's websocket for receiving push requests. Please note most json requests use NetRequest object.
    connect(&m_webSocket, &QWebSocket::connected, this, &MythApps::onConnectedWS);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &MythApps::closedWS);

    screen = QGuiApplication::primaryScreen();
}

/** \brief MythApps deconstructer to clean up timers on addon closing */
MythApps::~MythApps() {
    m_webSocket.close();
    exitToMainMenuSleepTimer->disconnect();
    minimizeTimer->disconnect();
    isKodiConnectedTimer->disconnect();
    playbackTimer->disconnect();
    searchTimer->disconnect();
    hintTimer->disconnect();
    musicBarOnStopTimer->disconnect();
    searchSuggestTimer->disconnect();
    nextPageTimer->disconnect();
    openSettingTimer->disconnect();
    managerFileBrowser->disconnect();

    delete apps;
    delete controls;
    delete m_filepath;
    delete m_plot;
    delete m_streamDetails;
    delete m_streamDetailsbackground;
    delete m_title;
    delete m_SearchTextEdit;
    delete m_screenshotMainMythImage;
    delete m_loaderImage;
    delete exitToMainMenuSleepTimer;
    delete minimizeTimer;
    delete isKodiConnectedTimer;
    delete playbackTimer;
    delete searchTimer;
    delete hintTimer;
    delete musicBarOnStopTimer;
    delete searchSuggestTimer;
    delete nextPageTimer;
    delete openSettingTimer;
    delete previouslyPlayedLink;
    delete favLink;
    delete watchedLink;
    delete searchListLink;
    delete managerFileBrowser;
    delete m_thumbnailImage;
    delete m_fileListGrid;
    delete m_searchButtonList;
    delete m_fileListMusicGrid;
    delete m_fileListSongs;
    delete m_filterGrid;
    delete m_filterOptionsList;
    delete m_playlistVertical;
    delete m_androidMenuBtn;
    // music app
    delete m_textSong;
    delete m_textArtist;
    delete m_textAlbum;
    delete m_musicDuration;
    delete m_hint;
    delete m_seekbar;
    delete m_musicTitle;
    delete m_trackProgress;
    delete m_coverart;
    delete m_blackhole_border;
    delete m_playingOn;
    delete m_next_buttonOn;
    delete m_ff_buttonOn;
    delete m_playingOff;
    delete m_next_buttonOff;
    delete m_ff_buttonOff;
}

/** \brief The Websocket connection to Kodi has been establised.  */
void MythApps::onConnectedWS() {
    LOG(VB_GENERAL, LOG_DEBUG, "WebSocket connected");
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &MythApps::onTextMessageReceived);
}

/** \brief Listens to OnPause,OnStop,OnPlay,OnInputRequested messages recieved from Kodi via the websocket.
 * \param kodi websocket message  */
void MythApps::onTextMessageReceived(QString message) {
    LOG(VB_GENERAL, LOG_DEBUG, "onTextMessageReceived():" + message);

    if (message.contains("Player.OnPause")) {
        openOSD("Player.OnPause");
    }

    if (message.contains("Player.OnStop")) {
        if (musicOpen || isHome) { // music
            musicBarOnStopTimer->start(500);
            if (controls->isFullscreenBool()) {
                goMinimize(true);
            }
        } else { // video
            openOSD("Player.OnStop");
            returnFocus();
            exitToMainMenuSleepTimer->start();
        }
    }
    if (message.contains("Player.OnPlay")) {
        musicBarOnStopTimer->stop();

        // if music video, ask if you want to open in fullscreen.
        if (musicOpen) {
            updateMusicPlayingBarStatus(); // update the music status bar
            if (message.contains("video")) {
                if (!controls->isFullscreenBool()) {
                    confirmDialog(tr("Do you want to open this video in fullscreen?"), "fullscreen");
                }
            }
        }
    }

    if (message.contains("Input.OnInputRequested") and !searching) {
        displayInputBox(message);
    }
}

/** \brief Create a inputbox with a title, type and value with pre populated feilds.
 * \param sonMessage json string from onTextMessageReceived() */
void MythApps::displayInputBox(QString jsonMessage) {
    LOG(VB_GENERAL, LOG_DEBUG, "displayInputBox()");
    MythScreenStack *mainStack = GetMythMainWindow()->GetMainStack();
    auto *mythInput = new MythInput(mainStack, "mythinput");

    // get the title, type and value from the json
    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonMessage.toLocal8Bit().data());
    QJsonObject jObject = jsonDocument.object();
    QVariantMap mainMap = jObject.toVariantMap();
    QVariantMap map2 = mainMap["params"].toMap();
    QVariantMap map3 = map2["data"].toMap();

    bool containsPassword = false;

    if (mythInput->Create()) {
        mainStack->AddScreen(mythInput);
        mythInput->setTitle(map3["title"].toString());
        containsPassword = mythInput->setType(map3["type"].toString());
        mythInput->setTextValue(map3["value"].toString());

        mythInput->waitforKey();
        if (mythInput->isCancelled()) { // inputbox cancelled by user, close inputbox
            mythInput->closeWindow();
            controls->inputBack();
        } else {
            inputSendText(mythInput->getTextValue()); // submit inputed text
            mythInput->closeWindow();

            if (containsPassword) { // refresh page if entered password
                delay(2);
                goBack();
            }
        }
    } else {
        delete mythInput;
    }
}

/** \brief close the 'exitToMainMenu' sleep timer and exit to the main menu */
void MythApps::exitToMainMenuSleepTimerClose() {
    LOG(VB_GENERAL, LOG_DEBUG, "exitToMainMenuSleepTimerClose()");
    niceClose(false);
}

/** \brief reset the 'exitToMainMenu' timer to zero seconds*/
void MythApps::exitToMainMenuSleepTimerReset() {
    if (exitToMainMenuSleepTimer->isActive()) {
        exitToMainMenuSleepTimer->stop();
        exitToMainMenuSleepTimer->start();
    }
}

/** \brief Creates a setting in the MythTv Database table if it doesn't exist
 *  \param settingName Setting Name
 *  \param settingValue Default setting value. Can be a string or boolean.
 *  \param remote Is the setting a backend or local setting*/
template <typename T> void MythApps::createSetting(QString settingName, T settingValue, bool remote) {
    if (remote) {
        if (gCoreContext->GetSettingOnHost(settingName, gCoreContext->GetMasterHostName()).isEmpty()) {
            gCoreContext->SaveSettingOnHost(settingName, QString(settingValue), gCoreContext->GetMasterHostName());
        }
    } else {
        if (gCoreContext->GetSetting(settingName).isEmpty()) {
            gCoreContext->SaveSetting(settingName, settingValue);
        }
    }
}

/** \brief create the main screen and initialize the setting and state*/
bool MythApps::Create() {
    LOG(VB_GENERAL, LOG_INFO, "Opening MythApp");
    createSetting("MythAppsusername", "kodi", false);
    createSetting("MythAppspassword", "kodi", false);
    createSetting("MythAppsip", "127.0.0.1", false);
    createSetting("MythAppsport", "8080", false);
    createSetting("MythAppsmyVideo", true, false);
    createSetting("MythAppsCloseOnExit", true, false);
    createSetting("MythAppsSearchList", "", false);
    createSetting("MythAppsInternalMute", true, false);
    createSetting("MythAppsInternalVol", true, false);
    createSetting("MythAppsInternalRemote", true, false);
    createSetting("MythAppsMusic", true, false);
    createSetting("MythAppsAppPass", "", true);
    createSetting("MythAppsYTapi", "", true);
    createSetting("MythAppsYTID", "", true);
    createSetting("MythAppsYTCS", "", true);
    createSetting("MythAppsCustomSearchSuggestUrl", "", true);
    createSetting("MythAppsPreviouslyPlayed", "", true);
    createSetting("MythAppsPreviouslyPlayedExclude", "be/play/?video_id=~", true);
    createSetting("MythAppsShowsAZfolderNames", "TV Shows~", true);

    username = QString(gCoreContext->GetSetting("MythAppsusername"));
    password = QString(gCoreContext->GetSetting("MythAppspassword"));
    ip = QString(gCoreContext->GetSetting("MythAppsip"));
    port = QString(gCoreContext->GetSetting("MythAppsport"));

    apps = new GetApps(username, password, ip, port);
    connect(apps, SIGNAL(loadProgramSignal(QString, QString, QString, bool)), this, SLOT(loadProgramSlot(QString, QString, QString, bool)));

    controls = new Controls(username, password, ip, port);
    showsAZfolderNames = gCoreContext->GetSettingOnHost("MythAppsShowsAZfolderNames", gCoreContext->GetMasterHostName()).split("~");

    bool foundtheme = false;

    QString theme = gCoreContext->GetSetting("Theme");

    if (theme.compare("Mythbuntu") == 0 || theme.compare("Willi") == 0 || theme.compare("Functionality") == 0 || theme.compare("Arclight") == 0 || theme.compare("Monochrome") == 0 ||
        theme.compare("MythAeon") == 0) {
        foundtheme = LoadWindowFromXML("mythapps-ui.xml", "mythapps", this);
    } else if (theme.compare("Steppes") == 0 || theme.compare("Steppes-large") == 0) {
        foundtheme = LoadWindowFromXML("mythapps-ui.Steppes.xml", "mythapps", this);
    } else if (theme.compare("MythCenter-wide") == 0) {
        foundtheme = LoadWindowFromXML("mythapps-ui.720.MCW.xml", "mythapps", this);
    } else if (theme.compare("MythCenterXMAS-wide") == 0) {
        foundtheme = LoadWindowFromXML("mythapps-ui.720.NoAlpha.xml", "mythapps", this);
        LOG(VB_GENERAL, LOG_INFO, "Using mythapps-ui.720.NoAlpha.xml");

    } else {
        foundtheme = LoadWindowFromXML("mythapps-ui.720.xml", "mythapps", this);
        LOG(VB_GENERAL, LOG_INFO, "Using mythapps-ui.720.xml");
    }

    if (!foundtheme){
        return false;
	}

    bool err = false;
    UIUtilE::Assign(this, m_plot, "plot", &err);
    UIUtilE::Assign(this, m_streamDetails, "streamDetails", &err);
    UIUtilE::Assign(this, m_streamDetailsbackground, "streamDetailsbackground", &err);
    UIUtilE::Assign(this, m_filepath, "filepath", &err);
    UIUtilE::Assign(this, m_title, "title", &err);
    UIUtilE::Assign(this, m_fileListGrid, "fileListGrid", &err); // main file browser used to display the apps.
    UIUtilE::Assign(this, m_screenshotMainMythImage, "screenshotImageMain", &err);
    UIUtilE::Assign(this, m_SearchTextEdit, "SearchTextEdit", &err);
    UIUtilE::Assign(this, m_searchButtonList, "searchButtonList", &err);
    UIUtilE::Assign(this, m_loaderImage, "loaderImage", &err);
    UIUtilE::Assign(this, m_searchButtonListGroup, "searchButtonListGroup", &err);
    UIUtilE::Assign(this, m_androidMenuBtn, "androidMenuBtn", &err);
    m_androidMenuBtn->SetVisible(false);
    m_androidMenuBtn->SetEnabled(false);
    UIUtilE::Assign(this, m_help, "help", &err);
    m_help->SetVisible(false);
    m_help->SetEnabled(false);

    m_SearchTextEdit->SetKeyboardPosition(VK_POSTOPDIALOG);

    MythUIButtonListItem *searchBtnList = new MythUIButtonListItem(m_searchButtonList, "");
    searchBtnList->SetText("Search", "buttontext2");

    bool err2 = initializeMusic(); // music
    if (err || err2) {
        LOG(VB_GENERAL, LOG_ERR, "Cannot load screen 'mythapps'");
        return false;
    }

    connect(m_fileListGrid, SIGNAL(itemClicked(MythUIButtonListItem *)), this, SLOT(appsCallback(MythUIButtonListItem *)));
    connect(m_fileListGrid, SIGNAL(itemSelected(MythUIButtonListItem *)), this, SLOT(selectAppsCallback(MythUIButtonListItem *)));
    connect(m_fileListGrid, SIGNAL(itemVisible(MythUIButtonListItem *)), this, SLOT(visibleAppsCallback(MythUIButtonListItem *)));
    connect(m_searchButtonList, SIGNAL(itemClicked(MythUIButtonListItem *)), this, SLOT(clickedSearchList(MythUIButtonListItem *)));
    connect(m_searchButtonList, SIGNAL(itemSelected(MythUIButtonListItem *)), this, SLOT(selectSearchList(MythUIButtonListItem *)));

    connect(m_SearchTextEdit, SIGNAL(valueChanged()), this, SLOT(searchTextEditValueChanged()));
    connect(m_androidMenuBtn, SIGNAL(Clicked()), this, SLOT(androidMenuBtnSlot()));

    // Start kodi if not running
#ifdef __ANDROID__
#elif _WIN32
    system("tasklist /nh /fi \"imagename eq kodi.exe\" | find /i \"kodi.exe\" > "
           "nul || (start kodi_start.cmd)");
#else
    if (gCoreContext->GetSetting("MythAppsInternalRemote").compare("1") == 0) {
        system("export LIRC_SOCKET_PATH=\"/\";if ! pgrep -x kodi > /dev/null; then "
               "kodi & fi;");
    } else {
        system("if ! pgrep -x kodi > /dev/null; then kodi & fi;");
    }
#endif

    for (int i = 0; i < 4; i++) {
        if (isKodiPingable(ip, port)) {
            connected = 1;
            break;
        } else {
            delay(1);
        }
        goMinimize(true);
    }

    m_webSocket.open(QUrl("ws://" + ip + ":9090"));

    loadApps();

    // Setup cache directories
    globalPathprefix = GetConfDir();
    createDirectoryIfDoesNotExist(globalPathprefix);
    globalPathprefix += "/MythApps/";
    createDirectoryIfDoesNotExist(globalPathprefix);
    createDirectoryIfDoesNotExist(globalPathprefix + "00cache/");

    // load icons
    fav_filename = createImageCachePath("ma_favourites.png");
    mm_albums_filename = createImageCachePath("ma_mm_albums.png");
    mm_alltracks_filename = createImageCachePath("ma_mm_alltracks.png");
    mm_artists_filename = createImageCachePath("ma_mm_artists.png");
    mm_genres_filename = createImageCachePath("ma_mm_genres.png");
    mm_playlist_filename = createImageCachePath("ma_mm_playlists.png");
    recent_filename = createImageCachePath("ma_recent.png");
    videos_filename = createImageCachePath("ma_video.png");
    music_filename = createImageCachePath("ma_music.png");
    back_filename = createImageCachePath("ma_mv_gallery_dir_up.png");
    ma_tv_filename = createImageCachePath("ma_tv.png");

    previouslyPlayedLink = new ProgramLink("mythappsPreviouslyPlayed", true, false); // load previously played videos
    favLink = new ProgramLink("mythappsAllFavourites", true, false);                 // load Favourites
    watchedLink = new ProgramLink("mythappsAllWatched", true, false);                // load Watched List
    searchListLink = new ProgramLink("MythAppsSearchList", false, false);            // load Search Sources
    searchListLink->sort();

    m_streamDetailsbackground->Hide();

#ifdef _WIN32
    delayMilli(400);
    wchar_t kodi_wchar[] = L"MythApps";
    SetActiveWindow(FindWindow(NULL, kodi_wchar));
#endif
    LOG(VB_GENERAL, LOG_DEBUG, "Create() Finished. Threads: " + QString::number(QThread::idealThreadCount()) + ", 4+ recommended");
    return true;
}

/** \brief Returns the full cached image path based of the original image location. Will copy the image to the cache if required. Used to copy mythapp
 * icons such as favourites to the cache directory.
 * \param imagePath path of the orginal image
 * \param imageFileName filename of the image
 * \param globalPathprefix this is the plungins cache directory
 * \return full cached image path with the image in the cache */
QString MythApps::createImageCachePath(QString imageFileName) {
    QString cachedPath = QString("%1%2").arg(GetShareDir()).arg("themes/default//" + imageFileName);
    if (QFile::exists(cachedPath)) {
        QFile::copy(cachedPath, globalPathprefix + "/" + imageFileName);
    }
    return QString("file://") + globalPathprefix + "/" + imageFileName;
}

/** \brief create and initialize the main app screen*/
void MythApps::loadApps() {
    LOG(VB_GENERAL, LOG_DEBUG, "loadApps() Start");
    // Initialize the load apps state
    isHome = true;
    browserOpen = false;
    isFavouritesOpen = false;
    currentSearchUrl = "";
    showMusicUI(false);
    azShowOnUrl.clear();
    delayMilli(5);
    toggleSearchVisible(true);
    m_hint->SetVisible(false);
    m_loaderImage->SetVisible(false);
    musicMode = 1;
    resetScreenshot();

#ifdef __ANDROID__ // display a button to bring up the menu on a touch screen
    m_androidMenuBtn->SetVisible(true);
    m_androidMenuBtn->SetEnabled(true);
#endif

    playbackTimer->stop();
    partyMode = false;
    m_fileListGrid->Reset();
    m_SearchTextEdit->SetText("");

    if (musicOpen) {
        clearAndStopPlaylist();
    }
    musicOpen = false;
    m_filepath->SetText("");
    if (connected == 1) {
        if (!controls->isUserNamePasswordCorrect()) {
            delayMilli(200); // retry incase slow to start
            if (!controls->isUserNamePasswordCorrect()) {
                createAutoClosingBusyDialog(tr("Authentication Error. Please check Kodi username/password in settings. (Setting m key)"), 6);
                openSettingTimer->start(200);
                return;
            }
        } else if (!controls->areAddonsInstalled()) {
            m_filepath->SetText(tr("No video addons installed. Press F3 to open Kodi and install some addons."));
            LOG(VB_GENERAL, LOG_ERR, "No Kodi video addons installed");
        } else if (!controls->isInputAdaptive()) {
            LOG(VB_GENERAL, LOG_ERR, "Please install the inputstream adaptive addon.");
        }

        isKodiConnectedTimer->start(10 * 1000);
        connected = 2;
    } else if (connected == 0) {
        createAutoClosingBusyDialog(tr("Failed to Connect. Is Kodi installed/running with remote control enabled? (Setting m key)"), 6);
        openSettingTimer->start(200);
        return;
    }

    apps->loadAll();
    SetFocusWidget(m_fileListGrid); // set the focus to the file list grid

    LOG(VB_GENERAL, LOG_DEBUG, "loadApps() Finished");
}

/** \brief updates the status of browserOpen varaible and returns the focus if
 * required */
void MythApps::updateBrowserOpenStatus() {
    if (browserOpen) {
        FILE *cmd = popen("pgrep -c --full 'chrome*.*kios*.*'", "r");
        char result[24] = {0x0};
        while (fgets(result, sizeof(result), cmd) != NULL) {
        }

        QString string(result);
        if (string.toInt() < 2) {
            browserOpen = false;
            SetFocusWidget(m_searchButtonList);
        }
        pclose(cmd);
    }
}

/** \brief handle key press events  */
bool MythApps::keyPressEvent(QKeyEvent *event) {
    exitToMainMenuSleepTimerReset();

    if (GetFocusWidget() && GetFocusWidget()->keyPressEvent(event)) {
        return true;
    }

    updateBrowserOpenStatus();
    bool handled = false;
    QStringList actions;
    handled = GetMythMainWindow()->TranslateKeyPress("mythapps", event, actions);
    bool musicPlayerFullscreenOpen = false;
    if (musicOpen) {
        musicPlayerFullscreenOpen = controls->isFullscreenBool();
    }

    for (int i = 0; i < actions.size() && !handled; i++) {
        QString action = actions[i];
        LOG(VB_GENERAL, LOG_DEBUG, "keyPressEvent()" + action);
        handled = true;

        if (m_busyPopup) {
            LOG(VB_GENERAL, LOG_WARNING, "Too busy. Not handling keypress");
        } else if (action == "CLOSE") {
            niceClose(true);
        } else if (action == "REFRESH") {
            refreshPage(true);
        } else if (action == "TOGGLEHIDDEN") {
            toggleHiddenFolders();
        } else if (action == "TOGGLERECORD") {
            addToUnWatchedList(true);
        } else if (action == "HELP") {
            m_help->SetVisible(!m_help->IsVisible());
#ifndef _WIN32
#ifndef __ANDROID__
        } else if ((action == "BACK" || action == "ESCAPE") and browserOpen) { // browser key events
            system("xdotool search --class Chromium windowactivate --sync %1 key alt+F4 windowactivate $(xdotool getactivewindow)");
            browserOpen = false;
            SetFocusWidget(m_fileListGrid);
        } else if ((action == "RIGHT") and browserOpen) {
            system("xdotool search --onlyvisible --classname chromium windowactivate --sync key Tab");
        } else if ((action == "PLAY" || action == "PAUSE") and browserOpen) {
            system("xdotool search --onlyvisible --classname chromium windowactivate --sync key space");
        } else if ((action == "DETAILS" || action == "MENU" || action == "LEFT") and browserOpen) {
            system("xdotool search --onlyvisible --classname chromium windowactivate --sync key F");
        } else if ((action == "UP") and browserOpen) {
            system("xdotool search --onlyvisible --classname chromium windowactivate --sync key Page_Up");
        } else if ((action == "DOWN") and browserOpen) {
            system("xdotool search --onlyvisible --classname chromium windowactivate --sync key Page_Down");
        } else if ((action == "FULLSCREEN")) { // generic key events
            toggleFullscreen();
            createAutoClosingBusyDialog(tr("Toggling Fullscreen"), 3);
        } else if ((action == "MINIMIZE")) {
            toggleAutoMinimize();
        } else if (browserOpen) { // do nothing.
#endif
#endif
            // player key events
        } else if ((action == "BACK" || action == "ESCAPE") and (kodiPlayerOpen || musicPlayerFullscreenOpen || GetFocusWidget() == m_filepath)) {
            stopPlayBack(); // Kodi player running. Lets stop
        } else if ((action == "PLAY" || action == "PAUSE") and (kodiPlayerOpen || musicOpen)) {
            pauseToggle();
        } else if ((action == "DETAILS") and kodiPlayerOpen) {
            controls->showInfo();
        } else if ((action == "DETAILS")) {
            if (m_streamDetailsbackground->IsVisible() == false) {
                m_streamDetailsbackground->Show();
                m_streamDetails->Show();
                m_streamDetails->SetText(getStreamBitrate() + streamDetails);
                LOG(VB_GENERAL, LOG_DEBUG, getStreamBitrate() + streamDetails);
            } else {
                m_streamDetailsbackground->Hide();
                m_streamDetails->Hide();
            }
        } else if ((action == "MENU") and kodiPlayerOpen) {
            controls->showPlayerProcessInfo();
        } else if ((action == "MUTE") and (kodiPlayerOpen || musicOpen)) {
            controls->setMute();
        } else if ((action == "VOLUMEDOWN") and (kodiPlayerOpen || musicOpen)) {
            controls->increaseVol();
        } else if ((action == "VOLUMEUP") and (kodiPlayerOpen || musicOpen)) {
            controls->decreaseVol();
        } else if ((action == "RIGHT" || action == "SEEKFFWD") and kodiPlayerOpen) {
            controls->seekFoward();
        } else if ((action == "LEFT" || action == "SEEKRWND") and kodiPlayerOpen) {
            controls->seekBack();
        } else if (action == "FFWD" and kodiPlayerOpen) {
            controls->setFFWD();
        } else if (action == "RWND" and kodiPlayerOpen) {
            controls->setRWND();
        } else if (action == "NEXTTRACK" and musicOpen) {
            // music key events
            nextTrack();
        } else if (action == "PREVTRACK" and musicOpen) {
            previousTrack();
        } else if ((action == "RIGHT") and musicPlayerFullscreenOpen) {
            nextTrack();
        } else if ((action == "LEFT") and musicPlayerFullscreenOpen) {
            previousTrack();
        } else if ((action == "RIGHT") and GetFocusWidget() == m_fileListSongs and m_playlistVertical->GetCount() > 0) {
            SetFocusWidget(m_playlistVertical);
        } else if ((action == "LEFT" || action == "BACK" || action == "ESCAPE") and GetFocusWidget() == m_playlistVertical) {
            SetFocusWidget(m_fileListSongs);
        } else if ((action == "LEFT" || action == "BACK" || action == "ESCAPE") and GetFocusWidget() == m_fileListSongs) {
            int pos = m_fileListMusicGrid->GetCurrentPos();
            if (pos > 0) {
                pos = pos - 1;
            }
            m_fileListMusicGrid->SetItemCurrent(pos);
            SetFocusWidget(m_fileListMusicGrid);
        } else if ((action == "BACK" || action == "ESCAPE") and GetFocusWidget() == m_fileListMusicGrid) {
            m_filterGrid->SetItemCurrent(0);
            SetFocusWidget(m_filterGrid);
        } else if ((action == "BACK" || action == "ESCAPE") and GetFocusWidget() == m_filterGrid) {
            if (m_SearchTextEdit->IsVisible()) {
                SetFocusWidget(m_SearchTextEdit);
                setSearchButtonListVisible(true);
            } else {
                goBack();
            }
        } else if ((action == "UP") and GetFocusWidget() == m_SearchTextEdit and musicOpen) {
            SetFocusWidget(m_fileListMusicGrid);
        } else if ((action == "SEEKFFWD") and musicOpen) {
            controls->seekFoward();
        } else if ((action == "SEEKRWND") and musicOpen) {
            controls->seekBack();
        } else if ((action == "SELECT") and musicOpen and (GetFocusWidget() == m_playingOn)) {
            pauseToggle();
        } else if ((action == "SELECT") and musicOpen and (GetFocusWidget() == m_ff_buttonOn)) {
            controls->seekFoward();
        } else if ((action == "SELECT") and musicOpen and (GetFocusWidget() == m_next_buttonOn)) {
            nextTrack();
        } else if ((action == "RIGHT") and GetFocusWidget() == m_playlistVertical and m_ff_buttonOff->IsVisible()) {
            m_currentMusicButton = 1; // m_playing
            showMusicPlayingBar(true);
        } else if ((action == "RIGHT") and GetFocusWidget() == m_playingOn) {
            m_currentMusicButton = 2; // m_next_button
            showMusicPlayingBar(true);
            SetFocusWidget(m_next_buttonOn);
        } else if ((action == "RIGHT") and GetFocusWidget() == m_next_buttonOn) {
            m_currentMusicButton = 3; // m_ff_button
            showMusicPlayingBar(true);
        } else if ((action == "RIGHT") and GetFocusWidget() == m_ff_buttonOn) {
            m_currentMusicButton = 0;
            showMusicPlayingBar(true);
            SetFocusWidget(m_playlistVertical);
        } else if ((action == "LEFT" || action == "BACK" || action == "ESCAPE") and GetFocusWidget() == m_ff_buttonOn) {
            m_currentMusicButton = 2; // m_next_button
            showMusicPlayingBar(true);
        } else if ((action == "LEFT" || action == "BACK" || action == "ESCAPE") and GetFocusWidget() == m_next_buttonOn) {
            m_currentMusicButton = 1; // m_playing
            showMusicPlayingBar(true);
        } else if ((action == "LEFT" || action == "BACK" || action == "ESCAPE") and GetFocusWidget() == m_playingOn) {
            m_currentMusicButton = 0;
            showMusicPlayingBar(true);
            SetFocusWidget(m_playlistVertical);
        } else if ((action == "LEFT" || action == "BACK" || action == "ESCAPE") and GetFocusWidget() == m_filterOptionsList) {
            m_filterGrid->SetItemCurrent(0, 0);
            SetFocusWidget(m_filterGrid);
        } else if ((action == "RIGHT") and GetFocusWidget() == m_filterOptionsList) {
            if (m_playlistVertical->GetCount() > 0) {
                SetFocusWidget(m_playlistVertical);
            } else {
                SetFocusWidget(m_fileListMusicGrid);
            }
            // mythapps key events
        } else if (kodiPlayerOpen) { // do nothing.

        } else if ((action == "BACK" || action == "ESCAPE") and GetFocusWidget() == m_fileListGrid and m_SearchTextEdit->IsVisible()) {
            SetFocusWidget(m_SearchTextEdit);
            setSearchButtonListVisible(true);
        } else if ((action == "BACK" || action == "ESCAPE") and GetFocusWidget() == m_fileListGrid) {
            goBack();
        } else if ((action == "BACK" || action == "ESCAPE") and GetFocusWidget() == m_SearchTextEdit and isHome) {
            niceClose(false);
        } else if ((action == "BACK" || action == "ESCAPE") and GetFocusWidget() == m_SearchTextEdit) {
            goBack();
        } else if (action == "UP" and GetFocusWidget() == m_SearchTextEdit) {
            SetFocusWidget(m_fileListGrid);
            setSearchButtonListVisible(false);
        } else if (action == "DOWN" and GetFocusWidget() == m_SearchTextEdit) {
            SetFocusWidget(m_searchButtonList);
        } else if ((action == "BACK" || action == "ESCAPE" || action == "UP" || action == "LEFT") and GetFocusWidget() == m_searchButtonList) {
            SetFocusWidget(m_SearchTextEdit);
        } else if ((action == "DOWN" || action == "RIGHT") and GetFocusWidget() == m_searchButtonList) {
            SetFocusWidget(m_fileListGrid);
            setSearchButtonListVisible(false);

            if (musicOpen) {
                SetFocusWidget(m_fileListMusicGrid);
            }
        } else if (action == "MENU") {
            showOptionsMenu();
        } else {
            handled = false;
        }
    }

    if (!handled && MythScreenType::keyPressEvent(event))
        handled = true;

    return handled;
}

/** \brief Delay the closing of this addon to allow any threads to complete. Will also close kodi if required.
 * \param forceClose force close the addon */
void MythApps::niceClose(bool forceClose) {
    LOG(VB_GENERAL, LOG_DEBUG, "niceClose()");

    if (forceClose) {
        connected = 0;
        delay(1);
    }

    if (gCoreContext->GetSetting("MythAppsCloseOnExit").compare("1") == 0 || forceClose) {

        if (!forceClose) {
#ifdef _WIN32
            system("taskkill /im kodi.exe");
#else
            system("kodi-send --action=\"Quit\"");
#endif
        }
        delayMilli(150);
#ifdef _WIN32
        system("taskkill /f /im kodi.exe");
#else
        system("pkill -9 kodi.bin");
#endif
    } else {
        delayMilli(50);
    }
    Close();
}

/** \brief get the number of threads running. Also remove finished threads from the thread pool
 \return returns the number of threads running */
int MythApps::getThreadCount() {
    int currentThreadsRunning = 0;
    int imageThreadListCount = 0;
    foreach (QThread *thread, imageThreadList) {
        if (thread->isRunning()) {
            currentThreadsRunning++;
        }
        if (thread->isFinished()) {
            thread->quit();
            imageThreadList.removeAt(imageThreadListCount);
        }
        imageThreadListCount++;
    }
    return currentThreadsRunning;
}

/** \brief wait for threads to complete
 *  \param  maxThreadsRunning what is the max threads that should be running?*/
void MythApps::waitForThreads(int maxThreadsRunning) {
    while (getThreadCount() > maxThreadsRunning) {
        delayMilli(50);
    }
}

/** \brief helper function for loadProgram */
void MythApps::loadProgramSlot(QString name, QString setdata, QString thumbnailPath, bool appDir) { loadProgram(name, setdata, thumbnailPath, appDir); }

/** \brief helper function for loadProgram */
void MythApps::loadProgram(QString name, QString setdata, QString thumbnailPath, bool appDir) { loadProgram(name, setdata, thumbnailPath, appDir, m_fileListGrid); }

/** \brief prepares to load the image onscreen. Will insert the image on a new line and sort the images if required \param name directory or file name.
 * \param setdata - all the parameters that need to be retrived on click or hover
 * \param thumbnailPath
 * \param appDir is the image a top level directory, basiclly is the image on the start screen.
 * \param mythUIButtonList The button to load the image*/
void MythApps::loadProgram(QString name, QString setdata, QString thumbnailPath, bool appDir, MythUIButtonList *mythUIButtonList) {
    static int count = 0;
    int max = 6;

    if (name.contains(tr("Back"))) {
        count = 0;
    }

    waitForThreads(512); // wait if there are too many threads running to avoid crashs

    if (appDir) {
        // load apps in alphabetical order
        QStringList pgList; // convert to a list and then map for sorting
        pgList.append(setdata);
        pgList.append(thumbnailPath);

        ProgramMap[name.toLocal8Bit().constData()] = pgList;
        sortProgram(); // sort apps in alphabetical order
    } else {
        // load apps in the default order
        count++;

        if (count == max) {
            count = 0;
        }

        if (name.compare("split") == 0) {
            // split will load the image on a new line. Used heavily in music.
            int maxCalc = (max - (count - 1));
            if (maxCalc == max + 1) {
                maxCalc = 1;
            }

            for (int i = 0; i < maxCalc; i++) {
                loadImage(mythUIButtonList, "Blank", setdata, "");
            }
            count = 0;
        } else {
            // load the image on the screen
            loadImage(mythUIButtonList, name, setdata, thumbnailPath);
        }
    }
}

/** \brief load the program/images in alphabetical order */
void MythApps::sortProgram() {
    QCoreApplication::processEvents();
    m_fileListGrid->Reset();

    // load the favourites and watch list before sorting
    loadImage(m_fileListGrid, QString(tr("Favourites") + favLink->getListSize()), QString("Favourites~Favourites"), fav_filename);
    loadImage(m_fileListGrid, QString(tr("Watched List") + watchedLink->getListSize()), QString("Watched List~Watched List"), recent_filename);

    std::map<std::string, QStringList>::iterator it = ProgramMap.begin(); // sort images
    while (it != ProgramMap.end()) {
        QString name = QString::fromUtf8(it->first.c_str());
        loadImage(m_fileListGrid, name, it->second.at(0), it->second.at(1));
        it++;
    }

    // load the my videos and my music after sorting if enabled
    if (gCoreContext->GetSetting("MythAppsmyVideo").compare("1") == 0) {
        loadImage(m_fileListGrid, tr("My Videos"), QString("Videos~Videos"), videos_filename);
    }
    if (gCoreContext->GetSetting("MythAppsMusic").compare("1") == 0) {
        loadImage(m_fileListGrid, tr("Music"), QString("Music~Music"), music_filename);
    }

    loadFavourites(true);
}

/** \brief converts local file or a http link into a cached image full path.
 * \param thumbnailPath - Can be blank, a local file or a http link
 * \return cached image full path */
QString MythApps::getStandarizedImagePath(QString imagePath) {
    QString imageLocation = "";

    if (imagePath.contains(QString("file://"),
                           Qt::CaseInsensitive)) { // local file location, such a
                                                   // the favourite image
        imageLocation = imagePath.replace(QString("file://"), QString(""));
    } else if (imagePath.compare(QString("")) == 0) { // no image file or link
        imageLocation = QString("%1%2").arg(GetShareDir()).arg("themes/default/ma_mv_browse_nocover.png");
    } else { // download image file from http (Kodi json)
        QString imgID = QString(QCryptographicHash::hash((imagePath.toUtf8()), QCryptographicHash::Md5).toHex());
        imageLocation = globalPathprefix + imgID;
    }
    return imageLocation;
}

/** \brief creates the data later used to load the image onscreen as a file or directory.
 * \param mythUIButtonList The button to load the image
 * \param name directory or file name.
 * \param etdata all the parameters that need to be retrived on click or hover
 * \param thumbnailPath */
void MythApps::loadImage(MythUIButtonList *mythUIButtonList, QString name, QString setdata, QString thumbnailPath) {
    auto *item = new MythUIButtonListItem(mythUIButtonList, "");

    if (name.compare("BlankR") == 0) { // no image and no click or hover
        item->setVisible(false);
        item->SetData("BlankR");
        return;
    }
    if (name.compare("Blank") == 0) { // no image
        item->setVisible(false);
        return;
    }

    QString playUrl = getParm(setdata, 0);
    if (previouslyPlayedLink->contains(playUrl) and !isHome) { // disable on home screen
        item->SetText(name, "buttontextWatched");              // set the image title / name
    } else {
        item->SetText(name, "buttontext2"); // set the image title / name
    }

    QString imageLocation = getStandarizedImagePath(thumbnailPath);
    QString appIcon = getStandarizedImagePath(apps->getLocationFromUrlAddress(setdata.split('~').at(0)));

    // to speedup the loading, save the parameters in a list. When the image is
    // displayed onscreen, the parameters are used to create a thread to display
    // the image.
    QStringList threadInfo;
    threadInfo.append("threadInfo");
    threadInfo.append(thumbnailPath);
    threadInfo.append(imageLocation);
    threadInfo.append(appIcon);
    threadInfo.append(setdata);

    item->SetData(threadInfo);
}

/** \brief receives result from image thread and displays the corresponding image onscreen.
 *  \param id id of the button to update with the new image
 *  \param fileListType button list or file browser to update*/
void MythApps::handleImageSlot(int id, MythUIButtonList *fileListType) {
    if (id <= fileListType->GetCount()) {
        QString itemData = fileListType->GetItemAt(id)->GetData().toString();
        if (itemData.contains("*_")) {
            QStringList imageFileLocation = itemData.split("*_");
            fileListType->GetItemAt(id)->SetImage(imageFileLocation.at(1));

            QString imageFileLocationCombined = imageFileLocation.at(0);
            fileListType->GetItemAt(id)->SetData(imageFileLocationCombined);
        }
    } else {
        LOG(VB_GENERAL, LOG_ERR, "handleImageSlot() error");
    }
}

void MythApps::setSearchButtonListVisible(bool visible) {
    m_searchButtonList->SetVisible(visible);
    m_searchButtonListGroup->SetEnabled(visible);
}

/** \brief  toggle the visibilty of the seachbox including the search button.
 *  \param  visible - set the search box visibility status */
void MythApps::toggleSearchVisible(bool visible) {
    m_SearchTextEdit->SetVisible(visible);
    setSearchButtonListVisible(false);

    m_title->SetVisible(!visible);
}

/** \brief Display's the back button in the file browser grid*/
void MythApps::loadBackButton() {
    LOG(VB_GENERAL, LOG_DEBUG, "loadBackButton()");

    if (searching) {
        SetFocusWidget(m_fileListGrid);
    }
    loadProgram(QString(tr("Back")), createProgramData(tr("Back"), "", "", false, ""), QString("file://") + back_filename, false);
}

/** \brief selected callback for the search list. */
void MythApps::selectSearchList(MythUIButtonListItem *item) {
    QString buttonName = item->GetText();
    if (buttonName.compare("Back to Search") == 0) {
        SetFocusWidget(m_SearchTextEdit);
        m_searchButtonList->SetItemCurrent(0);
    }
}

/** \brief  creates a text dialog that auto closes
 *  \param  dialogTex to display onscreen
 *  \param  delaySeconds seconds to autoclose */
void MythApps::createAutoClosingBusyDialog(QString dialogText, int delaySeconds) {
    createBusyDialog(dialogText);
    delay(delaySeconds);
    closeBusyDialog();
}

/** \brief clicked callback for the search list. */
void MythApps::clickedSearchList(MythUIButtonListItem *item) {
    if (searchListLink->getListSizeEnabled() == 0) { // display error is no search sources
        createAutoClosingBusyDialog(tr("Please setup search sources in M->Setting"), 3);
        return;
    }

    QString buttonName = item->GetText("buttontext2");
    if (buttonName.compare("Search") == 0) { // the search submit button is the first search suggeston
        goSearch("");
    } else if (buttonName.compare("Back to Search") == 0) {
    } else {
        m_SearchTextEdit->SetText(item->GetText("buttontext2"));
        goSearch("");
    }
}

/** \brief slot for the isKodiConnected timmer. Used to close kodi if not pingable after 3 times  */
void MythApps::isKodiConnectedSlot() {
    int notPingableCount = 0;
    while (!isKodiPingable(ip, port)) {
        notPingableCount++;
        if (notPingableCount == 3) {
            LOG(VB_GENERAL, LOG_INFO, "Closing as not pingable.");
            niceClose(true);
        }
        delay(1);
    }
}

/** \brief text changed in the searchbox */
void MythApps::searchTextEditValueChanged() {
    if (!searching) {
        setSearchButtonListVisible(true); // sets visible when mouse is used instead of remote.
    }
    searchSuggestTimer->start(10); // run the code in a seperate run once timer 'thread'
}

/** \brief Looks up the search suggestions for the search box after a slight delay*/
void MythApps::searchSuggestTimerSlot() {
    QString localSearchText = m_SearchTextEdit->GetText();
    searchText = localSearchText;

    // check if a popular video url is entered in the search box and open the
    // video. Feature detecion rather than hard coded website urls is used.
    if (searchText.contains("https") and searchText.contains("v=")) {

        QUrl qu(m_SearchTextEdit->GetText().trimmed());
        QString websiteName = qu.host().replace("www.", "").replace(".com", "");

        // try to find if the websites domain name correlates to any installed apps.
        if (QListContains(apps->getAppIDlist(), websiteName)) {
            QString appName = QListSearch(apps->getAppIDlist(), websiteName);

            QUrlQuery q;
            q.setQuery(qu.query());
            QString vUrl = q.queryItemValue("v", QUrl::FullyDecoded);

            m_SearchTextEdit->SetText(vUrl);
            foreach (QString search, searchListLink->getListEnabled()) {
                if (search.contains(websiteName)) {
                    isHome = false;
                    goSearch(search);
                }
            }

            play_Kodi("plugin://" + appName + "/?action=play_video&videoid=" + vUrl, QString("00:00:00"));
        }
        return;
    }

    SearchSuggestions *suggestions = new SearchSuggestions();
    QStringList word = suggestions->getSuggestions(localSearchText);

    m_searchButtonList->Reset();
    MythUIButtonListItem *searchButton = new MythUIButtonListItem(m_searchButtonList,
                                                                  ""); // submit button. 1st result in search suggestion list
    searchButton->SetText("Search", "buttontext2");

    for (int i = 0; i < word.size(); ++i) {
        MythUIButtonListItem *searchButton2 = new MythUIButtonListItem(m_searchButtonList, "");
        searchButton2->SetText(word.at(i).toLocal8Bit().constData(), "buttontext2");
    }

    if (word.size() > 1) {
        MythUIButtonListItem *searchButton2 = new MythUIButtonListItem(m_searchButtonList,
                                                                       ""); // submit button. 1st result in search suggestion list
        searchButton2->SetText("Back to Search", "buttontext2");
    }

    if (localSearchText.compare(searchText) != 0) { // incase search value has changed, retry
        searchSuggestTimer->start(10);
    }
}

/** \brief Can search music or apps from one or more sources
 *  \param overrideCurrentSearchUrl override the search source url used to retrieve the search result from */
void MythApps::goSearch(QString overrideCurrentSearchUrl) {
    LOG(VB_GENERAL, LOG_DEBUG, "goSearch()");

#ifndef _WIN32
    if (browserOpen) {
        system("xdotool search --onlyvisible --classname chromium windowactivate "
               "--sync key Return");
        return;
    }
#endif

    setSearchButtonListVisible(false);
    searchNoDuplicateCheck = QStringList();

    if (!previousURL.contains("searchResults")) {
        previousURL.append("searchResults"); // used to manage the back button history
    }
    searchText = m_SearchTextEdit->GetText();

    if (searchText.size() < 1) { // make sure search term is at least one character
        return;
    }

    if (musicOpen) { // search music
        musicSearch(searchText);
        return;
    }

    createBusyDialog(tr("Retrieving Search Results..."));

    searching = true;
    QCoreApplication::processEvents();

    m_fileListGrid->Reset();
    loadBackButton();

    if (overrideCurrentSearchUrl.compare("") != 0) {
        fetchSearch(overrideCurrentSearchUrl);
    } else if (isHome) {
        foreach (QString search, searchListLink->getListEnabled()) { fetchSearch(search); } // fetch all search results for all apps.
    } else {
        fetchSearch(currentSearchUrl); // fetch all search results for the current app.
    }

    QStringListIterator searchIterator(searchSubFoldersList); // fetch subfolders. Some apps return the search
                                                              // results in a subfolder
    while (searchIterator.hasNext()) {
        requestFileBrowser(searchIterator.next(), QStringList(), false, "");
    }
    searchSubFoldersList.clear();

    waitforRequests(); // wait for all search results to finish

    searchSubFoldersList.clear();
    m_SearchTextEdit->SetText("");

    closeBusyDialog(); // close the Retrieving Search Results dialog
    m_filepath->SetText(tr("Finished Searching - ") + m_filepath->GetText());
}

/** \brief gets search results from one app.
 * \param The app searchUrl */
void MythApps::fetchSearch(QString searchUrl) {
    LOG(VB_GENERAL, LOG_DEBUG, "fetchSearch(): " + searchUrl);
    searchTimer->stop();
    searchTimer->start(20 * 1000); // timer to timeout hung searchs

    QString searchUrlList;
    if (searchUrl.contains(",")) {
        QStringList searchUrlSplit = searchUrl.split(',');
        searchUrl = searchUrlSplit.at(0); // This is the url that displays the previously entered search terms.
    }

    requestFileBrowser(searchUrl, QStringList(), false, "");
    inputSendText(searchText); // this creates the search term and returns the search results in addons that use 'search'

    if (!searchUrlList.isNull()) { // the addon needs the search terms retriving (the addon uses 'new search')
        QString searchDirectory = controls->getSearchDirectory(searchUrlList, searchText);
        LOG(VB_GENERAL, LOG_DEBUG, "searchDirectory: " + searchDirectory);

        requestFileBrowser(searchDirectory, QStringList(), false, "");
        if (searchDirectory.compare("") == 0) {
            LOG(VB_GENERAL, LOG_ERR, "fetchSearch - error?" + searchUrl);
        }
    }
    searchTimer->stop();
    delayMilli(50);
}

/** \brief toggle showing and hiding hidden folders such as login, logout etc*/
void MythApps::toggleHiddenFolders() {
    if (enableHiddenFolders) {
        createAutoClosingBusyDialog(tr("Hiding Folders"), 3);
    } else {
        createAutoClosingBusyDialog(tr("Showing Hidden Folders"), 3);
    }
    enableHiddenFolders = !enableHiddenFolders;
    refreshPage(false);
}

/** \brief toggle the allowing of the auto minimizeing of Kodi. Useful to edit settings*/
void MythApps::toggleAutoMinimize() {
    allowAutoMinimize = !allowAutoMinimize;
    if (allowAutoMinimize) {
        createAutoClosingBusyDialog(tr("Auto Minimize On"), 3);
    } else {
        createAutoClosingBusyDialog(tr("Warning - Auto Minimize Off"), 3);
        goFullscreen();
        toggleFullscreen();
        controls->activateWindow("home");
    }
}

/** \brief send text to Kodi. Useful for dialogs without an api.
 * \param The text to send */
void MythApps::inputSendText(QString text) {
    LOG(VB_GENERAL, LOG_DEBUG, "inputSendText");

    int openDialogTimeout = 0; // confirm dialog is open
    while (!controls->isVirtualKeyboardOpen()) {
        delayMilli(50);
        openDialogTimeout++;

        if (openDialogTimeout > 20) {
            return;
        }
    }

    controls->inputSendText(text);

    int closeDialogTimeout = 0; // confirm Input Dialog has closed
    while (controls->isVirtualKeyboardOpen()) {
        delayMilli(50);
        closeDialogTimeout++;

        if (closeDialogTimeout > 20) {
            break;
        }
    }
    handleDialogs(false);
}

/** \brief Determine if a folder is allowed from a hardcoded blacklist. useful to filter out unwanted folders to reduce clutter.
 *  \param  label - folder name
 *  \param  previousSearchTerms - list of search terms than should not appear as a folder name. (helps to filter out previous searches)
 * \return is the folder allowed?*/
bool MythApps::folderAllowed(QString label, QStringList previousSearchTerms) {
    QList<QString> list = {"Bookmarks", "Settings", "Logout", "Search", "New Search", "Switch User", "Select Profile", "Quick Search (Incognito)"};
    QList<QString> list2 = {"Live", "Playlists"};

    if (enableHiddenFolders) {
        return true;
    }

    if (list.contains(label)) {
        return false;
    }

    if (searching and list2.contains(label)) {
        return false;
    }

    if (searching and previousSearchTerms.contains(label)) {
        return false;
    }

    return true;
}

/** \brief Runs when a directory or file is clicked in the file browser and loads the next directory or plays a video. Passes the json result to the
 * DispalyFileBrowser function and caches the json data*/
void MythApps::ReplyFinishedFileBrowser(QNetworkReply *reply) {
    QStringList previousSearchTerms = reply->property("previousSearchTerms").toStringList();
    QString filePathName = reply->property("filePathName").toString();

    QString answer = reply->readAll(); // reads the json reponse
    LOG(VB_GENERAL, LOG_DEBUG, "ReplyFinishedFileBrowser" + answer);

    // if the updateCacheOnly flag has been sent, then don't display in the gui
    if (!reply->property("updateCacheOnly").toBool()) {
        displayFileBrowser(answer, previousSearchTerms, reply->property("loadBackButton").toBool());
    }

    QFile file(filePathName); // cache the json data
    if (file.exists()) {
        file.remove();
    }
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&file);
    out << QDateTime::currentSecsSinceEpoch() << '\n' << answer;
    file.close();
}

/** \brief Load Back Button If Required
 * \param m_loadBackButton display the back button in the file browser */
void MythApps::loadBackButtonIfRequired(bool m_loadBackButton) {
    if (m_loadBackButton) {
        m_fileListGrid->Reset();
        loadBackButton();
        m_loaderImage->SetVisible(false);
    }
}

/** \brief If clicking on a folder that displays all shows (allShowsFolderFound), load az search to allow the user to search by leter.
 *  \param hash hash of the answered contents from displayFileBrowser
 *  \return is the showAZsearch being displayed*/
bool MythApps::loadAZSearch(QString hash) {
    if ((!searching and allShowsFolderFound and currentSearchUrl.size() > 1) || azShowOnUrl.contains(hash) || overrideAppAZSearch(hash)) {
        loadProgram(tr("Shows A-Z"), createProgramData("", "Shows A-Z", "", false, ""), QString("file://") + ma_tv_filename, false);
        return true;
    }
    return false;
}

/** \brief Called from ReplyFinishedFileBrowser(). Parses the json and calls loadProgram, to display the data/directories in the gui file browser
 * \param answer json file browser data
 * \param previousSearchTerms Pass a list of 'directory names' or 'previous search terms' to hide in the file browser
 * \param m_loadBackButton display the back button in the file browser */
void MythApps::displayFileBrowser(QString answer, QStringList previousSearchTerms, bool m_loadBackButton) {
    LOG(VB_GENERAL, LOG_DEBUG, "displayFileBrowser() Start");
    toggleSearchVisible(false);
    bool alphabeticalFolderFound = false;

    QString hash = QString(QCryptographicHash::hash((answer.toUtf8()), QCryptographicHash::Md5).toHex());
    QByteArray br = answer.toUtf8(); // parse json
    QJsonDocument doc = QJsonDocument::fromJson(br);
    QJsonObject addons = doc.object();
    QJsonValue addons2 = addons["result"];
    QJsonObject o = addons2.toObject();

    loadBackButtonIfRequired(m_loadBackButton);
    bool showAZsearch = loadAZSearch(hash);

    int count = 0;
    for (auto oIt = o.constBegin(); oIt != o.constEnd(); ++oIt) {
        QJsonArray agentsArray = oIt.value().toArray();
        // loop throught each file/folder in the file browser
        foreach (const QJsonValue &v, agentsArray) {
            QString label = removeBBCode(v.toObject().value("label").toString());
            QString thumbnail = v.toObject().value("thumbnail").toString();
            QString plot = v.toObject().value("plot").toString(); // art
            QString file = v.toObject().value("filetype").toString();
            QString fileUrl = v.toObject().value("file").toString();

            if (searchNoDuplicateCheck.contains(fileUrl)) {
                return;
            }
            searchNoDuplicateCheck.append(fileUrl);

            if (searchStartLeter.compare("") != 0 and label.size() > 0) { // only display items starting with the corrosponding start
                                                                          // leter when using the a-z filter
                if (searchStartLeter.compare(QString(label.at(0)), Qt::CaseInsensitive) != 0) {
                    continue;
                }
            }

            if (label.compare("a", Qt::CaseInsensitive) == 0) { // does a folder named a leter exist.
                alphabeticalFolderFound = true;
            }

            if (folderAllowed(label,
                              previousSearchTerms)) { // if not a banned folder such as logout
                                                      // etc, load / display folder
                bool playVideo = false;
                if (file.compare("file") == 0) {
                    playVideo = true;
                }
                loadProgram(label, createProgramData(fileUrl, plot, thumbnail, playVideo, ""), thumbnail, false);

                if (searching and isHome) { // return max of 5 items when using global search
                    if (count >= 4) {
                        return;
                    }
                }
                count++;
            }

            discoverSearchURLs(label,
                               v.toObject().value("file").toString()); // discover if the directory is a search source url
        }
    }

    // if alphabetical folders are natively supported by the app, remove the
    // mythapps az search as we dont need duplicate az search functionality
    if (alphabeticalFolderFound and showAZsearch) {
        m_fileListGrid->RemoveItem(m_fileListGrid->GetItemAt(1));
    } else if (showAZsearch) {
        azShowOnUrl.append(hash); // used to restore az search for back button
    }

    allShowsFolderFound = false;
    searchStartLeter = "";

    if (stopScroll) { // used for next page
        m_loaderImage->SetVisible(false);
        stopScroll = false;
    }
    LOG(VB_GENERAL, LOG_DEBUG, "displayFileBrowser() Finished");
}

/** \brief discover if the directory is a search url. Used by the searchbox to pass the search term to the searching url
 * \param label - directory label
 * \param  file - directory path */
void MythApps::discoverSearchURLs(QString label, QString file) {
    QString searchUrl;
    QString searchUrl2;

    if (!QListContains(searchListLink->getList(), file)) { // if search url not discovered,

        if (label.compare("Search") == 0) {
            searchUrl = file;                       // url to send the search request.
            QString newSearch = getNewSearch(file); // some apps require a second search url to receive the search request

            if (!newSearch.isNull()) { // if app has a secondary search url
                searchUrl = newSearch;
                searchUrl2 = file;
            }
        } else if (label.compare("Quick Search (Incognito)") == 0) { // Incognito does not have a newSearch url as search requests are not saved.
            searchUrl = file;
        }
    }

    if (label.compare("Search") == 0 || label.compare("Quick Search (Incognito)") == 0) { // if app has a search feature, display the search box
        currentSearchUrl = QListSearch(searchListLink->getList(), file);
        toggleSearchVisible(true);
    }

    // combine the first and second search url into a deliminated 'currentSearchUrl' string if any urls have been found
    if (!searchUrl2.isNull()) {
        currentSearchUrl = searchUrl + "," + searchUrl2;
    } else if (!searchUrl.isNull()) {
        currentSearchUrl = searchUrl;
    }

    // save the currentSearchUrl for the global search
    if (!currentSearchUrl.contains("channel_id")) { // work-a-round for yt plugin when browsing subfolders after searching
        if (!searchUrl.isNull()) {
            searchListLink->append(currentSearchUrl); // save search source
        }

        if (label.compare(searchText) == 0) {
            searchSubFoldersList.append(file); // some apps return search results in subfolders
        }
    }
}

/** \brief return url search source for apps with "new search" button */
QString MythApps::getNewSearch(QString url) {
    QString searchDirectory = controls->getSearchDirectory(url, "New Search");
    if (searchDirectory.compare("") == 0) {
        searchDirectory = QString();
        handleDialogs(false);
    }
    return searchDirectory;
}

/** \brief Removes the last url from the previousURL list. Used for the back button.  */
void MythApps::removeCurrentUrlFromList() {
    if (previousURL.size() > 0) {
        previousURL.removeLast();
    }
}

/** \brief go back in the file browser. previousURL is a list of directories the user has clicked on */
void MythApps::goBack() {
    LOG(VB_GENERAL, LOG_DEBUG, "goBack() Start");
    foreach (QString item, previousURL) { LOG(VB_GENERAL, LOG_DEBUG, "-" + friendlyUrl(item)); }

    searchNoDuplicateCheck = QStringList();

    if (previousURL.contains("AZ Shows") and previousURL.last().compare("searchResults") == 0) {
        removeCurrentUrlFromList();
    }

    if (previousURL.contains("Unwatched")) {
        loadWatched(false);
        previousURL = QStringList();
        return;
    }

    if (musicOpen and !m_SearchTextEdit->IsVisible()) {
        LOG(VB_GENERAL, LOG_DEBUG, "musicOpen");
        loadMusic();
        return;
    }

    if (previousURL.contains("playingVideo")) {
        LOG(VB_GENERAL, LOG_DEBUG, "playingVideo");
        removeCurrentUrlFromList();
        removeCurrentUrlFromList();
    }

    if (previousURL.contains("Links")) {
        loadApps();
        return;
    }

    // go back to the first page
    bool remove = false;
    foreach (QString url, previousURL) {
        if (url.contains("Next Page") || remove) {
            LOG(VB_GENERAL, LOG_DEBUG, "Next Page");
            previousURL.removeAll(url);
            remove = true;
        }
    }

    removeCurrentUrlFromList();

    if (previousURL.size() > 0 and previousURL.last().compare("searchResults") == 0) { // we cant natigate to search results
        LOG(VB_GENERAL, LOG_DEBUG, "searchResults");
        removeCurrentUrlFromList();
    }

    // get the previous url
    if (previousURL.size() > 0) {
        requestFileBrowser("plugin://" + friendlyUrl(previousURL.last()), QStringList(), true, "");
        m_filepath->SetText(previousURL.last());
    } else {
        loadApps(); // go to home
    }
    LOG(VB_GENERAL, LOG_DEBUG, "goBack() Finished");
}

/** \brief refresh the currrent page wihtout cache*/
void MythApps::refreshPage(bool enableDialog) {
    LOG(VB_GENERAL, LOG_DEBUG, "refreshPage()");
    searchNoDuplicateCheck = QStringList();
    if (previousURL.size() > 0 and previousURL.last().contains("plugin")) {
        if (enableDialog) {
            createAutoClosingBusyDialog(tr("Refreshing Page"), 3);
        }
        requestFileBrowser("plugin://" + friendlyUrl(previousURL.last()), QStringList(), true, "refresh");
    }
}

/** \brief gets the playback time of the playing media and updates the track progress bar in the music player gui
 * \param playerid - The kodi player id
 * \return  returns hours, minutes and seconds */
QVariantMap MythApps::getPlayBackTime(int playerid) {
    QVariantMap map1 = controls->getPlayBackTime(playerid);
    QVariantMap map2 = map1["time"].toMap();

    m_trackProgress->SetUsed(map1["percentage"].toInt());

    updateMusicPlayingBarStatus();
    return map2;
}

/** \return the playback time of the playing media as a string */
QString MythApps::getPlayBackTimeString() {
    QString playback = "00:00:00";
    if (!playBackTimeMap["hours"].isNull()) {
        QString hours = playBackTimeMap["hours"].toString();
        QString minutes = playBackTimeMap["minutes"].toString();
        QString seconds = playBackTimeMap["seconds"].toString();

        if (hours.size() == 1) {
            hours = "0" + hours;
        }
        if (minutes.size() == 1) {
            minutes = "0" + minutes;
        }
        if (seconds.size() == 1) {
            seconds = "0" + seconds;
        }

        playback = hours + ":" + minutes + ":" + seconds;
    }
    return playback;
}

/** \brief play the video
 * \param filePathParam url of the video*/
void MythApps::play(QString mediaLocation) {
    m_plot->SetText("Play");
    controls->play(mediaLocation);
}

/** \brief toggle kodi fullscreen */
void MythApps::toggleFullscreen() {
#ifdef __ANDROID__
    return;
#endif
    controls->inputActionHelper("togglefullscreen");
}

/** \brief make kodi go fullscreen with verification */
void MythApps::goFullscreen() {
    LOG(VB_GENERAL, LOG_DEBUG, "goFullscreen()");

    if (musicOpen) {
        controls->activateWindow("visualer");
    } else {
        controls->activateWindow("screensaver"); // hides the kodi gui
    }

    minimizeTimer->stop();      // stops minimizing kodi
    SetFocusWidget(m_filepath); // hack to make the seek work in fullscreen as the remote requires focus
    QString fullscreenStatus = controls->isFullscreen();

    for (int i = 0; i < 3; i++) {
        if (fullscreenStatus.compare("Connection refused") != 0) {
            break;
        }
        delayMilli(600);
    }

#ifdef __ANDROID__
    if (!controls->androidAppSwitch("Kodi")) {
        createAutoClosingBusyDialog(tr("Myth Apps Services (anroid app - apk) is either not installed or "
                                       "running. Have you opened the app?"),
                                    3);
        niceClose(true);
    }
#elif _WIN32
    wchar_t kodi_wchar[] = L"Kodi";
    ShowWindow(FindWindow(NULL, kodi_wchar), SW_SHOWMAXIMIZED);
    delayMilli(100);
    toggleFullscreen();
#else
    for (int i = 0; i < 2; i++) {
        if (controls->isFullscreen().compare("1")) { // fullscreen toggle from fullscreen to window mode and then full screen to bring kodi to the front of mythtv.
            delayMilli(50);
            toggleFullscreen();
            delayMilli(20);
            toggleFullscreen();
        } else { // window
            toggleFullscreen();
        }
        delayMilli(100);
        if (controls->isFullscreenBool()) {
            break;
        } else {
            delayMilli(200);
        }
    }
#endif
}

/** \brief minimizes kodi by toggling out of fullscreen and usinga xdotool command. Suggested to call multible times for reliability incase kodi drops a request */
void MythApps::goMinimize(bool fullscreenCheck) {
    if (!allowAutoMinimize) {
        return;
    }
    if (fullscreenCheck) {
        if (controls->isFullscreenBool()) {
            toggleFullscreen();
        }
    }
#ifdef _WIN32
    wchar_t kodi_wchar[] = L"Kodi";
    ShowWindow(FindWindow(NULL, kodi_wchar), SW_MINIMIZE);
    delayMilli(150);
    wchar_t kodi_wchar2[] = L"MythApps";
    SetActiveWindow(FindWindow(NULL, kodi_wchar2));
#else
    delayMilli(150);
    system("timeout 2 xdotool search --onlyvisible --classname --sync Kodi "
           "windowminimize");
    system("kodi-send -a \"Minimize\"");
#endif
}

/** \brief close dialog if one pops up. e.g. for a subscription
 * \param  forceFullScreenVideo hide seekbar OSD when video first opens*/
void MythApps::handleDialogs(bool forceFullScreenVideo) {
    QString systemCurrentWindow = controls->handleDialogs();
    LOG(VB_GENERAL, LOG_DEBUG, "handleDialogs() -" + systemCurrentWindow);

    if (systemCurrentWindow.compare("Home") == 0 || forceFullScreenVideo) { // socket used as more efficient
        m_webSocket.sendTextMessage(QString("{\"jsonrpc\": \"2.0\", \"method\": \"GUI.ActivateWindow\", "
                                            "\"params\": { \"window\": \"fullscreenvideo\" }, \"id\": \"1\"}"));
    }
}

/** \brief open kodi if a dialog is on screen
 *  \param reply QNetworkReply from the file browser*/
void MythApps::handleSettingsDialogs(QNetworkReply *reply) {
    if (reply->isRunning()) {
        QString systemCurrentWindow = controls->handleDialogs();
        LOG(VB_GENERAL, LOG_DEBUG, "handleSettingsDialogs() -" + systemCurrentWindow);

        if (isSettingsDialog(systemCurrentWindow)) {
            if (allowAutoMinimize) {
                toggleAutoMinimize();
            }
            reply->close();
            while (isSettingsDialog(systemCurrentWindow)) {
                systemCurrentWindow = controls->handleDialogs();
                delay(2);
            }
            toggleAutoMinimize();
        }
    }
}

/** \brief reset the screenshot to nothing */
void MythApps::resetScreenshot() {
    m_screenshotMainQimage = QImage();
    m_screenshotMainMythImage->SetEnabled(false);
    m_screenshotMainMythImage->Reset();
}

/** \brief take a screenshot. Useful for getting a thumbnail for the currently playing video \return is the screenshot a black screen? */
bool MythApps::takeScreenshot() {
#ifdef __ANDROID__
    return false;
#endif
    if (!musicOpen) {
        m_screenshotMainQimage = screen->grabWindow(0).toImage().convertToFormat(QImage::Format_ARGB32);
        m_screenshotMainQimage.scaled(1920, 1080, Qt::KeepAspectRatio);

        QColor imageColor(m_screenshotMainQimage.pixel(600, 600));
        if (imageColor.red() == 0 and imageColor.green() == 0 and imageColor.blue() == 0) {
            LOG(VB_GENERAL, LOG_DEBUG, "Screenshot is blank ");
            return false;
        }

        delayMilli(200);
        return true;
    }
    return false;
}

/** \brief toggle the pause status */
void MythApps::pauseToggle() {
    int activePlayerStatus = controls->getActivePlayer();
    bool paused = false;

    if (!controls->isPaused(activePlayerStatus)) {
        takeScreenshot();
        paused = true;
    }
    controls->pauseToggle(activePlayerStatus);

    if (!paused) {
        controls->showOSD();
        controls->showOSD();
        handleDialogs(true);
    }
    delayMilli(200); // button debounce
}

/** \brief stop playing the video from the remote */
int MythApps::stopPlayBack() {
    LOG(VB_GENERAL, LOG_DEBUG, "stopPlayBack()");
    takeScreenshot();
    return controls->stopPlayBack();
}

/** \brief unpause the video */
void MythApps::unPause() {
    pausedMenu = false;
    minimizeTimer->stop();
    if (!musicOpen) {
        goFullscreen();
        kodiPlayerOpen = true;
    }
    pauseToggle();
}

/** \brief get the stream bitrate. Uses thirdparty program vnstat */
QString MythApps::getStreamBitrate() {
    QFile vnstatFile(globalPathprefix + "vnstat.txt");
    QString details = "";

    if (vnstatFile.open(QIODevice::ReadOnly)) {
        QString vnstat;
        QTextStream in(&vnstatFile);
        vnstat = vnstatFile.readAll();

        QJsonDocument jsonDocument2 = QJsonDocument::fromJson(vnstat.toLocal8Bit().data());
        QJsonObject jObject2 = jsonDocument2.object();
        QVariantMap map9 = jObject2.toVariantMap();

        QVariantMap map10 = map9["rx"].toMap();
        details = "Bitrate: " + map10["ratestring"].toString();
    }
    vnstatFile.close();
    return details;
}

/** \brief is the video playing? will retry if its not.
 * \param retryNo - how many times to retry? */
int MythApps::isPlaying(int retryNo) {
    QString playStatus = controls->isPlaying();

    static int count = 0;
    if (playStatus.compare("Connection refused") == 0) {
        if (retryNo > 2) {
            count++;
            LOG(VB_GENERAL, LOG_WARNING, "isPlaying - no kodi running?");

            if (count == 3) {
                niceClose(true);
            }
            return 2;
        } else {
            delayMilli(500);
            return isPlaying(retryNo + 1);
        }
    } else {
        count = 0;
    }

    if (playStatus.contains("internal")) {
        return 1;

    } else if (playStatus.contains("jsonrpc")) {
        return 0;
    } else {
        LOG(VB_GENERAL, LOG_WARNING, "playStatus: no result? " + playStatus);

        if (retryNo > 2) {
            LOG(VB_GENERAL, LOG_WARNING, "isPlaying - no kodi running? -2");
            return 2;
        } else {
            delayMilli(500);
            return isPlaying(retryNo + 1);
        }
        return 1;
    }
}

/** \brief runs when clicking the button/image. Used to load the next directory, file, link or app
 \param  item - the mythui button */
void MythApps::appsCallback(MythUIButtonListItem *item) {
    ProgramMap.clear();
    searching = false;
    isHome = false;

    ProgramData *programData = new ProgramData(item->GetData().toString());

    // if back button clicked
    if (programData->hasBack()) {
        goBack();
        return;
    }

    // open internal plugins for Favourites, Music, Watched List or Videos if clicked
    if (programData->hasArtists()) { // if music buttons clicked
        loadSongsMain(item->GetText("buttontext2"), "artists");
        return;
    } else if (programData->haAlbums()) {
        loadSongsMain(item->GetText("buttontext2"), "albums");
        return;
    } else if (programData->hasGenres() || item->GetText().contains("Genre]")) {
        loadSongsMain(item->GetText("buttontext2"), "genres");
        return;
    } else if (programData->hasPlaylists()) {
        loadSongsMain(programData->getPlot(), "playlists");
        return;
    }
    // launch web browser if clicking on a link
    if (programData->hasWeb()) {
        openBrowser(programData->getFilePathParam());
        browserOpen = true;
        SetFocusWidget(m_searchButtonList);
        return;
    }

    if (programData->hasFavourites()) { // Favouries
        loadFavourites(false);
        return;
    }

    if (programData->hasSearchShowsAZ()) {
        searchStartLeter = item->GetText("buttontext2");
        m_SearchTextEdit->SetText(searchStartLeter);
        goSearch(currentSearchUrl);
        return;
    }

    if (programData->hasShowsAZ()) { // Shows A-Z
        loadShowsAZ();
        return;
    }

    if (programData->hasMusic()) { // Music
        loadMusic();
        return;
    }
    if (programData->hasWatchedList()) { // Watched List
        m_fileListGrid->Reset();
        loadBackButton();
        loadWatched(false);
        return;
    }

    if (programData->hasUnwatchedList()) { // Unwatched List
        m_fileListGrid->Reset();
        loadBackButton();
        loadWatched(true);
        return;
    }

    if (programData->hasVideos()) { // Videos
        loadVideos();
        return;
    }

    // call function for any app directories clicked
    if (appsCallbackPlugins(programData, item)) {
        return;
    }
}

/** \brief handle any app directories clicked. Used to load the next directory or open a video
\param  item - the selected button */
bool MythApps::appsCallbackPlugins(ProgramData *programData, MythUIButtonListItem *item) {
    LOG(VB_GENERAL, LOG_DEBUG, "appsCallbackPlugins()");
    m_loaderImage->SetVisible(true);

    m_androidMenuBtn->SetVisible(false);
    m_androidMenuBtn->SetEnabled(false);

    QString fileURL = friendlyUrl(programData->getFilePathParam());
    m_filepath->SetText(fileURL);
    currentURL = urlencode(fileURL);

    if (!QString(programData->getFilePathParam().at(0)).compare("/") == 0) { // file system if first character is a slash
        fileURL = "plugin://" + fileURL;
    }

    bool loadBackButton = false;
    QString label = item->GetText("buttontext2");
    if (!label.contains(tr("Back to App")) and !programData->isPlayRequest()) {
        loadBackButton = true;
    }

    foreach (QString folder, showsAZfolderNames) {
        if (label.compare(folder, Qt::CaseInsensitive) == 0 and !folder.isEmpty()) { // used to turn on shows AZ.
            allShowsFolderFound = true;
        }
    }
    requestFileBrowser(fileURL, QStringList(), loadBackButton,
                       item->GetData().toString()); // request the url to load the next directory
    previousURL.append(fileURL);

    if (programData->isPlayRequest()) {
        if (programData->hasPlotandImageUrl()) {
            lastPlayedDetails = getLabel(item) + "~" + fileURL + "~" + programData->getPlot() + "~" + programData->getImageUrl();
        }

        QString seek = "00:00:00"; // default no seek
        if (programData->hasSeek()) {
            seek = programData->getSeek(); // retrive seek value if specified
        }

        m_loaderImage->SetVisible(false);
        play_Kodi(fileURL, seek);
        return true;
    }
    return false;
}

/** \brief the hover selected button/image. Used to update the selected title and plot.
 \param  item - the mythui button */
void MythApps::selectAppsCallback(MythUIButtonListItem *item) {
    m_title->SetText(removeBBCode(item->GetText("buttontext2")));
    currentselectionDetails = "";

    ProgramData *programData = new ProgramData(item->GetData().toString());

    if (programData->hasPlotandImageUrl()) {
        currentselectionDetails = getLabel(item) + "~" + item->GetData().toString();

        if (currentselectionDetails.endsWith("~", Qt::CaseInsensitive)) {
            currentselectionDetails.remove(currentselectionDetails.length() - 1, 1); // removes the last character
        }
    }

    m_plot->SetText("");

    if (programData->getPlot().size() > 30) {
        QString playUrl = getParm(item->GetData().toString(), 0);

        if (previouslyPlayedLink->contains(playUrl)) {
            isPreviouslyPlayed = true;
        } else {
            isPreviouslyPlayed = false;
        }

        m_plot->SetText(removeBBCode(programData->getPlot()));
    }

    if (stopScroll) {
        m_fileListGrid->SetItemCurrent(nextPagePosm_fileListGrid - 1);
    }
}

/** \brief get the current label text
 * \param  item - the mythui button */
QString MythApps::getLabel(MythUIButtonListItem *item) {
    QString label = item->GetText("buttontext2");
    if (label.size() == 0) {
        label = item->GetText("buttontextWatched");
    }
    return label;
}

/** \brief single fire slot used to call the next page and displays a loading message with timeout when required */
void MythApps::nextPageTimerSlot() {
    appsCallback(nextPageitem);
    nextPageitem->SetData(createProgramData(tr("Back"), "", "", false, ""));
    delayWhileStopScroll(2);
    if (stopScroll) {
        createBusyDialog(tr("Loading Next Page"));
        delayWhileStopScroll(8); // timeout message and the stopscroll incase it gets hung
        closeBusyDialog();
        stopScroll = false;
        m_loaderImage->SetVisible(false);
    }
}

/** \brief set a max delay when the stopscroll varibles is true
 * \param maxDelay - max seconds to delay for */
void MythApps::delayWhileStopScroll(int maxDelay) {
    int delayCount = 0;
    while (stopScroll) {
        delay(1);
        delayCount++;
        if (delayCount == maxDelay) {
            return;
        }
    }
}

/** \brief runs when the button is visible on screen. Used to automatically request the next page to allow infinite scroll.
 \param item - the mythui button */
void MythApps::visibleAppsCallback(MythUIButtonListItem *item) {
    displayImage(item, m_fileListGrid);

    if (item->GetText("buttontext2").contains("Next Page")) {
        item->SetText(tr("Back to App"), "buttontext2");
        nextPageitem = item;
        stopScroll = true;
        waitForThreads(0);
        waitforRequests();
        previousURL.append("Next Page");
        nextPageTimer->start(50); // run this code is another single run 'timer thread' to avoid stacking/blocking visibleAppsCallback

        nextPagePosm_fileListGrid = m_fileListGrid->GetCurrentPos();
        LOG(VB_FILE, LOG_DEBUG, "visibleAppsCallback finished");
    }
}

/** \brief Called from visibleAppsCallback. Creates a thread to download and process the image.
 * \param item The mythui button
 * \param m_fileList The file browser */
void MythApps::displayImage(MythUIButtonListItem *item, MythUIButtonList *m_fileList) {
    // get thread data from the visible onscreen button/app in the file list
    if (item->GetData().userType() == QMetaType::QStringList) {
        QStringList threadInfo = item->GetData().toStringList();
        QString type = threadInfo.at(0);
        QString thumbnailPath = threadInfo.at(1);
        QString fileName = threadInfo.at(2);
        QString fav_filename2 = threadInfo.at(3);
        QString setdata = threadInfo.at(4);
        int buttonPosition = m_fileList->GetItemPos(item);

        QThread *_thread = new QThread(); // create image thread in the imageThreadList pool to download / process the image.
        ImageThread *_rdr = new ImageThread(buttonPosition, thumbnailPath, urlencode(thumbnailPath), fileName, fav_filename2, username, password, ip, port, m_fileList);
        _rdr->moveToThread(_thread);
        connect(_thread, &QThread::started, _rdr, &ImageThread::startRead);
        connect(_thread, &QThread::finished, _rdr, &ImageThread::deleteLater);
        connect(_rdr, SIGNAL(renderImage(int, MythUIButtonList *)), _thread, SLOT(quit())); // tried terminate() also
        connect(_rdr, SIGNAL(renderImage(int, MythUIButtonList *)), this, SLOT(handleImageSlot(int, MythUIButtonList *)));
        _thread->start();
        imageThreadList.append(_thread);

        item->SetData(setdata + "*_" + fileName + ".processed");
    }
}

/** \brief send a request to load the clicked directory. A cache is used to speed up the requests
 * \param  url - url of the clicked directory. */
void MythApps::requestFileBrowser(QString url, QStringList previousSearches, bool loadBackButton, QString itemData) {
    LOG(VB_GENERAL, LOG_DEBUG, "requestFileBrowser()");

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QUrl qurl("http://" + ip + ":" + port + "/jsonrpc");
    qurl.setUserName(username);
    qurl.setPassword(password);
    request.setUrl(qurl);

    QJsonDocument doc(controls->getDirectoryObject(url));
    QByteArray data = doc.toJson();

    QString urlID = QString(QCryptographicHash::hash((url.toUtf8()), QCryptographicHash::Md5).toHex());
    QString filePathName = globalPathprefix + "00cache/" + urlID + ".cache";
    QFile cacheFile(filePathName);
    bool useCache = true;
    QString cacheData = "";
    QString noCacheReason = "";

    // if over a day , request update, also check there is no videos and more than one data.
    int time = 0;
    if (cacheFile.open(QIODevice::ReadOnly)) {
        int lineNO = 0;
        QTextStream in(&cacheFile);

        // cache logic
        while (!in.atEnd()) {
            if (lineNO == 0) {
                time = in.readLine().toInt();
                if (QDateTime::currentSecsSinceEpoch() > time + 864000) { // if date is over 10 days ago.
                    cacheFile.remove();
                    useCache = false;
                    break;
                }
            }
            cacheData = cacheData + in.readLine();
            lineNO++;
        }

        if (itemData.contains("(On Home Screen) ") || itemData.compare("refresh") == 0) {
            cacheFile.remove();
            useCache = false;
            noCacheReason.append("on home screen / refresh flag, ");
        }

        if (cacheData.contains("filetype\":\"file") and QDateTime::currentSecsSinceEpoch() > time + 86400) {
            useCache = false;
            noCacheReason.append("the json contains a video file and the cache is over a day old, ");
        }

        if (cacheData.count("directory") < 3) { // if cache contains less than 3 directories, turn off the cache.(to help avoid caching dynamic content)
            cacheFile.remove();
            useCache = false;
            noCacheReason.append("contains less than 3 directories, ");
        }

        // if contains dynamic words such as login, dont cache as this.
        QStringList wordList = {"Login", "Logout", "login", "logout", "Profile", "Live", "Popular"};
        QStringListIterator words(wordList);
        while (words.hasNext()) {
            QString word = words.next().toLocal8Bit().constData();
            if (cacheData.contains(word, Qt::CaseSensitive)) {
                cacheFile.remove();
                useCache = false;
                noCacheReason.append("contains dynamic words: " + word);
            }
        }
    } else {
        useCache = false;
    }

    if (useCache and !searching) { // use cache
        LOG(VB_GENERAL, LOG_INFO, "Using cache -" + filePathName);
        displayFileBrowser(cacheData, QStringList(), loadBackButton);

        if (QDateTime::currentSecsSinceEpoch() > time + 86400) { // if date is over one day ago, load first and then update anyway to keep cache current
            delayMilli(200);                                     // dealy to give time for the page to load before updating cache.
            cacheFile.remove();
            LOG(VB_GENERAL, LOG_INFO, "updating cache as over a day old");
            QNetworkReply *reply = managerFileBrowser->post(request, data);
            reply->setProperty("previousSearchTerms", previousSearches);
            reply->setProperty("filePathName", filePathName);
            reply->setProperty("updateCacheOnly", true);
            reply->setProperty("loadBackButton", loadBackButton);
            reply->deleteLater();
        }
    } else { // no cache
        LOG(VB_GENERAL, LOG_INFO, "No cache ** Reason: " + noCacheReason + " -" + filePathName);
        QNetworkReply *reply = managerFileBrowser->post(request, data);
        reply->setProperty("previousSearchTerms", previousSearches);
        reply->setProperty("filePathName", filePathName);
        reply->setProperty("updateCacheOnly", false);
        reply->setProperty("loadBackButton", loadBackButton);

        QTimer *handleSettingsDialogTimer = new QTimer(this); //  checking what dialogs are open
        handleSettingsDialogTimer->setSingleShot(true);
        connect(handleSettingsDialogTimer, &QTimer::timeout, [=]() {
            handleSettingsDialogs(reply);
            reply->deleteLater();
        });
        handleSettingsDialogTimer->start(1500);
        handleSettingsDialogTimer->deleteLater();
    }
    cacheFile.close();
}

/** \brief load the favourites list */
void MythApps::loadFavourites(bool home) {
    LOG(VB_GENERAL, LOG_DEBUG, "loadFavourites()");
    if (!home) {
        isFavouritesOpen = true;
        m_fileListGrid->Reset();
        loadBackButton();
        toggleSearchVisible(false);
        previousURL.append("Links");
        isHome = false;
    }

    favLink->load(); // reload incase another host has changed the state.

    QListIterator<QString> favourite(favLink->getList());
    favourite.toBack();
    while (favourite.hasPrevious()) {
        QStringList favouriteItem = favourite.previous().split("~");

        if (favouriteItem.size() > 3) {
            QString label = favouriteItem.at(0);

            QString url = favouriteItem.at(1);
            QString plot = favouriteItem.at(2);
            QString image = favouriteItem.at(3);

            if (plot.contains("(On Home Screen) ")) {
            } else if (home) {
                continue;
            }

            bool play = false;
            if (favouriteItem.size() > 4) {
                if (favouriteItem.at(4).compare("play") == 0) {
                    play = true;
                }
            }

            loadProgram(label, createProgramData(url, plot, image, play, ""), image, false);
        }
    }
}

/** \brief load the watched list */
void MythApps::loadWatched(bool unwatched) {
    LOG(VB_GENERAL, LOG_DEBUG, "loadWatched()");
    m_fileListGrid->Reset();
    loadBackButton();
    toggleSearchVisible(false);
    watchedLink->load(); // reload incase another host has changed the state.

    if (unwatched) {
        previousURL.append("Unwatched");
    } else {
        loadImage(m_fileListGrid, QString(tr("Unwatched") + watchedLink->getUnWatchedSize()), QString("Unwatched~Unwatched"), recent_filename);
    }

    QListIterator<QString> watched(watchedLink->getList());
    watched.toBack();
    while (watched.hasPrevious()) {
        QStringList watchedItem = watched.previous().split("~");

        if (watchedItem.size() > 5) {
            QString label = watchedItem.at(0);
            QString url = watchedItem.at(1);
            QString plot = watchedItem.at(2);
            QString image = watchedItem.at(3);

            bool play = false;
            if (watchedItem.at(4).compare("play") == 0) {
                play = true;
            }

            QString seek = watchedItem.at(5);
            if (seek.compare("false") == 0 and !unwatched) {
                seek = "";
                continue;
            } else if (!seek.compare("false") == 0 and unwatched) {
                continue;
            }

            loadProgram(label, createProgramData(url, plot, image, play, seek), image, false);
        }
    }
}

/** \brief load the myvideos app */
void MythApps::loadVideos() {
    LOG(VB_GENERAL, LOG_DEBUG, "loadVideos()");
    m_fileListGrid->Reset();
    loadBackButton();
    toggleSearchVisible(false);
    previousURL.append("Links");
    isHome = false;

    musicMode = 2;
    if (gCoreContext->GetSetting("MythAppsMusic").compare("1") == 0) {
        loadImage(m_fileListGrid, tr("Music Videos"), QString("Music~Music"), music_filename);
    }

    QVariantMap map = controls->getVideos();
    QList list3 = map["sources"].toList();

    foreach (QVariant T, list3) { // load videos and folders in the file manager
        QVariantMap map4 = T.toMap();

        QString label = map4["label"].toString();
        QString url = map4["file"].toString();
        QString image = "";

        loadProgram(label, createProgramData(url, "", image, false, ""), image, false);
    }
}

/** \brief load shows AZ( */
void MythApps::loadShowsAZ() {
    LOG(VB_GENERAL, LOG_DEBUG, "loadShowsAZ()");
    m_fileListGrid->Reset();
    loadBackButton();
    toggleSearchVisible(false);
    previousURL.append("AZ Shows");

    QString alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    QString thumbnail = apps->getLocationFromUrlAddress(currentSearchUrl);

    for (int i = 0; i < alphabet.length(); i++) {
        loadProgram(QString(alphabet.at(i).toLatin1()), createProgramData("", "searchShowsAZ", "", false, ""), thumbnail, false); // currentSearchUrl
    }
}

/** \brief display the options menu */
void MythApps::showOptionsMenu() {
    LOG(VB_GENERAL, LOG_DEBUG, "showOptionsMenu()");
    MythScreenStack *popupStack = GetMythMainWindow()->GetStack("popup stack");
    m_menuPopup = new MythDialogBox(tr("Options"), popupStack, "mythappsmenupopup");

    if (m_menuPopup->Create()) {
        popupStack->AddScreen(m_menuPopup);
        m_menuPopup->SetReturnEvent(this, tr("options"));

        if (currentselectionDetails.compare("") != 0) {
            if (favLink->contains(currentselectionDetails)) {
                m_menuPopup->AddButton(tr("Remove Selection from Favourites"));
            } else {
                m_menuPopup->AddButton(tr("Add Selection to Favourites"));
            }

            if (favLink->isPinnedToHome(currentselectionDetails)) {
                m_menuPopup->AddButton(tr("Remove Selection from Home Screen"));
            } else if (favLink->contains(currentselectionDetails) and isFavouritesOpen) {
                m_menuPopup->AddButton(tr("Pin Selection to Home Screen"));
            }

            if (watchedLink->contains(currentselectionDetails)) {
                m_menuPopup->AddButton(tr("Remove from Watch List"));
            } else if (isPreviouslyPlayed) {
                m_menuPopup->AddButton(tr("Clear Previously Played"));
                if (currentselectionDetails.contains("~play")) {
                    m_menuPopup->AddButton(tr("Add to Watch List for Later Viewing"));
                }
            } else if (currentselectionDetails.contains("~play")) {
                m_menuPopup->AddButton(tr("Add to Watch List for Later Viewing"));
            }
        }

        if (musicOpen and m_playlistVertical->GetCount() > 0) {
            m_menuPopup->AddButton(tr("Remove all tracks from playlist"));
            if (GetFocusWidget() == m_playlistVertical) {
                m_menuPopup->AddButton(tr("Remove selected track from playlist"));
            }
        }
        if (isHome) {
            m_menuPopup->AddButton(tr("Settings"));
            m_menuPopup->AddButton(tr("Clear Cache"));
        }
        m_menuPopup->AddButton(tr("Exit to MythTV Menu"));

    } else {
        delete m_menuPopup;
        m_menuPopup = nullptr;
    }
}

/** \brief customEvent from the options menus. Used to connect buttons to actions.*/
void MythApps::customEvent(QEvent *event) {
    if (event->type() == DialogCompletionEvent::kEventType) {
        auto *dce = dynamic_cast<DialogCompletionEvent *>(event);
        if (dce == nullptr)
            return;

        QString resultid = dce->GetId();
        QString resulttext = dce->GetResultText();
        int buttonnum = dce->GetResult();

        if (resultid == "options") {
            if (resulttext == tr("Settings")) {
                runMythSettingsSlot();
            } else if (resulttext == tr("Clear Cache")) {
                // remove all files in the cache directory
                QDir dir(globalPathprefix + "00cache/");
                dir.removeRecursively();
                createDirectoryIfDoesNotExist(globalPathprefix + "00cache/");
            } else if (resulttext == tr("Clear Previously Played")) {
                QString playUrl = getParm(currentselectionDetails, 1);
                previouslyPlayedLink->removeOne(playUrl);
                setButtonWatched(false);
                refreshFileListGridSelection();
            } else if (resulttext == tr("Remove all tracks from playlist")) {
                controls->playListClear();
                m_playlistVertical->Reset();
                stopPlayBack();
            } else if (resulttext == tr("Remove selected track from playlist")) {
                removeFromPlaylist(m_playlistVertical->GetItemCurrent()->GetText());

            } else if (resulttext == tr("Exit to MythTV Menu")) {
                niceClose(false);
            } else if (resulttext == tr("Add Selection to Favourites")) {
                if (currentselectionDetails.contains("~play~")) { // remove seek time when saving to favourites
                    currentselectionDetails = currentselectionDetails.mid(0, currentselectionDetails.indexOf("~play~"));
                }
                favLink->append(currentselectionDetails); // add to favourites
            } else if (resulttext == tr("Remove Selection from Favourites")) {
                if (favLink->contains(currentselectionDetails)) {
                    favLink->removeOne(currentselectionDetails); // Remove from favourite
                    reload();
                }
            } else if (resulttext == tr("Remove Selection from Home Screen")) {
                favLink->removeFromHomeScreen(currentselectionDetails);
                reload();
            } else if (resulttext == tr("Pin Selection to Home Screen")) {
                favLink->addToHomeScreen(currentselectionDetails);
                reload();
            } else if (resulttext == tr("Remove from Watch List")) {
                watchedLink->listRemove(currentselectionDetails);

                m_fileListGrid->Reset();
                loadBackButton();
                loadWatched(false);
            } else if (resulttext == tr("Add to Watch List for Later Viewing")) {
                addToUnWatchedList(false);
            }
        }

        if (resultid == "watchList") {
            if (resulttext == tr("Flag as Watched and Exit to Menu")) {
                resetScreenshot();
                addToPreviouslyPlayed();
            } else if (resulttext == tr("Exit to Menu")) {
                resetScreenshot();
            } else if (resulttext == tr("Save Position to Watch List and Exit to Menu")) {
                watchedLink->append(lastPlayedDetails + "~play~" + getPlayBackTimeString());
                resetScreenshot();
                addToPreviouslyPlayed();
            } else if (resulttext == tr("Keep Watching")) {
                QStringList watchedItem = lastPlayedDetails.split("~");
                if (watchedItem.size() > 1) {
                    QString url = watchedItem.at(1);
                    play_Kodi(url, getPlayBackTimeString());
                }
            } else if (resulttext == tr("Exit to MythTV Menu")) {
                niceClose(false);
            } else {
                createPlayBackMenu();
            }
        }
        if (resultid == "fullscreen") {
            if (buttonnum == 1) {
                goFullscreen();
            }
        }

        m_menuPopup = nullptr;
    }
}

/** \brief reload either the home screen or favourites screen*/
void MythApps::reload() {
    if (isFavouritesOpen) {
        m_fileListGrid->Reset();
        loadBackButton();
        loadFavourites(false);
    } else {
        loadApps();
    }
}
/** \brief refresh the selection in the file list grid.  */
void MythApps::refreshFileListGridSelection() {
    int pos = m_fileListGrid->GetCurrentPos();
    m_fileListGrid->SetItemCurrent(0);
    m_fileListGrid->SetItemCurrent(pos);
}

/** \brief add to the watched list as unwatched for later viewing  */
void MythApps::addToUnWatchedList(bool menu) {
    LOG(VB_GENERAL, LOG_INFO, "addToUnWatchedList");
    if (currentselectionDetails.contains("~play")) {
        if (!watchedLink->contains(currentselectionDetails)) {
            watchedLink->append(currentselectionDetails + "~false");
            if (menu) {
                createAutoClosingBusyDialog(tr("Added to Watch List for Later Viewing"), 3);
            }
        } else {
            createAutoClosingBusyDialog(tr("Already in Watch List"), 3);
        }
    } else {
        createAutoClosingBusyDialog(tr("Unable to add to Watch List as not a video"), 3);
    }
}
/** \brief plays a video in kodi and seeks if required.
 * \param mediaLocation url of the video
 * \param seekAmount amount to seek in hours minutes seconds. Can be blank */
void MythApps::play_Kodi(QString mediaLocation, QString seekAmount) {
    LOG(VB_GENERAL, LOG_INFO, "play_Kodi(): " + mediaLocation + " seek: " + seekAmount);

    if (!previousURL.contains("playingVideo")) { // used for back button history
        previousURL.append("playingVideo");
    }

    if (gCoreContext->GetSetting("MythAppsInternalRemote").compare("1") == 0) {
        kodiPlayerOpen = true; // enable myth remote
    }

    minimizeTimer->stop();
    exitToMainMenuSleepTimer->stop();

    goFullscreen();

    QFile vnstatFile(globalPathprefix + "/vnstat.txt"); // used to record bitrate
    vnstatFile.remove();
    vnstatFile.close();

    play(mediaLocation); // play the media
    lastMediaLocation = mediaLocation;

    QTime dieTime = QTime::currentTime().addSecs(9); // check the media is playing
    while (isPlaying(0) == 0) {
        // not playing
        if (QTime::currentTime() > dieTime) { // sanity check incase loop gets stuck
                                              // for more than 9 seconds
            break;
        }
        delay(1);
    }

    if (isPlaying(0) == 1 and !seekAmount.compare("00:00:00") == 0) { // if playing and seek timestamp specified
        QStringList seekTime = seekAmount.split(":");
        if (seekTime.size() >= 2) {
            if (connected == 2) {
                controls->seek(seekTime.at(0).toInt(), seekTime.at(1).toInt(),
                               seekTime.at(2).toInt()); // seek to timestamp
                delay(2);
            }
        }
    }
    if (connected == 2) {
        controls->showOSD();
        controls->showOSD();
        handleDialogs(true);

        delayMilli(50);
        if (!takeScreenshot()) { // take a screenshot and retry if black screen
            delay(3);
            if (!takeScreenshot()) { // use thumbnail instead
                m_screenshotMainQimage.load(getStandarizedImagePath(getParm(lastPlayedDetails, 3)) + ".processed");
            }
        }

        globalActivePlayer = controls->getActivePlayer();
        playbackTimer->start(1 * 1000);

        streamDetails = "";
        if (isPlaying(0) == 1) {
            streamDetails = controls->getStreamDetailsAll(globalActivePlayer);
        }

        system(QString("vnstat -tr 7 --json >>" + globalPathprefix + "/vnstat.txt").toLocal8Bit().constData());
    }
    LOG(VB_GENERAL, LOG_DEBUG, "play_Kodi() Finsihed");
}

/** \brief Create menu list GUI prompt when video playback ends */
void MythApps::createPlayBackMenu() {
    LOG(VB_GENERAL, LOG_DEBUG, "createPlayBackMenu()");
    MythScreenStack *popupStack = GetMythMainWindow()->GetStack("popup stack");
    m_menuPopup = new MythDialogBox(tr("Options"), popupStack, "mythappsmenupopup");

    if (m_menuPopup->Create()) {
        popupStack->AddScreen(m_menuPopup);
        m_menuPopup->SetReturnEvent(this, "watchList");

        if (!previouslyPlayedLink->contains(lastMediaLocation) and !excludePreviouslyPlayed(lastMediaLocation)) {
            m_menuPopup->AddButton(tr("Flag as Watched and Exit to Menu"));
        }
        m_menuPopup->AddButton(tr("Exit to Menu"));
        m_menuPopup->AddButton(tr("Save Position to Watch List and Exit to Menu"));
        m_menuPopup->AddButton(tr("Keep Watching"));
        m_menuPopup->AddButton(tr("Exit to MythTV Menu"));
    } else {
        delete m_menuPopup;
        m_menuPopup = nullptr;
    }
}

/** \brief calls minimize kodi */
void MythApps::minimizeTimerSlot() { goMinimize(false); }

/** \brief closes any hung searchs */
void MythApps::searchTimerSlot() {
    closeBusyDialog();
    searchTimer->stop();
}

/** \brief decides what menu to open when media playback has finished
 * \param screenType Player.OnStop or Player.OnPause */
void MythApps::openOSD(QString screenType) {
    pausedMenu = true;
    goMinimize(true);
    kodiPlayerOpen = false;

    if (!m_screenshotMainQimage.isNull()) {
        MythImage *m_image = GetPainter()->GetFormatImage();
        m_image->Assign(m_screenshotMainQimage);
        m_screenshotMainMythImage->SetEnabled(true);
        m_screenshotMainMythImage->SetImage(m_image);
    }

    if (screenType.compare("Player.OnStop") == 0) {
        playbackTimer->stop();
        createPlayBackMenu();
        goMinimize(true);
        minimizeTimer->start();
#ifdef __ANDROID__
        controls->androidAppSwitch("MythTV");
#endif
    } else if (screenType.compare("Player.OnPause") == 0) { // 2
        goMinimize(true);
        minimizeTimer->start();

        MythScreenStack *mainStack = GetMythMainWindow()->GetMainStack();
        auto *mythOSD = new MythOSD(mainStack, "mythosd");

        if (mythOSD->Create()) {
            mainStack->AddScreen(mythOSD);
            mythOSD->setPlayBackTimeOSD(getPlayBackTimeString());
            mythOSD->waitforKey();
            mythOSD->closeWindow();
        } else {
            delete mythOSD;
        }
        unPause();
    }
}

/** \brief create the busy dialog
 * \param title lablel for the dialog*/
void MythApps::createBusyDialog(const QString &title) {
    if (m_busyPopup) {
        return;
    }

    MythScreenStack *popupStack = GetMythMainWindow()->GetStack("popup stack");
    m_busyPopup = new MythUIBusyDialog(title, popupStack, "mythvideobusydialog");

    if (m_busyPopup->Create()) {
        popupStack->AddScreen(m_busyPopup, false);
    } else {
        m_busyPopup = nullptr;
    }
}

/** \brief close the busy dialoge */
void MythApps::closeBusyDialog() {
    if (m_busyPopup) {
        m_busyPopup->Close();
        m_busyPopup = nullptr;
    }
}

/** \brief wait for the managerFileBrowser requests to complete */
void MythApps::waitforRequests() {
    while (true) {
        QList<QNetworkReply *> replyList = managerFileBrowser->findChildren<QNetworkReply *>();

        bool retrievingSearch = true;
        foreach (QNetworkReply *item, replyList) {
            if (item->isFinished()) {
                retrievingSearch = false;
                item->deleteLater();
            }
        }

        if (!retrievingSearch || replyList.size() == 0) {
            break;
        }
        delay(1);
    }
}

/** \brief return the focus to the filelist*/
void MythApps::returnFocus() {
    if (GetFocusWidget() == m_filepath) {
        if (m_fileListMusicGrid->IsVisible()) {
            SetFocusWidget(m_fileListMusicGrid);
        } else {
            SetFocusWidget(m_fileListGrid);
        }
    }
}

/** \brief used to micro delay opening mythsettings to avoid race conditions */
void MythApps::runMythSettingsSlot() {
    MythScreenStack *mainStack = GetMythMainWindow()->GetMainStack();
    auto *mythsettings = new MythSettings(mainStack, "mythsettings");

    if (mythsettings->Create()) {
        mainStack->AddScreen(mythsettings);
        niceClose(false);
        return;
    }
    delete mythsettings;
    niceClose(false);
}

/** \brief  set the button as watched/ unwatched in the gui. THe green text colour on the button means watched flag
 * \param watched shoud the button dispaly watched or unwatched coloured text */
void MythApps::setButtonWatched(bool watched) {
    if (watched) {
        QString btnText = m_fileListGrid->GetItemCurrent()->GetText("buttontext2");
        m_fileListGrid->GetItemCurrent()->SetText("", "buttontext2");
        m_fileListGrid->GetItemCurrent()->SetText(btnText, "buttontextWatched");
    } else {
        QString btnText = m_fileListGrid->GetItemCurrent()->GetText("buttontextWatched");
        m_fileListGrid->GetItemCurrent()->SetText("", "buttontextWatched");
        m_fileListGrid->GetItemCurrent()->SetText(btnText, "buttontext2");
    }
}

/** \brief add the previosulyPlayed Video to the previouslyPlayedLink. Used for watched/played flag shown in the gui with the green text colour on the button*/
void MythApps::addToPreviouslyPlayed() {
    previouslyPlayedLink->append(lastMediaLocation);
    setButtonWatched(true);
    refreshFileListGridSelection();
}

/** \brief display the options menu when the Android menu button is clicked*/
void MythApps::androidMenuBtnSlot() { showOptionsMenu(); }

/** \brief open the web browser
 * 	\param websiteUrl url of the website */
void MythApps::openBrowser(QString website) {
#ifdef __ANDROID__
    if (!website.contains("http")) {
        website = "http://" + website;
    }
    controls->androidAppSwitch(website);
#elif _WIN32
    system("start msedge " + website.toLocal8Bit());
#else
    if (system("command -v chromium >/dev/null 2>&1 || { exit 1; }") == 0) {
        system("chromium -kiosk " + website.toLocal8Bit() + " &");
    } else if (system("command -v firefox >/dev/null 2>&1 || { exit 1; }") == 0) {
        system("firefox --kiosk " + website.toLocal8Bit() + " &");
    } else {
        system("sensible-browser" + website.toLocal8Bit() + " &");
    }
#endif
}
