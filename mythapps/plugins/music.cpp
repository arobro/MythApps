#include "music.h"

// QT headers
#include <QDebug>
#include <QDir>
#include <QDirIterator>

// MythTV headers
#include "libmyth/mythcontext.h"
#include "libmythui/mythuiutils.h"

// MythTV headers
#include <libmythui/mythscreentype.h>
#include <libmythui/mythuibutton.h>
#include <libmythui/mythuibuttonlist.h>
#include <libmythui/mythuiprogressbar.h>
#include <libmythui/mythuishape.h>

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
#include "SafeDelete.h"
#include "netRequest.h"
#include "plugin_api.h"
#include "programData.h"
#include "shared.h"

// Globals
const QString appPathName = "app://Music/";

Music::Music() : pluginName("Music"), pluginIcon("ma_music.png") { 
	music_icon = createImageCachePath("ma_music.png"); 
	
    username = QString(gCoreContext->GetSetting("MythAppsusername"));
    password = QString(gCoreContext->GetSetting("MythAppspassword"));
    ip = QString(gCoreContext->GetSetting("MythAppsip"));
    port = QString(gCoreContext->GetSetting("MythAppsport"));
}

Music::~Music() {
    playbackTimer->disconnect();
    playbackTimer->stop();
    playbackTimer->deleteLater();

    hintTimer->disconnect();
    hintTimer->stop();
    hintTimer->deleteLater();

    SafeDelete(m_textSong);
    SafeDelete(m_textArtist);
    SafeDelete(m_textAlbum);
    SafeDelete(m_musicDuration);
    SafeDelete(m_hint);
    SafeDelete(m_seekbar);
    SafeDelete(m_musicTitle);
    SafeDelete(m_trackProgress);
    SafeDelete(m_coverart);
    SafeDelete(m_blackhole_border);
    SafeDelete(m_playingOn);
    SafeDelete(m_next_buttonOn);
    SafeDelete(m_ff_buttonOn);
    SafeDelete(m_playingOff);
    SafeDelete(m_next_buttonOff);
    SafeDelete(m_ff_buttonOff);

    SafeDelete(m_fileListMusicGrid);
    SafeDelete(m_fileListSongs);
    SafeDelete(m_filterGrid);
    SafeDelete(m_filterOptionsList);
    SafeDelete(m_playlistVertical);

    SafeDelete(albumArtImage);
}

QString Music::getPluginName() const { return pluginName; }

QString Music::getPluginDisplayName() { return pluginName; }

QString Music::getPluginIcon() const { return pluginIcon; }

bool Music::getPluginStartPos() const { return false; }

bool Music::useBasicMenu() { return true; }

void Music::displayHomeScreenItems() { return; }

bool Music::initializeUI(MythUIType *ui) {
    bool err = false;

    UIUtilE::Assign(ui, m_musicDetailsUIGroup, "musicDetailsUI", &err);
    UIUtilE::Assign(ui, m_fileListMusicGrid, "fileListMusicGrid", &err);
    UIUtilE::Assign(ui, m_fileListSongs, "fileListDetails", &err);
    UIUtilE::Assign(ui, m_filterGrid, "filterGrid", &err);
    UIUtilE::Assign(ui, m_filterOptionsList, "filterOptionsList", &err);
    UIUtilE::Assign(ui, m_playlistVertical, "playlistVertical", &err);
    UIUtilE::Assign(ui, m_textSong, "song", &err);
    UIUtilE::Assign(ui, m_textArtist, "artist", &err);
    UIUtilE::Assign(ui, m_textAlbum, "album", &err);
    UIUtilE::Assign(ui, m_coverart, "coverart", &err);
    UIUtilE::Assign(ui, m_blackhole_border, "mm_blackhole_border", &err);
    UIUtilE::Assign(ui, m_seekbar, "seekbar", &err);
    UIUtilE::Assign(ui, m_musicTitle, "musicTitle", &err);
    UIUtilE::Assign(ui, m_musicDuration, "musicDuration", &err);
    UIUtilE::Assign(ui, m_trackProgress, "trackprogress", &err);
    UIUtilE::Assign(ui, m_playingOn, "playingOn", &err);
    UIUtilE::Assign(ui, m_next_buttonOn, "next_buttonOn", &err);
    UIUtilE::Assign(ui, m_ff_buttonOn, "ff_buttonOn", &err);
    UIUtilE::Assign(ui, m_playingOff, "playingOff", &err);
    UIUtilE::Assign(ui, m_next_buttonOff, "next_buttonOff", &err);
    UIUtilE::Assign(ui, m_ff_buttonOff, "ff_buttonOff", &err);
    UIUtilE::Assign(ui, m_hint, "hint", &err);

    if (!err) {
        m_trackProgress->SetStart(0);
        m_trackProgress->SetTotal(100);

        mm_albums_icon = createImageCachePath("ma_mm_albums.png");
        mm_alltracks_icon = createImageCachePath("ma_mm_alltracks.png");
        mm_artists_icon = createImageCachePath("ma_mm_artists.png");
        mm_genres_icon = createImageCachePath("ma_mm_genres.png");
        mm_playlist_icon = createImageCachePath("ma_mm_playlists.png");

        showMusicUI(false);

        albumArtImage = ui->GetPainter()->GetFormatImage();

        connect(m_fileListMusicGrid, SIGNAL(itemClicked(MythUIButtonListItem *)), this, SLOT(fileListMusicGridClickedCallback(MythUIButtonListItem *)));
        connect(m_fileListMusicGrid, SIGNAL(itemSelected(MythUIButtonListItem *)), this, SLOT(fileListMusicGridSelectedCallback(MythUIButtonListItem *)));
        connect(m_fileListMusicGrid, SIGNAL(itemVisible(MythUIButtonListItem *)), this, SLOT(fileListMusicGridVisibleCallback(MythUIButtonListItem *)));

        connect(m_fileListSongs, SIGNAL(itemClicked(MythUIButtonListItem *)), this, SLOT(addToPlaylistClickedCallback(MythUIButtonListItem *)));
        connect(m_playlistVertical, SIGNAL(itemClicked(MythUIButtonListItem *)), this, SLOT(addToPlaylistClickedCallback(MythUIButtonListItem *)));
        connect(m_filterGrid, SIGNAL(itemClicked(MythUIButtonListItem *)), this, SLOT(filterGridClickedCallback(MythUIButtonListItem *)));
        connect(m_filterGrid, SIGNAL(itemSelected(MythUIButtonListItem *)), this, SLOT(filterGridSelectedCallback(MythUIButtonListItem *)));
        connect(m_filterOptionsList, SIGNAL(itemClicked(MythUIButtonListItem *)), this, SLOT(filterOptionsListClickedCallback(MythUIButtonListItem *)));

        hintTimer = new QTimer(this); // displays a hint message in the music app
        connect(hintTimer, SIGNAL(timeout()), this, SLOT(hintTimerSlot()));

        playbackTimer = new QTimer(this); // update the playback status in the music app
        connect(playbackTimer, SIGNAL(timeout()), this, SLOT(playbackTimerSlot()));
    }

    return err;
}

