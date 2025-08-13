#include "mythapps.h"

// QT headers
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QTextStream>
#include <QtWidgets>

// MythTV headers
#include "libmythbase/mythlogging.h"
#include "libmythui/mythuistatetracker.h"
#include <libmyth/mythcontext.h>
#include <libmythui/mythdialogbox.h>
#include <libmythui/mythmainwindow.h>
#include <libmythui/mythprogressdialog.h>
#include <libmythui/mythuibutton.h>
#include <libmythui/mythuiimage.h>
#include <libmythui/mythuitext.h>

// MythApps headers
#include "imageThread.h"
#include "music_functions.cpp"
#include "mythapps.h"
#include "mythinput.h"
#include "mythsettings.h"
#include "netSocketRequest.h"
#include "searchSuggestions.h"
#include "shared.h"

#ifdef _WIN32
#include <iostream>
#include <windows.h>
#endif

static QDateTime gLastPluginUnloadTime;
QAtomicInteger<quint64> MythApps::currentLoadId{0};

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
    searchFocusTimer = new QTimer(this); // micro singleshot timer to delay the search suggestions after keypress.
    searchFocusTimer->setSingleShot(true);
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
    connect(searchFocusTimer, SIGNAL(timeout()), this, SLOT(searchFocusTimerSlot()));
    connect(nextPageTimer, SIGNAL(timeout()), this, SLOT(nextPageTimerSlot()));
    connect(openSettingTimer, SIGNAL(timeout()), this, SLOT(runMythSettingsSlot()));

    screen = QGuiApplication::primaryScreen();
}

/** \brief MythApps deconstructer to clean up timers on addon closing */
MythApps::~MythApps() {
    CleanupResources();
    gLastPluginUnloadTime = QDateTime::currentDateTime();
}

/** \brief Listens to OnPause,OnStop,OnPlay,OnInputRequested messages recieved from Kodi via the websocket.
 * \param kodi websocket message  */