void Music::load(const QString label, const QString data) {
    LOG(VB_GENERAL, LOG_DEBUG, "Music::load");

    m_toggleSearchVisibleCallback(true);

    if (data.length() < 2) {
        musicMode = 1;
        loadMusic();
    } else {
        updateMediaListCallback(label, data);
    }
}

void Music::loadMusic() {
    LOG(VB_GENERAL, LOG_DEBUG, "loadMusic()");

    controls->setActivePlayer(1);
    controls->setAudioLibraryScan();

    loadSongsMain("", "artists");
    exitToMainMenuSleepTimer->stop();
}

void Music::updateMediaListCallback(const QString &label, const QString &data) {
    LOG(VB_GENERAL, LOG_DEBUG, "updateMediaListCallback(): " + data);

    if (data.startsWith("/")) {
        QStringList parts = data.split('/');

        if (data.startsWith("/artists/")) {
            loadSongsMain(parts.at(2), "artists");
        } else if (data.startsWith("/albums/")) {
            loadSongsMain(parts.at(2), "albums");
        } else if (data.startsWith("/playlists/")) {
            loadSongsMain(parts.at(2), "playlists");
        } else if (data.startsWith("/genres/")) {
            loadSongsMain(parts.at(2), "genres");
        }
    }
}

void Music::clearAndStopPlaylist() {
    controls->stopPlayBack();
    controls->playListClear();
    playbackTimer->stop();
}

/** \brief hide the helpful hint text after it has displayed once. */
void Music::hintTimerSlot() {
    if (m_hint)
        m_hint->SetVisible(false);

    if (hintTimer)
        hintTimer->stop();
}

QMap<QString, QStringList> Music::filterQMap(QMap<QString, QStringList> map, QString filterKey) {
    QMap<QString, QStringList> list;

    QMapIterator<QString, QStringList> i(map);
    while (i.hasNext()) {
        i.next();
        QString label = i.key();
        QString image = i.value().at(0);
        QString file = i.value().at(1);

        QString album = i.value().at(2);
        QString artist = i.value().at(3);
        QString genre = i.value().at(4);

        QStringList stringList;
        stringList.append(image);
        stringList.append(file);

        if (filterKey.compare("artist") == 0) {
            list.insert(artist, stringList);
        } else if (filterKey.compare("album") == 0) {
            list.insert(album, stringList);
        } else if (filterKey.compare("genre") == 0) {
            list.insert(genre, stringList);
        } else {
            list.insert(label, stringList);
        }
    }
    return list;
}

void Music::fileListMusicGridClickedCallback(MythUIButtonListItem *item) {
    QString label = item->GetText("buttontext2");
    LOG(VB_GENERAL, LOG_DEBUG, "fileListMusicGridClickedCallback: " + label);

    if (label.contains("[Album]")) {
        label = label.replace("[Album] ", "");
        loadSongsMain(label, "albums");
        refreshGuiPlaylist();
    } else if (label.contains("Artist]")) {
        label = label.replace("[Artist] ", "");
        label = label.replace("[Video,Artist] ", "");
        loadSongsMain(label, "artists");
        refreshGuiPlaylist();
    } else if (label.contains("[Song]")) {
        QString getData = item->GetData().toString();
        addToPlaylistClickedCallback(item);
    } else {
        SetFocusWidget(m_fileListSongs);
    }
}

/** \brief show the music UI */
void Music::showMusicUI(bool show) {
    showMusicPlayingBar(show);
    setWidgetVisibility(m_musicDetailsUIGroup, show);
    setWidgetVisibility(m_fileListMusicGrid, show);
    setWidgetVisibility(m_fileListSongs, show);
    setWidgetVisibility(m_filterGrid, show);
    setWidgetVisibility(uiCtx->fileListGrid, !show);
    setWidgetVisibility(m_musicTitle, show);
    setWidgetVisibility(uiCtx->title, !show);
    setWidgetVisibility(uiCtx->plot, !show);
    setWidgetVisibility(m_filterOptionsList, show);
    setWidgetVisibility(m_playlistVertical, true);
    setWidgetVisibility(m_hint, false);
    setWidgetVisibility(uiCtx->androidMenuBtn, false);
}

/** \brief set the music party mode that plays a random track */
void Music::setPartyMode() {
    partyMode = true;
    delay(1);
    controls->setPartyMode();
}

void Music::showMusicPlayingBar(bool show) {
    m_next_buttonOn->SetVisible(false);
    m_ff_buttonOn->SetVisible(false);
    m_playingOn->SetVisible(false);

    m_next_buttonOff->SetVisible(show);
    m_ff_buttonOff->SetVisible(show);
    m_playingOff->SetVisible(show);

    if (m_currentMusicButton == 0 || !show) {

    } else if (m_currentMusicButton == 1) {
        m_playingOn->SetVisible(true);
        m_playingOff->SetVisible(false);
        SetFocusWidget(m_playingOn);
    } else if (m_currentMusicButton == 2) {
        m_next_buttonOn->SetVisible(true);
        m_next_buttonOff->SetVisible(false);
        SetFocusWidget(m_next_buttonOn);
    } else if (m_currentMusicButton == 3) {
        m_ff_buttonOn->SetVisible(true);
        m_ff_buttonOff->SetVisible(false);
        SetFocusWidget(m_ff_buttonOn);
    }

    m_textSong->SetVisible(show);
    m_textArtist->SetVisible(show);
    m_textAlbum->SetVisible(show);
    m_coverart->SetVisible(show);
    m_blackhole_border->SetVisible(show);
    m_seekbar->SetVisible(show);
    m_musicDuration->SetVisible(show);
    m_trackProgress->SetVisible(show);
}

void Music::loadMusicSetup() {
    LOG(VB_GENERAL, LOG_DEBUG, "Music::loadMusicSetup(");

    m_fileListMusicGrid->Reset();
    m_filterGrid->Reset();
    m_filterOptionsList->Reset();
    m_playlistVertical->Reset();

    auto *item0 = new MythUIButtonListItem(m_filterGrid, "");
    item0->SetImage(mm_alltracks_icon.replace("file://", ""));
    item0->SetData("alltracks");
    auto *item1 = new MythUIButtonListItem(m_filterGrid, "");
    item1->SetImage(mm_artists_icon.replace("file://", ""));
    item1->SetData("artists");
    auto *item2 = new MythUIButtonListItem(m_filterGrid, "");
    item2->SetImage(mm_albums_icon.replace("file://", ""));
    item2->SetData("albums");
    auto *item3 = new MythUIButtonListItem(m_filterGrid, "");
    item3->SetImage(mm_genres_icon.replace("file://", ""));
    item3->SetData("genres");

    auto *itemI = new MythUIButtonListItem(m_filterGrid, "");
    itemI->SetData("BlankR");

    auto *item4 = new MythUIButtonListItem(m_filterGrid, "");
    item4->SetImage(mm_artists_icon.replace("file://", ""));
    item4->SetData("artists2");
    auto *item5 = new MythUIButtonListItem(m_filterGrid, "");
    item5->SetImage(mm_albums_icon.replace("file://", ""));
    item5->SetData("albums2");

    auto *item6 = new MythUIButtonListItem(m_filterGrid, "");
    item6->SetImage(mm_playlist_icon.replace("file://", ""));
    item6->SetData("playlists");

    auto *itemII = new MythUIButtonListItem(m_filterGrid, "");
    itemII->SetData("BlankR");

    QString crossfadeStatus = "Crossfade Off";
    if (controls->getCrossFade()) {
        crossfadeStatus = "Crossfade On";
    }

    if (musicMode == 1) {
        new MythUIButtonListItem(m_filterOptionsList, "Music Only");
    } else if (musicMode == 2) {
        new MythUIButtonListItem(m_filterOptionsList, "Video Only");
    }

    new MythUIButtonListItem(m_filterOptionsList, crossfadeStatus);
    new MythUIButtonListItem(m_filterOptionsList, "Party Mode Off");
    new MythUIButtonListItem(m_filterOptionsList, "Visualizer");

    showMusicUI(true);
    playbackTimer->start(1 * 1000);
}