void MythApps::onTextMessageReceived(const QString &method, const QString &message) {
    LOG(VB_GENERAL, LOG_DEBUG, "onTextMessageReceived():" + method);

    if (method == "Player.OnStop" && (musicOpen || isHome)) {
        musicBarOnStopTimer->start(500);
        if (controls->isFullscreen()) {
            goMinimize(true);
        }
    } else if (method == "Player.OnStop" && !musicOpen) {
        openOSD("Player.OnStop");
        returnFocus();
        exitToMainMenuSleepTimer->start();

    } else if (method == "Player.OnPlay" && musicOpen) {
        musicBarOnStopTimer->stop();

        updateMusicPlayingBarStatus(); // update the music status bar
        if (message.contains("video")) {
            if (!controls->isFullscreen()) {
                confirmDialog(tr("Do you want to open this video in fullscreen?"), "fullscreen");
            }
        }
    } else if (method == "Input.OnInputRequested" and !searching) {
        displayInputBox(message);
    } else if (method == "Player.OnAVStart" && !musicOpen) {
        handleDialogs(true);
        streamDetails = controls->getStreamDetailsAll();

        QTimer::singleShot(2000, this, [this]() { takeScreenshot(); });
    }

    handlePlaybackEvent(method, message);
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

PluginManager MythApps::pluginManager;

/** \brief create the main screen and initialize the setting and state*/
bool MythApps::Create() {
    LOG(VB_GENERAL, LOG_INFO, "Opening MythApp");
    coolDown();

    username = QString(gCoreContext->GetSetting("MythAppsusername"));
    password = QString(gCoreContext->GetSetting("MythAppspassword"));
    ip = QString(gCoreContext->GetSetting("MythAppsip"));
    port = QString(gCoreContext->GetSetting("MythAppsport"));

    controls = new Controls(ip, port);
    connect(controls, &Controls::loadProgramSignal, this, &MythApps::loadProgramSlot);

    showsAZfolderNames = gCoreContext->GetSetting("MythAppsShowsAZfolderNames").split("~");

    browser = new Browser(controls);
    connect(browser, SIGNAL(setFocusWidgetSignal(QString)), this, SLOT(setFocusWidgetSlot(QString)));

    fileBrowserHistory = new FileBrowserHistory();

    bool foundtheme = false;
    QString theme = gCoreContext->GetSetting("Theme");
    QString xmlFile = GetThemeXmlFile(theme);
    LOG(VB_GENERAL, LOG_INFO, "Loading " + xmlFile);
    foundtheme = LoadWindowFromXML(xmlFile, "mythapps", this);

    if (!foundtheme)
        return false;

    bool err = false;
    uiCtx = new UIContext();

    UIUtilE::Assign(this, uiCtx->plot, "plot", &err);
    UIUtilE::Assign(this, uiCtx->streamDetails, "streamDetails", &err);
    UIUtilE::Assign(this, uiCtx->streamDetailsbackground, "streamDetailsbackground", &err);
    UIUtilE::Assign(this, uiCtx->filepath, "filepath", &err);
    UIUtilE::Assign(this, uiCtx->title, "title", &err);
    UIUtilE::Assign(this, uiCtx->fileListGrid, "fileListGrid", &err); // main file browser used to display the apps.
    UIUtilE::Assign(this, uiCtx->screenshotMainMythImage, "screenshotImageMain", &err);
    UIUtilE::Assign(this, uiCtx->SearchTextEdit, "SearchTextEdit", &err);
    UIUtilE::Assign(this, uiCtx->SearchTextEditBackgroundText, "SearchTextEditBackgroundText", &err);
    UIUtilE::Assign(this, uiCtx->searchButtonList, "searchButtonList", &err);
    UIUtilE::Assign(this, uiCtx->searchSettingsButtonList, "searchSettingsButtonList", &err);

    UIUtilE::Assign(this, uiCtx->loaderImage, "loaderImage", &err);
    UIUtilE::Assign(this, uiCtx->searchButtonListGroup, "searchButtonListGroup", &err);
    UIUtilE::Assign(this, uiCtx->searchSettingsGroup, "searchSettingsGroup", &err);
    UIUtilE::Assign(this, uiCtx->androidMenuBtn, "androidMenuBtn", &err);
    UIUtilE::Assign(this, uiCtx->help, "help", &err);

    dialog = new Dialog(this, uiCtx->loaderImage);

    MythUIButtonListItem *searchBtnList = new MythUIButtonListItem(uiCtx->searchButtonList, "");
    searchBtnList->SetText("Search", "buttontext2");

    bool err2 = initializeMusic(); // music
    if (err || err2) {
        LOG(VB_GENERAL, LOG_ERR, "Cannot load screen 'mythapps'");
        return false;
    }

    setWidgetVisibility(uiCtx->androidMenuBtn, false);
    setWidgetVisibility(uiCtx->help, false);
    uiCtx->SearchTextEdit->SetKeyboardPosition(VK_POSTOPDIALOG);

    connect(uiCtx->fileListGrid, SIGNAL(itemClicked(MythUIButtonListItem *)), this, SLOT(appsCallback(MythUIButtonListItem *)));
    connect(uiCtx->fileListGrid, SIGNAL(itemSelected(MythUIButtonListItem *)), this, SLOT(selectAppsCallback(MythUIButtonListItem *)));
    connect(uiCtx->fileListGrid, SIGNAL(itemVisible(MythUIButtonListItem *)), this, SLOT(visibleAppsCallback(MythUIButtonListItem *)));

    connect(uiCtx->androidMenuBtn, SIGNAL(Clicked()), this, SLOT(androidMenuBtnSlot()));

    connect(uiCtx->searchButtonList, SIGNAL(itemClicked(MythUIButtonListItem *)), this, SLOT(clickedSearchList(MythUIButtonListItem *)));
    connect(uiCtx->searchButtonList, SIGNAL(itemSelected(MythUIButtonListItem *)), this, SLOT(selectSearchList(MythUIButtonListItem *)));
    connect(uiCtx->searchSettingsButtonList, SIGNAL(itemClicked(MythUIButtonListItem *)), this, SLOT(searchSettingsClicked(MythUIButtonListItem *)));

    connect(uiCtx->SearchTextEdit, SIGNAL(valueChanged()), this, SLOT(searchTextEditValueChanged()));
    connect(uiCtx->SearchTextEdit, SIGNAL(LosingFocus()), this, SLOT(searchTextEditLosingFocus()));
    connect(uiCtx->SearchTextEdit, SIGNAL(TakingFocus()), this, SLOT(searchTextEditValueChanged()));
    connect(uiCtx->searchButtonList, SIGNAL(LosingFocus()), this, SLOT(searchTextEditLosingFocus()));

    controls->startKodiIfNotRunning();
    controls->waitUntilKodiPingable();
    goMinimize(true);
    controls->initializeWebSocket();
    connect(controls->netSocketRequest.data(), &NetSocketRequest::receivedFromSocket, this, &MythApps::onTextMessageReceived);

    createDirectoryIfDoesNotExist(getGlobalPathPrefix() + "00cache/"); // Setup cache directories

    // load icons
    mm_albums_icon = createImageCachePath("ma_mm_albums.png");
    mm_alltracks_icon = createImageCachePath("ma_mm_alltracks.png");
    mm_artists_icon = createImageCachePath("ma_mm_artists.png");
    mm_genres_icon = createImageCachePath("ma_mm_genres.png");
    mm_playlist_icon = createImageCachePath("ma_mm_playlists.png");
    music_icon = createImageCachePath("ma_music.png");
    back_icon = createImageCachePath("ma_mv_gallery_dir_up.png");
    ma_tv_icon = createImageCachePath("ma_tv.png");
    ma_popular_icon = createImageCachePath("ma_popular.png");
    ma_search_icon = createImageCachePath("ma_search.png");

    previouslyPlayedLink = new ProgramLink("previouslyPlayed"); // load previously played videos
    searchListLink = new ProgramLink("searchList");             // load Search Sources

    currentSelectionDetails = new ProgramData("", "");
    lastPlayedDetails = new ProgramData("", "");

    uiCtx->streamDetailsbackground->Hide();

    pluginManager.setLoadProgramCallback([this](const QString &name, const QString &setdata, const QString &thumbnailUrl) { this->loadProgram(name, setdata, thumbnailUrl); });
    pluginManager.setToggleSearchVisibleCallback([this](bool visible) { this->toggleSearchVisible(visible); });
    pluginManager.setGoBackCallback([this]() { this->goBack(); });
    pluginManager.setFocusWidgetCallback([this](MythUIType *widget) { this->SetFocusWidget(widget); });
    pluginManager.setPlay_KodiCallback([this](QString mediaLocation) { play_Kodi(mediaLocation, "00:00:00"); });

    pluginManager.setControls(controls);
    pluginManager.setDialog(dialog);
    pluginManager.setUIContext(uiCtx);

    loadApps();

#ifdef _WIN32
    delayMilli(400);
    wchar_t kodi_wchar[] = L"MythApps";
    SetActiveWindow(FindWindow(NULL, kodi_wchar));
#endif
    LOG(VB_GENERAL, LOG_DEBUG, "Create() Finished. Threads: " + QString::number(QThread::idealThreadCount()) + ", 4+ recommended");

    return true;
}

/** \brief create and initialize the main app screen*/
void MythApps::loadApps() {
    LOG(VB_GENERAL, LOG_DEBUG, "loadApps() Start");
    // Initialize the load apps state
    isHome = true;
    browser->setOpenStatus(false);
    currentSearchUrl = "";
    showMusicUI(false);
    azShowOnUrl.clear();
    delayMilli(5);
    toggleSearchVisible(true);
    m_hint->SetVisible(false);
    dialog->getLoader()->SetVisible(false);
    setWidgetVisibility(uiCtx->searchSettingsGroup, false);
    musicMode = 1;
    resetScreenshot();
    firstDirectoryName = tr("All");
    controls->resetActivePlayer();

#ifdef __ANDROID__ // display a button to bring up the menu on a touch screen
    uiCtx->androidMenuBtn->SetVisible(true);
    uiCtx->androidMenuBtn->SetEnabled(true);
#endif

    playbackTimer->stop();
    partyMode = false;
    uiCtx->fileListGrid->Reset();
    makeSearchTextEditEmpty();

    if (musicOpen) {
        clearAndStopPlaylist();
    }
    musicOpen = false;
    uiCtx->filepath->SetText("");

    if (controls->getConnected() == 1) {
        if (!controls->isUserNamePasswordCorrect()) {
            delayMilli(400); // retry incase slow to start
            if (!controls->isUserNamePasswordCorrect()) {
                dialog->createAutoClosingBusyDialog(tr("authentication error. please check kodi username/password in settings. (setting m key)"), 6);
                openSettingTimer->start(200);
                return;
            }
        } else if (!controls->areAddonsInstalled()) {
            uiCtx->filepath->SetText(tr("no video addons installed. press f3 to open kodi and install some addons."));
            LOG(VB_GENERAL, LOG_DEBUG, "no kodi video addons installed");
        } else if (!controls->isInputAdaptive()) {
            LOG(VB_GENERAL, LOG_DEBUG, "please install the inputstream adaptive addon.");
        }

        isKodiConnectedTimer->start(10 * 1000);
        controls->setConnected(2);
    } else if (controls->getConnected() == 0) {
        dialog->createAutoClosingBusyDialog(tr("failed to connect. is kodi installed/running with remote control enabled? (setting m key)"), 6);
        openSettingTimer->start(200);
        return;
    }

    loadPlugins(true);

    controls->loadAddons(pluginManager.hidePlugins());
    SetFocusWidget(uiCtx->fileListGrid); // set the focus to the file list grid

    loadPlugins(false); // load plugins after sorting

    if (gCoreContext->GetSetting("MythAppsMusic").compare("1") == 0) {
        loadImage(uiCtx->fileListGrid, tr("Music"), QString("Music~Music"), music_icon);
    }

    pluginManager.getPluginByName("Favourites")->displayHomeScreenItems();

    LOG(VB_GENERAL, LOG_DEBUG, "loadApps() Finished");
}

void MythApps::loadPlugins(bool start) {
    QList<PluginDisplayInfo> plugins = pluginManager.getPluginsForDisplay(start);
    for (const auto &plugin : plugins) {
        loadImage(uiCtx->fileListGrid, plugin.name, plugin.setData, plugin.iconPath);
    }
}

/** \brief handle key press events  */
bool MythApps::keyPressEvent(QKeyEvent *event) {
    exitToMainMenuSleepTimerReset();

    if (GetFocusWidget() && GetFocusWidget()->keyPressEvent(event)) {
        return true;
    }

    browser->updateBrowserOpenStatus();
    bool handled = false;
    QStringList actions;
    handled = GetMythMainWindow()->TranslateKeyPress("mythapps", event, actions);

    for (int i = 0; i < actions.size() && !handled; i++) {
        QString action = actions[i];
        LOG(VB_GENERAL, LOG_DEBUG, "keyPressEvent(): " + action + "  GetFocusWidget(): " + GetFocusWidget()->objectName());
        handled = true;

        // generic key events
        if (m_busyPopup) {
            LOG(VB_GENERAL, LOG_WARNING, "Too busy. Not handling keypress");
        } else if (action == "CLOSE") {
            niceClose(true);
        } else if (action == "REFRESH") {
            refreshPage();
        } else if (action == "TOGGLEHIDDEN") {
            toggleHiddenFolders();
        } else if (action == "TOGGLERECORD") {
            pluginManager.handleAction(action, currentSelectionDetails);
        } else if (action == "HELP") {
            uiCtx->help->SetVisible(!uiCtx->help->IsVisible());
        } else if (browser->proccessRemote(action)) {

        } else if ((action == "FULLSCREEN")) {
            controls->toggleFullscreen();
            dialog->createAutoClosingBusyDialog(tr("Toggling Fullscreen"), 2);
        } else if ((action == "MINIMIZE")) {
            toggleAutoMinimize();

            // player key events
        } else if ((action == "BACK" || action == "ESCAPE") and (kodiPlayerOpen || GetFocusWidget() == uiCtx->filepath) and !musicOpen) {
            stopPlayBack(); // Kodi player running.
        } else if ((action == "PLAY" || action == "PAUSE") and (kodiPlayerOpen || musicOpen)) {
            pauseToggle();
        } else if (action == "ShowVideoSettings" and kodiPlayerOpen) {
            controls->activateWindow("osdvideosettings");
        } else if ((action == "DETAILS") and kodiPlayerOpen) {
            controls->showInfo();
            controls->togglePlayerDebug(true); // requires double press
        } else if ((action == "DETAILS")) {
            toggleStreamDetails();
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
            // searchSettings
        } else if ((action == "BACK" || action == "ESCAPE") and GetFocusWidget() == uiCtx->searchSettingsButtonList) {
            SetFocusWidget(uiCtx->fileListGrid);
        } else if (handleMusicAction(action)) {

            // mythapps key events
        } else if (kodiPlayerOpen) { // do nothing.

        } else if ((action == "BACK" || action == "ESCAPE") and GetFocusWidget() == uiCtx->fileListGrid and uiCtx->SearchTextEdit->IsVisible()) {
            SetFocusWidget(uiCtx->SearchTextEdit);
        } else if ((action == "BACK" || action == "ESCAPE") and GetFocusWidget() == uiCtx->fileListGrid) {
            goBack();
        } else if ((action == "BACK" || action == "ESCAPE") and GetFocusWidget() == uiCtx->SearchTextEdit and isHome) {
            niceClose(false);
        } else if ((action == "BACK" || action == "ESCAPE") and GetFocusWidget() == uiCtx->SearchTextEdit) {
            goBack();
        } else if (action == "UP" and GetFocusWidget() == uiCtx->SearchTextEdit) {
            SetFocusWidget(uiCtx->fileListGrid);
        } else if (action == "DOWN" and GetFocusWidget() == uiCtx->SearchTextEdit) {
            SetFocusWidget(uiCtx->searchButtonList);
        } else if ((action == "BACK" || action == "ESCAPE" || action == "UP" || action == "LEFT") and GetFocusWidget() == uiCtx->searchButtonList) {
            SetFocusWidget(uiCtx->SearchTextEdit);
        } else if ((action == "DOWN" || action == "RIGHT") and GetFocusWidget() == uiCtx->searchButtonList) {
            SetFocusWidget(uiCtx->fileListGrid);

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
        controls->setConnected(0);
        delay(1);
    }

    if (gCoreContext->GetSetting("MythAppsCloseOnExit").compare("1") == 0 || forceClose) {
        if (!forceClose) {
            controls->quitKodi();
        }
        delayMilli(150);
#ifdef _WIN32
        systemSafe("taskkill /f /im kodi.exe");
#else
        systemSafe("pkill -9 kodi.bin");
#endif
    } else {
        delayMilli(50);
    }
    Close();
}

/** \brief helper function for loadProgram */
void MythApps::loadProgramSlot(QString name, QString setdata, QString thumbnailUrl) { loadProgram(name, setdata, thumbnailUrl); }

/** \brief helper function for loadProgram */
void MythApps::loadProgram(QString name, QString setdata, QString thumbnailUrl) { loadProgram(name, setdata, thumbnailUrl, uiCtx->fileListGrid); }

/** \brief prepares to load the image onscreen. Will insert the image on a new line and sort the images if required \param name directory or file name.
 * \param setdata - all the parameters that need to be retrived on click or hover
 * \param thumbnailUrl
 * \param mythUIButtonList The button to load the image*/
void MythApps::loadProgram(QString name, QString setdata, QString thumbnailUrl, MythUIButtonList *mythUIButtonList) {
    static int count = 0;
    int max = 6;

    if (name.contains(tr("Back"))) {
        count = 0;
    }
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
        loadImage(mythUIButtonList, name, setdata, thumbnailUrl);
    }
}

/** \brief converts local file or a http link into a cached image full path.
 * \param thumbnailUrl - Can be blank, a local file or a http link
 * \return cached image full path */
QString MythApps::getStandardizedImagePath(QString imagePath) {
    QString localImagePath = "";

    if (imagePath.contains(QString("file://"), Qt::CaseInsensitive)) { // local file location, such as the favourite image
        localImagePath = imagePath.replace(QString("file://"), QString(""));
    } else if (imagePath.compare(QString("")) == 0) { // no image file or link
        localImagePath = QString("%1%2").arg(GetShareDir()).arg("themes/default/ma_mv_browse_nocover.png");
    } else { // download image file from http (Kodi json)
        QString imgID = QString(QCryptographicHash::hash((imagePath.toUtf8()), QCryptographicHash::Md5).toHex());
        localImagePath = getGlobalPathPrefix() + imgID;
    }
    return localImagePath;
}

/** \brief creates the data later used to load the image onscreen as a file or directory.
 * \param mythUIButtonList The button to load the image
 * \param name directory or file name.
 * \param setdata all the parameters that need to be retrived on click or hover
 * \param thumbnailUrl */
void MythApps::loadImage(MythUIButtonList *mythUIButtonList, QString name, QString setdata, QString thumbnailUrl) {
    auto *item = new MythUIButtonListItem(mythUIButtonList, "");

    if (name.compare("BlankR") == 0) { // no image and no click or hover
        item->setVisible(false);
        item->SetData("BlankR");
        return;
    } else if (name.compare("Blank") == 0) { // no image
        item->setVisible(false);
        return;
    }

    ProgramData programData(name, setdata);
    if (previouslyPlayedLink->contains(programData.getUrl()) and !isHome) { // disable on home screen
        item->SetText(name, "buttontextWatched");                           // set the image title / name
    } else {
        item->SetText(name, "buttontext2"); // set the image title / name
    }

    QString localImagePath = getStandardizedImagePath(thumbnailUrl);
    QString appIcon = getStandardizedImagePath(controls->getLocationFromUrlAddress(programData.getUrl()));

    QVariantMap imageInfo;
    imageInfo.insert("thumbnailUrl", thumbnailUrl);
    imageInfo.insert("localImagePath", localImagePath);
    imageInfo.insert("appIcon", appIcon);
    imageInfo.insert("setData", setdata);

    item->SetData(imageInfo);
}

/** \brief receives result from image thread and displays the corresponding image onscreen.
 *  \param id id of the button to update with the new image
 *  \param fileListType button list or file browser to update*/
void MythApps::handleImageSlot(int id, const QString thumbnailUrl, MythUIButtonList *fileListType) {
    if (id <= fileListType->GetCount()) {
        auto *item = fileListType->GetItemAt(id);
        QVariant itemData = item->GetData();

        if (itemData.canConvert<QVariantMap>()) {
            QVariantMap map = itemData.toMap();

            // Verify that this item actually matches the filename
            if (map.value("thumbnailUrl").toString() == thumbnailUrl) {
                item->SetData(map.value("setData").toString());
                item->SetImage(map.value("localImagePath").toString() + ".processed");
                return;
            } else {
                LOG(VB_GENERAL, LOG_DEBUG, QString("handleImageSlot() error: index %1, thumbnailUrl '%2'").arg(id).arg(thumbnailUrl));
                return;
            }
        }
    } else {
        LOG(VB_GENERAL, LOG_DEBUG, "handleImageSlot() error");
    }
}

/** \brief allows child classess to set the focus Widget
 *  \param widetName the name of the widget to set focus*/
void MythApps::setFocusWidgetSlot(QString widetName) {
    LOG(VB_GENERAL, LOG_DEBUG, "setFocusWidgetSlot()");

    if (widetName.compare("uiCtx->searchButtonList") == 0) {
        SetFocusWidget(uiCtx->searchButtonList);
    } else if (widetName.compare("uiCtx->fileListGrid") == 0) {
        SetFocusWidget(uiCtx->fileListGrid);
    }
}

void MythApps::setSearchButtonListVisible(bool visible) {
    uiCtx->searchButtonList->SetVisible(visible);
    uiCtx->searchButtonListGroup->SetEnabled(visible);
}

/** \brief  toggle the visibilty of the seachbox including the search button.
 *  \param  visible - set the search box visibility status */
void MythApps::toggleSearchVisible(bool visible) {
    uiCtx->SearchTextEdit->SetVisible(visible);
    setSearchButtonListVisible(false);
    uiCtx->title->SetVisible(!visible);

    if (uiCtx->SearchTextEdit->GetText().isEmpty() and visible) {
        uiCtx->SearchTextEditBackgroundText->SetVisible(true);
    } else {
        uiCtx->SearchTextEditBackgroundText->SetVisible(false);
    }
}

void MythApps::makeSearchTextEditEmpty() {
    uiCtx->SearchTextEdit->SetText("");
    showSearchTextEditBackgroundText();
}

void MythApps::showSearchTextEditBackgroundText() { uiCtx->SearchTextEditBackgroundText->SetText(tr("Search") + " " + firstDirectoryName); }

/** \brief Display's the back button in the file browser grid*/
void MythApps::loadBackButton() {
    LOG(VB_GENERAL, LOG_DEBUG, "loadBackButton()");

    if (searching) {
        SetFocusWidget(uiCtx->fileListGrid);
    }
    loadProgram(QString(tr("Back")), createProgramData(tr("Back"), "", "", false, ""), QString("file://") + back_icon);
}

/** \brief selected callback for the search list. */
void MythApps::selectSearchList(MythUIButtonListItem *item) {
    QCoreApplication::processEvents();
    QString buttonName = item->GetText();
    if (buttonName.compare("Back to Search") == 0) {
        SetFocusWidget(uiCtx->SearchTextEdit);
        uiCtx->searchButtonList->SetItemCurrent(0);
    }
}

/** \brief clicked callback for the search list. */
void MythApps::clickedSearchList(MythUIButtonListItem *item) {
    if (searchListLink->getListSizeEnabled() == 0) { // display error is no search sources
        dialog->createAutoClosingBusyDialog(tr("Please setup search sources in M->Setting"), 3);
        return;
    }

    QString buttonName = item->GetText("buttontext2");
    if (buttonName.compare("Search") == 0) { // the search submit button is the first search suggeston
        goSearch("");
    } else if (buttonName.compare("Back to Search") == 0) {
    } else {
        uiCtx->SearchTextEdit->SetText(buttonName);
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
    if (uiCtx->SearchTextEdit->GetText().isEmpty()) { // make search button list disappear instantly
        setSearchButtonListVisible(false);
        showSearchTextEditBackgroundText();
    } else {
        uiCtx->SearchTextEditBackgroundText->SetText("");
        searchSuggestTimer->start(10); // run the code in a seperate run once timer 'thread'
    }
}

/** \brief updates the search focus by running the searchFocusTimerSlot timer */
void MythApps::searchTextEditLosingFocus() { searchFocusTimer->start(10); }

/** \brief updates the search focus after a slight delay*/
void MythApps::searchFocusTimerSlot() {
    if (GetFocusWidget() != uiCtx->searchButtonList and GetFocusWidget() != uiCtx->SearchTextEdit) {
        setSearchButtonListVisible(false);
    }
}

/** \brief Looks up the search suggestions for the search box after a slight delay*/
void MythApps::searchSuggestTimerSlot() {
    QString localSearchText = uiCtx->SearchTextEdit->GetText();
    searchText = localSearchText;

    if (pluginManager.handleSuggestion(localSearchText)) {
        uiCtx->SearchTextEdit->SetText("");
        return;
    }

    SearchSuggestions suggestions;
    QStringList word = suggestions.getSuggestions(localSearchText);

    uiCtx->searchButtonList->Reset();
    MythUIButtonListItem *searchButton = new MythUIButtonListItem(uiCtx->searchButtonList,
                                                                  ""); // submit button. 1st result in search suggestion list
    searchButton->SetText("Search", "buttontext2");

    for (int i = 0; i < word.size(); ++i) {
        MythUIButtonListItem *searchButton2 = new MythUIButtonListItem(uiCtx->searchButtonList, "");
        searchButton2->SetText(word.at(i).toLocal8Bit().constData(), "buttontext2");
    }

    if (word.size() > 1) {
        MythUIButtonListItem *searchButton2 = new MythUIButtonListItem(uiCtx->searchButtonList, ""); // submit button. 1st result in search suggestion list
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

    browser->bringToFrontIfOpen();
    setSearchButtonListVisible(false);
    searchNoDuplicateCheck = QStringList();
    searchText = uiCtx->SearchTextEdit->GetText();

    if (searchText.size() < 1) { // make sure search term is at least one character
        return;
    }

    if (musicOpen) { // search music
        musicSearch(searchText);
        return;
    }

    uiCtx->fileListGrid->Reset();
    loadBackButton();

    if (fileBrowserHistory->isAppOpen()) {
        QString appPath = "app://" + fileBrowserHistory->getCurrentApp() + "/search?=" + searchText;

        fileBrowserHistory->append(appPath, appPath);
        uiCtx->filepath->SetText(appPath);

        pluginManager.search(searchText, fileBrowserHistory->getCurrentApp());
        return;
    }

    searching = true;
    QCoreApplication::processEvents();
    dialog->createBusyDialog(tr("Retrieving Search Results..."));

    if (overrideCurrentSearchUrl.compare("") != 0) {
        fetchSearch(overrideCurrentSearchUrl);
    } else if (isHome) {
        foreach (QString search, searchListLink->getListEnabled()) {
            fetchSearch(search);
        } // fetch all search results for all apps.
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
    makeSearchTextEditEmpty();

    dialog->closeBusyDialog(); // close the Retrieving Search Results dialog
    uiCtx->filepath->SetText(tr("Finished Searching - ") + uiCtx->filepath->GetText());
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
        dialog->createAutoClosingBusyDialog(tr("Hiding Folders"), 3);
    } else {
        dialog->createAutoClosingBusyDialog(tr("Showing Hidden Folders"), 2);
    }
    enableHiddenFolders = !enableHiddenFolders;
    reload();
}

/** \brief toggle the allowing of the auto minimizeing of Kodi. Useful to edit settings*/
void MythApps::toggleAutoMinimize() {
    allowAutoMinimize = !allowAutoMinimize;
    if (allowAutoMinimize) {
        dialog->createAutoClosingBusyDialog(tr("Auto Minimize On"), 3);
    } else {
        dialog->createAutoClosingBusyDialog(tr("Warning - Auto Minimize Off"), 2);
        goFullscreen();
        controls->toggleFullscreen();
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
bool MythApps::folderAllowed(const QString &label, const QStringList &previousSearchTerms) {
    static const QSet<QString> blacklist = {"Bookmarks", "Settings", "Logout", "Search", "New Search", "Switch User", "Select Profile", "Quick Search (Incognito)"};
    static const QSet<QString> searchBlacklist = {"Live", "Playlists"};

    if (enableHiddenFolders)
        return true;

    if (blacklist.contains(label))
        return false;

    if (searching && (searchBlacklist.contains(label) || previousSearchTerms.contains(label)))
        return false;

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
        uiCtx->fileListGrid->Reset();
        loadBackButton();
        dialog->getLoader()->SetVisible(false);
    }
}

/** \brief If clicking on a folder that displays all shows (allShowsFolderFound), load az search to allow the user to search by leter.
 *  \param hash hash of the answered contents from displayFileBrowser
 *  \return is the showAZsearch being displayed*/
bool MythApps::loadAZSearch(QString hash) {
    if ((!searching and allShowsFolderFound and currentSearchUrl.size() > 1) || azShowOnUrl.contains(hash) || overrideAppAZSearch(hash)) {
        loadProgram(tr("Shows A-Z"), createProgramData("", "Shows A-Z", "", false, ""), QString("file://") + ma_tv_icon);
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

    const quint64 loadId = QDateTime::currentMSecsSinceEpoch();
    currentLoadId.store(loadId);

    QString hash = QString(QCryptographicHash::hash((answer.toUtf8()), QCryptographicHash::Md5).toHex());
    QByteArray br = answer.toUtf8(); // parse json
    QJsonDocument doc = QJsonDocument::fromJson(br);
    QJsonObject addons = doc.object();
    QJsonValue addons2 = addons["result"];
    QJsonObject o = addons2.toObject();
    QJsonArray agentsArray = o["files"].toArray();

    loadBackButtonIfRequired(m_loadBackButton);
    bool showAZsearch = loadAZSearch(hash);

    int searchCount = 0;
    int count = 0;

    foreach (const QJsonValue &v, agentsArray) { // loop throught each file/folder in the file browser
        if (currentLoadId.loadRelaxed() != loadId) {
            LOG(VB_GENERAL, LOG_DEBUG, "displayFileBrowser() Cancelled");
            return;
        }

        QString label = removeBBCode(v.toObject().value("label").toString());
        QString thumbnail = v.toObject().value("thumbnail").toString();
        QString plot = v.toObject().value("plot").toString(); // art
        QString file = v.toObject().value("filetype").toString();
        QString fileUrl = v.toObject().value("file").toString();

        count++;
        if (count > 25)
            QCoreApplication::processEvents();

        if (searching and searchNoDuplicateCheck.contains(fileUrl)) {
            LOG(VB_GENERAL, LOG_DEBUG, "displayFileBrowser() duplicate found");
            continue;
        }
        searchNoDuplicateCheck.append(fileUrl);

        // only display items starting with the corrosponding start leter when using the a-z filter
        if (searchStartLeter.compare("") != 0 and label.size() > 0) {
            if (searchStartLeter.compare(QString(label.at(0)), Qt::CaseInsensitive) != 0) {
                continue;
            }
        }

        if (label.compare("a", Qt::CaseInsensitive) == 0) // does a folder named a leter exist.
            alphabeticalFolderFound = true;

        if (folderAllowed(label, previousSearchTerms)) { // if not a banned folder such as logout etc, load / display folder
            bool playVideo = false;
            if (file.compare("file") == 0) {
                playVideo = true;
            }
            loadProgram(label, createProgramData(fileUrl, plot, thumbnail, playVideo, ""), thumbnail);

            if (searching and isHome) { // return max of 5 items when using global search
                if (searchCount >= 4)
                    return;
            }
            searchCount++;
        }

        discoverSearchURLs(label, v.toObject().value("file").toString()); // discover if the directory is a search source url
    }

    // if alphabetical folders are natively supported by the app, remove the
    // mythapps az search as we dont need duplicate az search functionality
    if (alphabeticalFolderFound and showAZsearch) {
        uiCtx->fileListGrid->RemoveItem(uiCtx->fileListGrid->GetItemAt(1));
    } else if (showAZsearch) {
        azShowOnUrl.append(hash); // used to restore az search for back button
    }

    allShowsFolderFound = false;
    searchStartLeter = "";

    if (stopScroll) { // used for next page
        dialog->getLoader()->SetVisible(false);
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

    if (!searchListLink->containsLike(file)) { // if search url not discovered,
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
        currentSearchUrl = searchListLink->findSearchUrl(file);
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
            searchListLink->appendSearchUrl(currentSearchUrl); // save search source
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

/** \brief go to the previous url in the file browser*/
void MythApps::goBack() {
    searchNoDuplicateCheck = QStringList();
    currentLoadId.fetchAndAddOrdered(0);
    fileBrowserHistory->goBack();
    reload();
}

/** \brief refresh the currrent page wihtout cache*/
void MythApps::refreshPage() {
    LOG(VB_GENERAL, LOG_DEBUG, "refreshPage()");

    searchNoDuplicateCheck = QStringList();
    dialog->createAutoClosingBusyDialog(tr("Refreshing Page"), 2);
    reload();
}

/** \brief play the video
 * \param filePathParam url of the video
 * \param seekAmount amount to seek in hours minutes seconds. Can be blank */
void MythApps::play(QString mediaLocation, QString seekAmount) {
    uiCtx->plot->SetText("Play");
    controls->play(mediaLocation, seekAmount);
}

/** \brief make kodi go fullscreen */
void MythApps::goFullscreen() {
    LOG(VB_GENERAL, LOG_DEBUG, "goFullscreen()");

    if (musicOpen) {
        controls->activateWindow("visualer");
    } else {
        controls->activateWindow("screensaver"); // hides the kodi gui (may be unstable?)
    }

    minimizeTimer->stop();           // stops minimizing kodi
    SetFocusWidget(uiCtx->filepath); // hack to make the seek work in fullscreen as the remote requires focus

#ifdef __ANDROID__
    if (!controls->androidAppSwitch("Kodi")) {
        dialog->createAutoClosingBusyDialog(tr("Myth Apps Services (anroid app - apk) is either not installed or "
                                               "running. Have you opened the app?"),
                                            3);
        niceClose(true);
    }
#elif _WIN32
    wchar_t kodi_wchar[] = L"Kodi";
    ShowWindow(FindWindow(NULL, kodi_wchar), SW_SHOWMAXIMIZED);
    delayMilli(100);
    controls->toggleFullscreen();
#else // linux
    if (isGnomeWayland()) { // use activate-window-by-title
        activateWindowWayland("Kodi");
        if (!controls->isFullscreen()) {
            delayMilli(350);
            controls->inputActionHelper("togglefullscreen");
        }

    } else { // X11
        for (int i = 0; i < 2; i++) {
            if (controls->isFullscreen()) { // fullscreen toggle from fullscreen to window mode and then full screen to bring kodi to the front of mythtv.
                delayMilli(50);
                controls->toggleFullscreen();
                delayMilli(20);
                controls->toggleFullscreen();
            } else { // window
                controls->toggleFullscreen();
            }
            delayMilli(100);
            if (controls->isFullscreen()) {
                break;
            } else {
                delayMilli(200);
            }
        }
    }
#endif
}

/** \brief minimizes kodi by toggling out of fullscreen. Suggested to call multible times for reliability incase kodi drops a request */
void MythApps::goMinimize(bool fullscreenCheck) {
    if (!allowAutoMinimize) {
        return;
    }
    if (fullscreenCheck && isX11()) {
        if (controls->isFullscreen()) {
            controls->toggleFullscreen();
        }
    }
#ifdef __ANDROID__
#elif _WIN32
    wchar_t kodi_wchar[] = L"Kodi";
    ShowWindow(FindWindow(NULL, kodi_wchar), SW_MINIMIZE);
    delayMilli(150);
    wchar_t kodi_wchar2[] = L"MythApps";
    SetActiveWindow(FindWindow(NULL, kodi_wchar2));
#else
    controls->goMinimize();
#endif
}

/** \brief close dialog if one pops up. e.g. for a subscription
 * \param  forceFullScreenVideo hide seekbar OSD when video first opens*/
void MythApps::handleDialogs(bool forceFullScreenVideo) {
    QString systemCurrentWindow = controls->handleDialogs();
    LOG(VB_GENERAL, LOG_DEBUG, "handleDialogs() -" + systemCurrentWindow);

    if (systemCurrentWindow.compare("Home") == 0) {
        controls->activateFullscreenVideo();
    }

    if (systemCurrentWindow.compare("Fullscreen OSD") == 0 and forceFullScreenVideo) {
        controls->showOSD();
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
    m_screenshot = QImage();
    uiCtx->screenshotMainMythImage->SetEnabled(false);
    uiCtx->screenshotMainMythImage->Reset();
}

/** \brief take a screenshot. Useful for getting a thumbnail for the currently playing video \return is the screenshot a black screen? */
void MythApps::takeScreenshot() {
#ifdef __ANDROID__
    return;
#endif
    if (musicOpen)
        return;

    QImage screenshot = screen->grabWindow(0).toImage().convertToFormat(QImage::Format_ARGB32).scaled(1920, 1080, Qt::KeepAspectRatio);

    QColor imageColor(screenshot.pixel(600, 600));
    if (imageColor.red() == 0 and imageColor.green() == 0 and imageColor.blue() == 0) {
        LOG(VB_GENERAL, LOG_DEBUG, "takeScreenshot() is blank ");
    } else {
        m_screenshot = screenshot;
        LOG(VB_GENERAL, LOG_DEBUG, "takeScreenshot()");
    }
}

/** \brief toggle the pause status */
void MythApps::pauseToggle() {
    controls->pauseToggle();
    delayMilli(200); // button debounce
}

/** \brief stop playing the video from the remote */
void MythApps::stopPlayBack() {
    LOG(VB_GENERAL, LOG_DEBUG, "stopPlayBack()");
    takeScreenshot();
    controls->stopPlayBack();
}

/** \brief runs when clicking the button/image. Used to load the next directory, file, link or app
 \param  item - the mythui button */
void MythApps::appsCallback(MythUIButtonListItem *item) { appsCallback(item->GetText("buttontext2"), item->GetData().toString()); }

void MythApps::appsCallback(QString label, QString data, bool allowBack) {
    LOG(VB_GENERAL, LOG_DEBUG, "appsCallback()-" + data);

    searching = false;

    QScopedPointer<ProgramData> programData(new ProgramData(label, data));
    programData->setFirstDirectory(isHome);
    firstDirectoryName = programData->getAppName(firstDirectoryName);
    showSearchTextEditBackgroundText();
    isHome = false;

    // if back button clicked
    if (programData->hasBack()) {
        goBack();
        return;
    }

    // back
    if (allowBack) {
        fileBrowserHistory->append(label, data);
    }

    // open internal plugins for Favourites, Music, Watched List or Videos if clicked
    if (programData->hasArtists()) { // if music buttons clicked
        loadSongsMain(label, "artists");
        return;
    } else if (programData->haAlbums()) {
        loadSongsMain(label, "albums");
        return;
    } else if (programData->hasGenres()) {
        loadSongsMain(label, "genres");
        return;
    } else if (programData->hasPlaylists()) {
        loadSongsMain(programData->getPlot(), "playlists");
        return;
    }
    // launch web browser if clicking on a link
    if (programData->hasWeb()) {
        browser->openBrowser(programData->getFilePathParam());
        SetFocusWidget(uiCtx->searchButtonList);
        return;
    }

    if (programData->hasApp()) {
        QString appName = programData->getAppPluginName();
        PluginAPI *plugin = pluginManager.getPluginByName(appName);
        if (plugin) {
            if (programData->refreshGrid()) {
                uiCtx->fileListGrid->Reset();
                loadBackButton();
            }
            isHome = false;
            plugin->load(label, programData->getDataWithoutAppName(data));
            uiCtx->filepath->SetText(programData->getFriendlyPathName(data));
        }
        return;
    }

    if (programData->hasSearchShowsAZ()) {
        searchStartLeter = label;
        uiCtx->SearchTextEdit->SetText(searchStartLeter);
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

    // call function for any app directories clicked
    if (appsCallbackPlugins(programData, label, data)) {
        return;
    }
}

/** \brief handle any app directories clicked. Used to load the next directory or open a video
\param  item - the selected button */
bool MythApps::appsCallbackPlugins(QScopedPointer<ProgramData> &programData, QString label, QString data) {
    LOG(VB_GENERAL, LOG_DEBUG, "appsCallbackPlugins()");
    dialog->getLoader()->SetVisible(true);

    uiCtx->androidMenuBtn->SetVisible(false);
    uiCtx->androidMenuBtn->SetEnabled(false);

    QString fileURL = friendlyUrl(programData->getFilePathParam());
    uiCtx->filepath->SetText(fileURL);

    if (!QString(programData->getFilePathParam().at(0)).compare("/") == 0) { // file system if first character is a slash
        fileURL = "plugin://" + fileURL;
    }

    bool loadBackButton = false;
    if (!label.contains(tr("Back to App")) and !programData->isPlayRequest()) {
        loadBackButton = true;
    }

    foreach (QString folder, showsAZfolderNames) {
        if (label.compare(folder, Qt::CaseInsensitive) == 0 and !folder.isEmpty()) { // used to turn on shows AZ.
            allShowsFolderFound = true;
        }
    }

    if (programData->isPlayRequest()) {
        lastPlayedDetails->set(label, fileURL + "~" + programData->getPlot() + "~" + programData->getImageUrl() + "~play");

        QString seek = "00:00:00"; // default no seek
        if (programData->hasSeek()) {
            seek = programData->getSeek(); // retrive seek value if specified
        }

        dialog->getLoader()->SetVisible(false);
        play_Kodi(fileURL, seek);
        return true;
    }
    requestFileBrowser(fileURL, QStringList(), loadBackButton, data); // request the url to load the next directory

    return false;
}

/** \brief the hover selected button/image. Used to update the selected title and plot.
 \param  item - the mythui button */
void MythApps::selectAppsCallback(MythUIButtonListItem *item) {
    uiCtx->title->SetText(removeBBCode(item->GetText("buttontext2")));

    ProgramData programData(item->GetText("buttontext2"), item->GetData().toString());

    if (programData.hasPlotandImageUrl()) { // figure out if 'video' home screen item
        currentSelectionDetails->set(getLabel(item), item->GetData().toString());
    } else {
        currentSelectionDetails->set("", "");
    }

    uiCtx->plot->SetText("");

    if (programData.getPlot().size() > 30) {
        if (previouslyPlayedLink->contains(programData.getUrl())) {
            currentSelectionDetails->setPreviouslyPlayed(true);
        } else {
            currentSelectionDetails->setPreviouslyPlayed(false);
        }

        uiCtx->plot->SetText(removeBBCode(programData.getPlot()));
    }

    if (stopScroll) {
        uiCtx->fileListGrid->SetItemCurrent(nextPagePosm_fileListGrid - 1);
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
    appsCallback(nextPageitem->GetText("buttontext2"), nextPageitem->GetData().toString(), false);

    nextPageitem->SetData(createProgramData(tr("Back"), "", "", false, ""));
    delayWhileStopScroll(2);
    if (stopScroll) {
        dialog->createBusyDialog(tr("Loading Next Page"));
        delayWhileStopScroll(8); // timeout message and the stopscroll incase it gets hung
        dialog->closeBusyDialog();
        stopScroll = false;
        dialog->getLoader()->SetVisible(false);
    }
}

/** \brief set a max delay when the stopscroll varibles is true
 * \param maxDelay - max seconds to delay for */
void MythApps::delayWhileStopScroll(int maxDelay) {
    int delayCount = 0;
    while (stopScroll) {
        delay(1);
        delayCount++;
        if (delayCount == maxDelay)
            return;
    }
}

/** \brief runs when the button is visible on screen. Used to automatically request the next page to allow infinite scroll.
 \param item - the mythui button */
void MythApps::visibleAppsCallback(MythUIButtonListItem *item) {
    displayImage(item, uiCtx->fileListGrid);

    if (item->GetText("buttontext2").contains("Next Page")) {
        item->SetText(tr("Back to App"), "buttontext2");
        nextPageitem = item;
        stopScroll = true;
        waitForThreads(0);
        waitforRequests();
        nextPageTimer->start(50); // run this code is another single run 'timer thread' to avoid stacking/blocking visibleAppsCallback

        nextPagePosm_fileListGrid = uiCtx->fileListGrid->GetCurrentPos();
        LOG(VB_FILE, LOG_DEBUG, "visibleAppsCallback finished");
    }
}

/** \brief Called from visibleAppsCallback. Creates a thread to download and process the image.
 * \param item The mythui button
 * \param m_fileList The file browser */
void MythApps::displayImage(MythUIButtonListItem *item, MythUIButtonList *fileList) {
    if (!item->GetData().canConvert<QVariantMap>())
        return;

    QVariantMap info = item->GetData().toMap();
    Params p{fileList->GetItemPos(item), info["thumbnailUrl"].toString(), info["localImagePath"].toString(), info["appIcon"].toString(), username, password, ip, port, fileList};

    auto *worker = new ImageThread(p);
    connect(worker, &ImageThread::finished, this, &MythApps::handleImageSlot, Qt::QueuedConnection);
    QThreadPool::globalInstance()->start(worker);

    QVariantMap handleImageMap;
    handleImageMap.insert("setData", info["setData"].toString());
    handleImageMap.insert("localImagePath", p.localImagePath);
    handleImageMap.insert("thumbnailUrl", p.thumbnailUrl);

    item->SetData(QVariant::fromValue(handleImageMap));
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
    QString filePathName = getGlobalPathPrefix() + "00cache/" + urlID + ".cache";
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

void MythApps::searchSettingsClicked(MythUIButtonListItem *item) { pluginManager.searchSettingsClicked(item); }

/** \brief load shows AZ( */
void MythApps::loadShowsAZ() {
    LOG(VB_GENERAL, LOG_DEBUG, "loadShowsAZ()");
    uiCtx->fileListGrid->Reset();
    loadBackButton();
    toggleSearchVisible(false);

    QString alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    QString thumbnail = controls->getLocationFromUrlAddress(currentSearchUrl);

    for (int i = 0; i < alphabet.length(); i++) {
        loadProgram(QString(alphabet.at(i).toLatin1()), createProgramData("", "searchShowsAZ", "", false, ""), thumbnail);
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

        if (!currentSelectionDetails->isEmpty()) {
            QStringList menuItems = pluginManager.getOptionsMenuLabels(currentSelectionDetails, fileBrowserHistory->getCurrentData());
            for (const QString &item : menuItems) {
                m_menuPopup->AddButton(item);
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
                QDir dir(getGlobalPathPrefix() + "00cache/");
                dir.removeRecursively();
                createDirectoryIfDoesNotExist(getGlobalPathPrefix() + "00cache/");
            } else if (resulttext == tr("Clear Previously Played")) {
                previouslyPlayedLink->listRemove(currentSelectionDetails->get());
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
            }

            if (pluginManager.menuCallBack(resulttext, currentSelectionDetails))
                reload();
        }

        if (resultid == "watchList") {
            if (resulttext == tr("Flag as Watched and Exit to Menu")) {
                resetScreenshot();
                addToPreviouslyPlayed();
            } else if (resulttext == tr("Exit to Menu")) {
                resetScreenshot();
            } else if (resulttext == tr("Save Position to Watch List and Exit to Menu")) {
                lastPlayedDetails->setSeek(getPlayBackTimeString(true, false));
                pluginManager.appendWatchedLink(lastPlayedDetails->get());
                resetScreenshot();
                addToPreviouslyPlayed();
            } else if (resulttext.startsWith(tr("Keep Watching:"))) {
                play_Kodi(lastPlayedDetails->getUrl(), getPlayBackTimeString(true, false));
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
    if (fileBrowserHistory->isEmpty()) {
        loadApps();
    } else {
        appsCallback(fileBrowserHistory->getCurrentLabel(), fileBrowserHistory->getCurrentData(), false);
    }
}

/** \brief refresh the selection in the file list grid.  */
void MythApps::refreshFileListGridSelection() {
    int pos = uiCtx->fileListGrid->GetCurrentPos();
    uiCtx->fileListGrid->SetItemCurrent(0);
    uiCtx->fileListGrid->SetItemCurrent(pos);
}

/** \brief plays a video in kodi and seeks if required.
 * \param mediaLocation url of the video
 * \param seekAmount amount to seek in hours minutes seconds. Can be blank */
void MythApps::play_Kodi(QString mediaLocation, QString seekAmount) {
    LOG(VB_GENERAL, LOG_INFO, "play_Kodi(): " + mediaLocation + " seek: " + seekAmount);

    if (gCoreContext->GetSetting("MythAppsInternalRemote").compare("1") == 0) {
        kodiPlayerOpen = true; // enable myth remote
    }

    minimizeTimer->stop();
    exitToMainMenuSleepTimer->stop();

    goFullscreen();
    smartDelay();
    play(mediaLocation, seekAmount); // play the media
    videoStopReceived = false;

    m_screenshot.load(getStandardizedImagePath(lastPlayedDetails->getImageUrl()) + ".processed");
    m_screenshot = m_screenshot.copy(0, m_screenshot.height() * 0.20, m_screenshot.width(), m_screenshot.height() * 0.80);

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
        bool isApp = false;

        if (fileBrowserHistory->isAppOpen()) {
            isApp = pluginManager.useBasicMenu(fileBrowserHistory->getCurrentApp());
        }

        if (!previouslyPlayedLink->contains(lastPlayedDetails->getUrl()) && !excludePreviouslyPlayed(lastPlayedDetails->getUrl()) && !isApp) {
            m_menuPopup->AddButton(tr("Flag as Watched and Exit to Menu"));
        }
        m_menuPopup->AddButton(tr("Exit to Menu"));
        if (!isApp)
            m_menuPopup->AddButton(tr("Save Position to Watch List and Exit to Menu"));
        m_menuPopup->AddButton(tr("Keep Watching: ") + getPlayBackTimeString(true, true).replace("00:00", ""));
        m_menuPopup->AddButton(tr("Exit to MythTV Menu"));
    } else {
        delete m_menuPopup;
        m_menuPopup = nullptr;
    }
}

/** \brief calls minimize kodi */
void MythApps::minimizeTimerSlot() { goMinimize(false); }

/** \brief Delays execution based on the time elapsed since last closed. */
void MythApps::coolDown() {
    if (!gLastPluginUnloadTime.isValid())
        return;

    int elapsed = gLastPluginUnloadTime.msecsTo(QDateTime::currentDateTime());
    int remaining = 2000 - elapsed;

    if (remaining > 0)
        delayMilli(remaining);
}

/** \brief closes any hung searchs */
void MythApps::searchTimerSlot() {
    dialog->closeBusyDialog();
    searchTimer->stop();
}

/** \brief decides what menu to open when media playback has finished
 * \param screenType Player.OnStop */
void MythApps::openOSD(QString screenType) {
    goMinimize(true);
    kodiPlayerOpen = false;

    if (!m_screenshot.isNull()) {
        MythImage *m_image = GetPainter()->GetFormatImage();
        m_image->Assign(m_screenshot);
        uiCtx->screenshotMainMythImage->SetEnabled(true);
        uiCtx->screenshotMainMythImage->SetImage(m_image);
    }

    if (screenType.compare("Player.OnStop") == 0) {
        videoStopReceived = true;
        playbackTimer->stop();
        createPlayBackMenu();
        goMinimize(true);
        minimizeTimer->start();
#ifdef __ANDROID__
        controls->androidAppSwitch("MythTV");
#endif
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
    if (GetFocusWidget() == uiCtx->filepath) {
        if (m_fileListMusicGrid->IsVisible()) {
            SetFocusWidget(m_fileListMusicGrid);
        } else {
            SetFocusWidget(uiCtx->fileListGrid);
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
        QString btnText = uiCtx->fileListGrid->GetItemCurrent()->GetText("buttontext2");
        uiCtx->fileListGrid->GetItemCurrent()->SetText("", "buttontext2");
        uiCtx->fileListGrid->GetItemCurrent()->SetText(btnText, "buttontextWatched");
    } else {
        QString btnText = uiCtx->fileListGrid->GetItemCurrent()->GetText("buttontextWatched");
        uiCtx->fileListGrid->GetItemCurrent()->SetText("", "buttontextWatched");
        uiCtx->fileListGrid->GetItemCurrent()->SetText(btnText, "buttontext2");
    }
}

/** \brief add the previosulyPlayed Video to the previouslyPlayedLink. Used for watched/played flag shown in the gui with the green text colour on the button*/
void MythApps::addToPreviouslyPlayed() {
    previouslyPlayedLink->append(lastPlayedDetails->get());
    setButtonWatched(true);
    refreshFileListGridSelection();
}

/** \brief display the options menu when the Android menu button is clicked*/
void MythApps::androidMenuBtnSlot() { showOptionsMenu(); }

void MythApps::toggleStreamDetails() {
    if (!uiCtx->streamDetailsbackground->IsVisible()) {
        uiCtx->streamDetailsbackground->Show();
        uiCtx->streamDetails->Show();
        uiCtx->streamDetails->SetText(streamDetails);
    } else {
        uiCtx->streamDetailsbackground->Hide();
    }
}

/** \brief Central handler: call on Play, Pause, Resume, Seek, Stop*/
void MythApps::handlePlaybackEvent(const QString &method, const QString &message) {
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    if (method == "Player.OnAVStart" || method == "Player.OnResume" || method == "Player.OnSpeedChanged") {
        manualAccumulatedMs_ = getKodiPlaybackTimeMs();
        playbackStartMs_ = now;
    } else if (method == "Player.OnSeek") {
        manualAccumulatedMs_ = controls->getTimeFromSeekTimeMs(message);
        playbackStartMs_ = now;
    } else if (method == "Player.OnPause") {
        if (playbackStartMs_ != -1) {
            manualAccumulatedMs_ += now - playbackStartMs_;
            playbackStartMs_ = -1;
        }
    } else if (method == "Player.OnStop") {
        if (playbackStartMs_ != -1) {
            manualAccumulatedMs_ += now - playbackStartMs_;
            playbackStartMs_ = -1;
        }
    }
}

/** \brief Query Kodi for the true position (in milliseconds) */
qint64 MythApps::getKodiPlaybackTimeMs() {
    QVariantMap map = controls->getPlayBackTime(); //
    if (!map.contains("time"))
        return 0;

    playbackDuration = map["duration"].toString();

    QVariantMap t = map["time"].toMap();
    QTime qt(t["hours"].toInt(), t["minutes"].toInt(), t["seconds"].toInt());
    return QTime(0, 0).msecsTo(qt);
}

/** \brief Compute the current total playback time (ms) */
qint64 MythApps::getCurrentPlaybackTimeMs() const {
    if (playbackStartMs_ != -1) {
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        return manualAccumulatedMs_ + (now - playbackStartMs_);
    }
    return manualAccumulatedMs_;
}

/** \return the playback time of the playing media as a string
 * \param adjustEnd - Check if current time is within 60 seconds of the end */
QString MythApps::getPlayBackTimeString(bool adjustEnd, bool removeHoursIfNone) {
    qint64 currentTimeMs = getCurrentPlaybackTimeMs();
    QString time = formatTime(currentTimeMs);

    qint64 playbackDurationMs = QTime(0, 0).msecsTo(QTime::fromString(playbackDuration, "hh:mm:ss"));

    if (adjustEnd) {
        if (playbackDurationMs - currentTimeMs <= 30000)
            time = "00:00:00";
    }

    if (removeHoursIfNone)
        time = removeHoursIfZero(time);

    return time;
}