QMap<QString, QStringList> Music::getMusicHelper(QString methodValue, QString type, QString filterKey, QString filterValue, QString operatorValue) {
    LOG(+VB_GENERAL, LOG_DEBUG, "getMusicHelper()" + methodValue);

    QMap<QString, QStringList> artistList;
    QJsonArray properties;
    properties.push_back("thumbnail");
    QJsonObject params;

    if (methodValue == "AudioLibrary.GetSongs") {
        properties.push_back("file");
    } else if (methodValue == "VideoLibrary.GetMusicVideos") {
        properties.push_back("album");
        properties.push_back("artist");
        properties.push_back("genre");
        properties.push_back("file");
    }

    if (!filterKey.isEmpty()) {
        QJsonObject filter;
        filter["field"] = filterKey;
        filter["operator"] = operatorValue;
        filter["value"] = filterValue;
        params["filter"] = filter;
    }

    QJsonValue answer = controls->callJsonRpc(methodValue, params, properties);
    QJsonObject rootObj = answer.toObject();
    QJsonArray items = rootObj.value(type).toArray();

    for (const QJsonValue &v : items) {
        QJsonObject obj = v.toObject();
        QString label = obj.value("label").toString();
        if (label.isEmpty())
            continue;

        QString thumbnail = obj.value("thumbnail").toString();
        QString file, album, artist, genre;

        if (methodValue == "AudioLibrary.GetSongs") {
            file = obj.value("file").toString();
        } else if (methodValue == "VideoLibrary.GetMusicVideos") {
            file = obj.value("file").toString();
            album = obj.value("album").toString();

            QJsonArray aArr = obj.value("artist").toArray();
            if (!aArr.isEmpty())
                artist = aArr.at(0).toString();

            QJsonArray gArr = obj.value("genre").toArray();
            if (!gArr.isEmpty())
                genre = gArr.at(0).toString();
        } else if (methodValue == "AudioLibrary.GetAlbums") {
            album = QString::number(obj.value("albumid").toInt());
        }

        QStringList values{thumbnail, file, album, artist, genre};

        artistList.insert(label, values);
    }

    return artistList;
}

/** \brief helper to display music by artist */
QMap<QString, QStringList> Music::getByArtist(QString artist) {

    if (musicMode == 1) {
        if (!artist.compare("") == 0) {
            return getMusicHelper("AudioLibrary.GetArtists", "artists", "artist", artist, "contains");
        } else {
            return getMusicHelper("AudioLibrary.GetArtists", "artists", "", "", "is");
        }
    } else { // musicMode == 2
        if (!artist.compare("") == 0) {
            return filterQMap(getMusicHelper("VideoLibrary.GetMusicVideos", "musicvideos", "artist", artist, "contains"), "artist");
        } else {
            return filterQMap(getMusicHelper("VideoLibrary.GetMusicVideos", "musicvideos", "", "", "contains"), "artist");
        }
    }
}

/** \brief helper to display music by album */
QMap<QString, QStringList> Music::getByAlbums(QString artist) {
    if (musicMode == 1) {
        QString fetchArtist = "";
        if (!artist.compare("") == 0) {
            fetchArtist = "artist";
        }
        return getMusicHelper("AudioLibrary.GetAlbums", "albums", fetchArtist, artist, "is");
    } else { // musicMode == 2
        QString fetchArtist = "";
        if (!artist.compare("") == 0) {
            fetchArtist = "artist";
        }
        return filterQMap(getMusicHelper("VideoLibrary.GetMusicVideos", "musicvideos", fetchArtist, artist, "is"), "album");
    }
}
QMap<QString, QStringList> Music::getByAlbumsSearch(QString search) {
    if (musicMode == 1) {
        return getMusicHelper("AudioLibrary.GetAlbums", "albums", "album", search, "contains");
    } else { // musicMode == 2
        return filterQMap(getMusicHelper("VideoLibrary.GetMusicVideos", "musicvideos", "album", search, "contains"), "album");
    }
}

QMap<QString, QStringList> Music::getByAlbumsWithGenre(QString genres) {
    if (musicMode == 1) {
        return getMusicHelper("AudioLibrary.GetAlbums", "albums", "genre", genres, "is");
    } else { // musicMode == 2
        return filterQMap(getMusicHelper("VideoLibrary.GetMusicVideos", "musicvideos", "genre", genres, "is"), "album");
    }
}

/** \brief helper to display music by genres */
QMap<QString, QStringList> Music::getByGenres() {
    if (musicMode == 1) {
        return getMusicHelper("AudioLibrary.GetGenres", "genres", "", "", "is");
    } else { // musicMode == 2
        return filterQMap(getMusicHelper("VideoLibrary.GetMusicVideos", "musicvideos", "", "", ""), "genre");
    }
}

/** \brief display music by artist */
void Music::loadArtists(bool listview) {
    LOG(VB_GENERAL, LOG_DEBUG, "loadArtists()");
    showMusicUI(false);
    m_toggleSearchVisibleCallback(false);

    QStringListIterator list = addSpacingToList(getByArtist(""), listview);
    while (list.hasNext()) {
        QStringList split = list.next().split(",");
        QString label = split.at(0);
        QString image = split.at(1);

        m_loadProgramCallback(label, appPathName + "/artists/" + label, image, nullptr);
    }
}

/** \brief display music by album */
void Music::loadAlbums(bool listview) {
    LOG(VB_GENERAL, LOG_DEBUG, "loadAlbums()");
    showMusicUI(false);
    m_toggleSearchVisibleCallback(false);

    QStringListIterator list = addSpacingToList(getByAlbums(""), listview);
    while (list.hasNext()) {
        QStringList split = list.next().split(",");
        QString label = split.at(0);
        QString image = split.at(1);

        m_loadProgramCallback(label, appPathName + "/albums/" + label, image, nullptr);
    }
}
/** \brief display music by genre */
void Music::loadGenres() {
    LOG(VB_GENERAL, LOG_DEBUG, "loadGenres()");
    showMusicUI(false);
    m_toggleSearchVisibleCallback(false);

    if (musicMode == 2 || musicMode == 3) {
        loadMusicHelper("[Video,Genre] ", filterQMap(getMusicHelper("VideoLibrary.GetMusicVideos", "musicvideos", "", "", "contains"), "genre"), uiCtx->fileListGrid);
    }
    if (!(musicMode == 1 || musicMode == 3)) {
        return;
    }

    loadMusicHelper("", getByGenres(), uiCtx->fileListGrid);
}

void Music::loadPlaylists() {
    LOG(VB_GENERAL, LOG_DEBUG, "loadPlaylists()");
    m_toggleSearchVisibleCallback(false);

    QJsonArray properties{"label", "thumbnail", "file"};
    QJsonObject params;
    params["directory"] = "special://musicplaylists";

    QJsonValue answer = controls->callJsonRpc("Files.GetDirectory", params, properties);
    QJsonArray files = answer.toObject().value("files").toArray();

    for (const QJsonValue &v : files) {
        QJsonObject obj = v.toObject();
        QString label = obj.value("label").toString();
        if (label.isEmpty())
            continue;

        QString image = obj.value("thumbnail").toString();
        QString url = obj.value("file").toString();
        m_loadProgramCallback(label, appPathName + "/playlists/" + url, image, nullptr);
    }
}

void Music::updateMusicPlaylistUI() {
    if (m_playlistVertical->GetCount() == 0) {
        refreshGuiPlaylist();
    }

    for (int i = 0; i < m_playlistVertical->GetCount(); i++) {
        MythUIButtonListItem *item = m_playlistVertical->GetItemAt(i);
        QString label = item->GetText();
        if (label.compare("") == 0) {
            label = item->GetText("buttontext2");
        }
        item->SetText(label, "");
        item->SetText("", "buttontext2");

        if (m_textSong->GetText().compare(label) == 0) {
            item->SetText(label, "");
            item->SetText(label, "buttontext2");
        }
    }
}

void Music::playbackTimerSlot() {
    PlaybackInfo info = m_PlaybackInfoCallback();

    m_musicDuration->SetText(info.currentTime + " / " + removeHoursIfZero(info.duration));

    QTime durationTime = QTime::fromString(info.duration, "hh:mm:ss");
    qint64 totalMs = QTime(0, 0).msecsTo(durationTime);
    int percent = totalMs > 0 ? static_cast<int>((static_cast<double>(info.elapsedMs) / totalMs) * 100.0) : 0;
    percent = qBound(0, percent, 100);

    m_trackProgress->SetUsed(percent);

    updateMusicPlaylistUI();
}

/** \brief is the song/label currently in the playlist?
 * \param label - name of the song
 * \return -1 not found. number of matches*/
int Music::isInPlaylist(QString label) {
    label.remove(QRegExp("^[0-9]*")); // remove trailing numbers and spaces
    label = label.trimmed();

    for (int i = 0; i < m_playlistVertical->GetCount(); i++) {
        QString labelBtn = m_playlistVertical->GetItemAt(i)->GetText();

        if (labelBtn.compare(label) == 0) {
            return i;
        }
    }
    return -1;
}
/** \brief remove the song/label from the playlist?
 * \param label - name of the song */
void Music::removeFromPlaylist(QString label) {
    int inPlaylist = isInPlaylist(label);
    if (inPlaylist != -1) {
        controls->removeFromPlaylist(inPlaylist);
    }
    refreshGuiPlaylist();
}

void Music::addToPlaylistClickedCallback(MythUIButtonListItem *item) {
    LOG(VB_GENERAL, LOG_DEBUG, "addToPlaylistClickedCallback()");
    if (dialog->getLoader()->IsVisible()) { // button debounce
        return;
    }

    dialog->getLoader()->SetVisible(true);

    QString label = item->GetText();
    if (item->GetText("buttontext2").size() > label.size()) {
        label = item->GetText("buttontext2");
    }

    int inPlaylist = isInPlaylist(label);

    int static run = 0;
    run++;
    if (run == 2) {
        m_hint->SetText(tr("Tip: Select Twice to play the song now"));
        m_hint->SetVisible(true);
        hintTimer->start(10 * 1000);
    }

    if (partyMode) {
        partyMode = false;
        m_filterOptionsList->GetItemAt(1)->SetText(tr("Party Mode Off"));
        controls->playListClear();
        controls->stopPlayBack();
        m_playlistVertical->Reset();
        inPlaylist = -1;
        refreshGuiPlaylist();
    }

    if (inPlaylist == -1) {
        QVariantMap info = item->GetData().toMap();
        ProgramData programData("", info["setData"].toString());
        controls->playListAdd(friendlyUrl(programData.getFilePathParam()));

        if (!controls->isPlaying()) {
            controls->playListOpen(1);
        }
        refreshGuiPlaylist();
    } else { // already in playlist
        controls->playListOpen(inPlaylist);
    }
    dialog->getLoader()->SetVisible(false);
}

/** \brief refresh/update the playlist. */
void Music::refreshGuiPlaylist() {
    QJsonArray properties;
    QJsonObject params;
    params["playlistid"] = controls->getActivePlayer(); // fix!

    QJsonValue answer = controls->callJsonRpc("Playlist.GetItems", params, properties);
    QJsonArray items = answer.toObject().value("items").toArray();

    m_playlistVertical->Reset();
    for (const QJsonValue &v : items) {
        QString label = v.toObject().value("label").toString();
        if (!label.isEmpty())
            new MythUIButtonListItem(m_playlistVertical, label);
    }
}

/** \brief add a newline when the 1st character changes in the list */
QStringList Music::addSpacingToList(QMap<QString, QStringList> map, bool listview) {
    QStringList list;
    char currentLetter = 'A';

    QMapIterator<QString, QStringList> i(map);
    while (i.hasNext()) {
        i.next();
        QString label = i.key();
        QString image = i.value().at(0);

        if (listview) {
            while (label.at(0).toUpper().toLatin1() > currentLetter) {
                currentLetter = label.at(0).toUpper().toLatin1();
            }

            if (label.at(0).toUpper().toLatin1() == currentLetter) {
                list.append("split,");
                currentLetter = (char)((int)currentLetter + 1);
            }
        }

        list.append(label + "," + image);
    }
    return list;
}

void Music::loadMusicHelper(QString labelPrefix, QMap<QString, QStringList> loadMusicType, MythUIButtonList *m_fileList) {
    QMapIterator<QString, QStringList> i(loadMusicType);
    int count = 0;
    while (i.hasNext()) {
        count++;
        i.next();
        QString label = i.key();
        QString image = i.value().at(0);
        QString file = i.value().at(1);
        QString setData = "";

        if (file.compare("") == 0) {
            setData = appPathName + "/genres/" + label;
        } else {
            setData = i.value().at(1) + "~|";
        }

        m_loadProgramCallback(labelPrefix + label, setData, image, m_fileList);

        if (count % 3 == 0 and m_fileListMusicGrid == m_fileList) { // load an invisible colum to help with navigation
            m_loadProgramCallback("BlankR", "", "", m_fileListMusicGrid);
        }
    }
}

void Music::loadSongs(QString album) {
    m_musicTitle->SetText(album);
    m_toggleSearchVisibleCallback(true);

    album.replace("[Video,Artist] ", "");
    album.replace("[Video,Album] ", "");
    album.replace("[Video,Genre] ", "");

    if (musicMode == 2) {
        auto map = getMusicHelper("VideoLibrary.GetMusicVideos", "musicvideos", "album", album, "is");
        loadMusicHelper("", filterQMap(map, ""), m_fileListSongs);
        return;
    }

    QJsonArray properties{"artist", "thumbnail", "file", "album"};

    QJsonObject params;
    QJsonObject sort;
    sort["order"] = "ascending";
    sort["method"] = "artist";
    params["sort"] = sort;

    if (!album.isEmpty()) {
        QJsonObject filter;
        filter["album"] = album;
        params["filter"] = filter;
    }

    QJsonValue answer = controls->callJsonRpc("AudioLibrary.GetSongs", params, properties);
    QJsonObject rootObj = answer.toObject();
    QJsonArray songs = rootObj.value("songs").toArray();

    int count = 0;
    for (const QJsonValue &v : songs) {
        QJsonObject obj = v.toObject();
        QString label = obj.value("label").toString();
        if (label.isEmpty())
            continue;

        QString thumbnail = obj.value("thumbnail").toString();
        QString file = obj.value("file").toString();

        QString artist;
        QJsonArray aArr = obj.value("artist").toArray();
        if (!aArr.isEmpty())
            artist = aArr.at(0).toString();

        ++count;
        QString number = QString::number(count) + "   ";
        if (count < 10)
            number += " ";

        QString payload = file + "~|" + thumbnail;
        m_loadProgramCallback(number + label, payload, thumbnail, m_fileListSongs);
    }
}

/** \brief default view when loading the music app */
void Music::loadSongsMain(QString value, QString type) {
    LOG(VB_GENERAL, LOG_DEBUG, "Music::loadSongsMain");
    loadMusicSetup();

    if (type.compare("playlists") == 0) {
        controls->play(value, "");
        value = "";
    }

    QString album = "";
    if (type.compare("artists") == 0) {
        album = value;
    }

    QMapIterator<QString, QStringList> i = getByAlbums(album);
    if (type.compare("genres") == 0) {
        value = value.replace("[Video,Genre] ", "");
        value = value.replace("Genre] ", "");
        i = getByAlbumsWithGenre(value);
    }

    int count = 0;
    while (i.hasNext()) {
        count++;
        i.next();
        QString label = i.key();
        QString image = i.value().at(0);

        if (type.compare("albums") == 0 and !value.compare(label) == 0) {
            continue;
        }

        m_loadProgramCallback(label, ::createProgramData("", "", image, false, ""), image, m_fileListMusicGrid);
        if (count % 3 == 0) { // load an invisible colum to help with navigation
            m_loadProgramCallback("BlankR", "", "", m_fileListMusicGrid);
        }
    }

    SetFocusWidget(m_fileListMusicGrid);
    updateMusicPlayingBarStatus();
}

void Music::previousTrack() { controls->inputActionHelper("skipprevious"); }

void Music::nextTrack() { controls->inputActionHelper("skipnext"); }

/** \brief update the music playing bar labels. song playing etc */
void Music::updateMusicPlayingBarStatus() {
    LOG(VB_GENERAL, LOG_DEBUG, "updateMusicPlayingBarStatus()");
    QString answer = controls->playerGetItem();

    QJsonDocument jsonDocument = QJsonDocument::fromJson(answer.toLocal8Bit());
    QJsonObject jObject = jsonDocument.object();
    QVariantMap mapItem = jObject["item"].toObject().toVariantMap();

    QString thumbnail = mapItem["thumbnail"].toString();

    QList listArtist = mapItem["artist"].toList();
    if (listArtist.size() > 0) {
        m_textArtist->SetText(listArtist.at(0).toString());
    }

    QString label = mapItem["label"].toString();
    m_textSong->SetText(label);
    m_textAlbum->SetText(mapItem["album"].toString());

    if (label.compare("") == 0) {
        showMusicPlayingBar(false);
    } else {
        showMusicPlayingBar(true);
    }

    downloadImage(thumbnail);
}

/** \brief download the cover art image */
void Music::downloadImage(QString thumbnailPath) {
    LOG(VB_GENERAL, LOG_DEBUG, "downloadImage() " + thumbnailPath);

    if (thumbnailPath.compare("") == 0) {
        QString filename = QString("%1%2").arg(GetShareDir()).arg("themes/default/mv_browse_nocover.png");

        m_coverart->SetFilename(filename);
        m_coverart->Load();
        return;
    }

    NetRequest nr(username, password, ip, port, false);
    QImage image;
    image.loadFromData(nr.downloadImage(urlEncode(thumbnailPath), false));
    albumArtImage->Assign(image);
    m_coverart->SetImage(albumArtImage);
}

void Music::fileListMusicGridSelectedCallback(MythUIButtonListItem *item) {
    QString label = item->GetText("buttontext2");

    if (item->GetData().toString().compare("BlankR") == 0) {
        SetFocusWidget(m_fileListSongs);
        return;
    }
    m_fileListSongs->Reset();

    if (!label.contains("[Artist]") and !label.contains("[Song]")) {
        loadSongs(label);
        delayMilli(50);
    }

    if (label.contains("[Song]") || label.contains("[Video]")) {
        label = label.replace("[Song] ", "");
        label = label.replace("[Video] ", "");
        auto *item2 = new MythUIButtonListItem(m_fileListSongs, label);
        item2->SetData(item->GetData().toString());
    }
}

void Music::fileListMusicGridVisibleCallback(MythUIButtonListItem *item) { m_DisplayImageCallback(item, m_fileListMusicGrid); }

void Music::filterGridSelectedCallback(MythUIButtonListItem *item) {
    QString label = item->GetData().toString();

    if (label.compare("BlankR") == 0) {
        SetFocusWidget(m_filterOptionsList);
    }
}

/** \brief the filterGird (albums, artists, genres etc) had been clicked. load the corresponding filter */
void Music::filterGridClickedCallback(MythUIButtonListItem *item) {
    QString label = item->GetData().toString();
    if (label.compare("alltracks") == 0) {
        loadSongsMain("", "");
        return;
    } else if (label.compare("albums") == 0) {
        loadAlbums(false);
    } else if (label.compare("artists") == 0) {
        loadArtists(false);
    } else if (label.compare("genres") == 0) {
        loadGenres();
    } else if (label.compare("albums2") == 0) {
        loadAlbums(true);
    } else if (label.compare("artists2") == 0) {
        loadArtists(true);
    } else if (label.compare("playlists") == 0) {
        loadPlaylists();
    }
    showMusicUI(false);
    SetFocusWidget(uiCtx->fileListGrid);
}

/** \brief the filterOptions (Crossfade, Party Mode Video & Music Only etc) had been clicked. load the corresponding option */
void Music::filterOptionsListClickedCallback(MythUIButtonListItem *item) {
    QString label = item->GetText();
    qDebug() << label;

    if (label.compare(tr("Crossfade On")) == 0) {
        controls->setCrossFade(0);
        item->SetText(tr("Crossfade Off"));
    } else if (label.compare(tr("Crossfade Off")) == 0) {
        controls->setCrossFade(10);
        item->SetText(tr("Crossfade On"));
    }

    if (label.compare(tr("Party Mode On")) == 0) {
        item->SetText(tr("Party Mode Off"));
        m_playlistVertical->Reset();
        controls->stopPlayBack();
    } else if (label.compare(tr("Party Mode Off")) == 0) {
        if (musicMode == 1) {
            item->SetText(tr("Party Mode On"));
            setPartyMode();
        }
    }

    if (label.compare(tr("Music Only")) == 0) {
        item->SetText(tr("Video Only"));
        musicMode = 2;
        loadSongsMain("", "artists");
    } else if (label.compare(tr("Video Only")) == 0) {
        item->SetText(tr("Music Only"));
        musicMode = 1;
        loadSongsMain("", "artists");
    }

    if (label.compare(tr("Visualizer")) == 0 and m_playlistVertical->GetCount() > 0) {
        m_fullscreenCallback();
    }
}

void Music::search(const QString &searchText) {
    LOG(VB_GENERAL, LOG_DEBUG, "musicSearch()");
    m_fileListMusicGrid->Reset();
    m_fileListSongs->Reset();

    QString vidPrefix = "";
    if (musicMode == 2)
        vidPrefix = "Video,";

    loadMusicHelper("[" + vidPrefix + "Artist] ", getByArtist(searchText), m_fileListMusicGrid);
    loadMusicHelper("[" + vidPrefix + "Album] ", getByAlbumsSearch(searchText), m_fileListMusicGrid);

    if (musicMode == 1) {
        loadMusicHelper("[Song] ", getMusicHelper("AudioLibrary.GetSongs", "songs", "title", searchText, "contains"), m_fileListMusicGrid);
    }
    if (musicMode == 2) {
        loadMusicHelper("[Video] ", filterQMap(getMusicHelper("VideoLibrary.GetMusicVideos", "musicvideos", "title", searchText, "contains"), ""), m_fileListMusicGrid);
    }

    delayMilli(100);
    SetFocusWidget(m_fileListMusicGrid);
}

void Music::onTextMessageReceived(const QString &method, const QString &message) {
    if (method == "Player.OnAVStart") {
        updateMusicPlayingBarStatus(); // update the music status bar
        refreshGuiPlaylist();

        if (message.contains("video")) {
            // dialog->confirmDialog(tr("Do you want to open this video in fullscreen?"), "fullscreen");
        }
    } else if (method == "Player.OnStop") {
        m_textArtist->SetText("");
        m_textSong->SetText("");
        m_textAlbum->SetText("");
        showMusicPlayingBar(false);
        SetFocusWidget(m_fileListMusicGrid);
    }
}

void Music::exitPlugin() {
    showMusicUI(false);
    clearAndStopPlaylist();
}

bool Music::handleAction(const QString &action, MythUIType *focusWidget) {
    bool musicPlayerFullscreenOpen = false; // todo: not working

    if (action == "NEXTTRACK") {
        nextTrack();
    } else if (action == "PREVTRACK") {
        previousTrack();
    } else if ((action == "RIGHT") and musicPlayerFullscreenOpen) {
        nextTrack();
    } else if ((action == "LEFT") and musicPlayerFullscreenOpen) {
        previousTrack();
    } else if ((action == "RIGHT") and focusWidget == m_fileListSongs and m_playlistVertical->GetCount() > 0) {
        SetFocusWidget(m_playlistVertical);
    } else if ((action == "LEFT" || action == "BACK" || action == "ESCAPE") and focusWidget == m_playlistVertical) {
        SetFocusWidget(m_fileListSongs);
    } else if ((action == "LEFT" || action == "BACK" || action == "ESCAPE") and focusWidget == m_fileListSongs) {
        int pos = m_fileListMusicGrid->GetCurrentPos();
        if (pos > 0) {
            pos = pos - 1;
        }
        m_fileListMusicGrid->SetItemCurrent(pos);
        SetFocusWidget(m_fileListMusicGrid);
    } else if ((action == "BACK" || action == "ESCAPE") and musicPlayerFullscreenOpen) {
        // stopPlayBack();
    } else if ((action == "BACK" || action == "ESCAPE") and focusWidget == m_fileListMusicGrid) {
        m_filterGrid->SetItemCurrent(0);
        SetFocusWidget(m_filterGrid);
    } else if ((action == "BACK" || action == "ESCAPE") and focusWidget == m_filterGrid) {
        if (uiCtx->SearchTextEdit->IsVisible()) {
            SetFocusWidget(uiCtx->SearchTextEdit);
        } else {
            // goBack();
        }

    } else if (action == "SEEKFFWD") {
        controls->seekFoward();
    } else if (action == "SEEKRWND") {
        controls->seekBack();
    } else if (action == "SELECT") {
        if (focusWidget == m_playingOn) {
            // pauseToggle();
        } else if (focusWidget == m_ff_buttonOn) {
            controls->seekFoward();
        } else if (focusWidget == m_next_buttonOn) {
            nextTrack();
        }
    } else if (action == "UP" && focusWidget == uiCtx->SearchTextEdit) {
        SetFocusWidget(m_fileListMusicGrid);
    } else if ((action == "DOWN" || action == "RIGHT") && focusWidget == uiCtx->searchButtonList) {
        SetFocusWidget(m_fileListMusicGrid);
    } else if ((action == "RIGHT") and focusWidget == m_playlistVertical and m_ff_buttonOff->IsVisible()) {
        m_currentMusicButton = 1; // m_playing
        showMusicPlayingBar(true);
    } else if ((action == "RIGHT") and focusWidget == m_playingOn) {
        m_currentMusicButton = 2; // m_next_button
        showMusicPlayingBar(true);
        SetFocusWidget(m_next_buttonOn);
    } else if ((action == "RIGHT") and focusWidget == m_next_buttonOn) {
        m_currentMusicButton = 3; // m_ff_button
        showMusicPlayingBar(true);
    } else if ((action == "RIGHT") and focusWidget == m_ff_buttonOn) {
        m_currentMusicButton = 0;
        showMusicPlayingBar(true);
        SetFocusWidget(m_playlistVertical);
    } else if ((action == "LEFT" || action == "BACK" || action == "ESCAPE") and focusWidget == m_ff_buttonOn) {
        m_currentMusicButton = 2; // m_next_button
        showMusicPlayingBar(true);
    } else if ((action == "LEFT" || action == "BACK" || action == "ESCAPE") and focusWidget == m_next_buttonOn) {
        m_currentMusicButton = 1; // m_playing
        showMusicPlayingBar(true);
    } else if ((action == "LEFT" || action == "BACK" || action == "ESCAPE") and focusWidget == m_playingOn) {
        m_currentMusicButton = 0;
        showMusicPlayingBar(true);
        SetFocusWidget(m_playlistVertical);
    } else if ((action == "LEFT" || action == "BACK" || action == "ESCAPE") and focusWidget == m_filterOptionsList) {
        m_filterGrid->SetItemCurrent(0, 0);
        SetFocusWidget(m_filterGrid);
    } else if ((action == "RIGHT") and focusWidget == m_filterOptionsList) {
        if (m_playlistVertical->GetCount() > 0) {
            SetFocusWidget(m_playlistVertical);
        } else {
            SetFocusWidget(m_fileListMusicGrid);
        }
    } else {
        return false; // Not handled
    }
    return true; // Handled
}
