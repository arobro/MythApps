#include "mythapps.h"

/** \class music_functions
 *  \brief Extension of MythApps class as file was getting too big. TODO: Put into seperate class */

/** \brief initialize Music state */
bool MythApps::initializeMusic() {
    bool err = false;
    UIUtilE::Assign(this, m_musicDetailsUIGroup, "musicDetailsUI", &err);
    UIUtilE::Assign(this, m_fileListMusicGrid, "fileListMusicGrid", &err);
    UIUtilE::Assign(this, m_fileListSongs, "fileListDetails", &err);
    UIUtilE::Assign(this, m_filterGrid, "filterGrid", &err);
    UIUtilE::Assign(this, m_filterOptionsList, "filterOptionsList", &err);
    UIUtilE::Assign(this, m_playlistVertical, "playlistVertical", &err);
    UIUtilE::Assign(this, m_textSong, "song", &err);
    UIUtilE::Assign(this, m_textArtist, "artist", &err);
    UIUtilE::Assign(this, m_textAlbum, "album", &err);
    UIUtilE::Assign(this, m_coverart, "coverart", &err);
    UIUtilE::Assign(this, m_blackhole_border, "mm_blackhole_border", &err);
    UIUtilE::Assign(this, m_seekbar, "seekbar", &err);
    UIUtilE::Assign(this, m_musicTitle, "musicTitle", &err);
    UIUtilE::Assign(this, m_musicDuration, "musicDuration", &err);
    UIUtilE::Assign(this, m_trackProgress, "trackprogress", &err);
    UIUtilE::Assign(this, m_playingOn, "playingOn", &err);
    UIUtilE::Assign(this, m_next_buttonOn, "next_buttonOn", &err);
    UIUtilE::Assign(this, m_ff_buttonOn, "ff_buttonOn", &err);
    UIUtilE::Assign(this, m_playingOff, "playingOff", &err);
    UIUtilE::Assign(this, m_next_buttonOff, "next_buttonOff", &err);
    UIUtilE::Assign(this, m_ff_buttonOff, "ff_buttonOff", &err);
    UIUtilE::Assign(this, m_hint, "hint", &err);

    m_trackProgress->SetStart(0);
    m_trackProgress->SetTotal(100);

    connect(m_fileListMusicGrid, SIGNAL(itemClicked(MythUIButtonListItem *)), this, SLOT(fileListMusicGridClickedCallback(MythUIButtonListItem *)));
    connect(m_fileListMusicGrid, SIGNAL(itemSelected(MythUIButtonListItem *)), this, SLOT(fileListMusicGridSelectedCallback(MythUIButtonListItem *)));
    connect(m_fileListMusicGrid, SIGNAL(itemVisible(MythUIButtonListItem *)), this, SLOT(fileListMusicGridVisibleCallback(MythUIButtonListItem *)));

    connect(m_fileListSongs, SIGNAL(itemClicked(MythUIButtonListItem *)), this, SLOT(addToPlaylistClickedCallback(MythUIButtonListItem *)));
    connect(m_playlistVertical, SIGNAL(itemClicked(MythUIButtonListItem *)), this, SLOT(addToPlaylistClickedCallback(MythUIButtonListItem *)));
    connect(m_filterGrid, SIGNAL(itemClicked(MythUIButtonListItem *)), this, SLOT(filterGridClickedCallback(MythUIButtonListItem *)));
    connect(m_filterGrid, SIGNAL(itemSelected(MythUIButtonListItem *)), this, SLOT(filterGridSelectedCallback(MythUIButtonListItem *)));
    connect(m_filterOptionsList, SIGNAL(itemClicked(MythUIButtonListItem *)), this, SLOT(filterOptionsListClickedCallback(MythUIButtonListItem *)));

    return err;
}

/** \brief set the music party mode that plays a random track */
void MythApps::setPartyMode() {
    partyMode = true;
    delay(1);
    controls->setPartyMode();
    refreshGuiPlaylist();
}

void MythApps::showMusicPlayingBar(bool show) {
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

bool MythApps::isMusicPlayingBarVisible() { return m_trackProgress->IsVisible(); }

/** \brief show the music UI */
void MythApps::showMusicUI(bool show) {
    showMusicPlayingBar(show);
    m_musicDetailsUIGroup->SetEnabled(show);
    m_musicDetailsUIGroup->SetVisible(show);
    m_fileListMusicGrid->SetVisible(show);
    m_fileListSongs->SetVisible(show);
    m_filterGrid->SetVisible(show);
    uiCtx->fileListGrid->SetVisible(!show);
    uiCtx->fileListGrid->SetEnabled(!show);
    m_musicTitle->SetVisible(show);
    uiCtx->title->SetVisible(!show);
    uiCtx->plot->SetVisible(!show);
    m_filterOptionsList->SetVisible(show);
    m_playlistVertical->SetVisible(show);
    m_playlistVertical->SetEnabled(true);

    uiCtx->androidMenuBtn->SetVisible(false);
    uiCtx->androidMenuBtn->SetEnabled(false);
}

void MythApps::previousTrack() { controls->inputActionHelper("skipprevious"); }

void MythApps::nextTrack() { controls->inputActionHelper("skipnext"); }

/** \brief update the music playing bar labels. song playing etc */
void MythApps::updateMusicPlayingBarStatus() {
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
void MythApps::downloadImage(QString thumbnailPath) {
    if (thumbnailPath.compare("") == 0) {
        QString filename = QString("%1%2").arg(GetShareDir()).arg("themes/default/mv_browse_nocover.png");

        m_coverart->SetFilename(filename);
        m_coverart->Load();
        return;
    }

    NetRequest *nr = new NetRequest(username, password, ip, port, false);
    QImage image;
    image.loadFromData(nr->downloadImage(urlEncode(thumbnailPath), false));
    MythImage *m_image = GetPainter()->GetFormatImage();
    m_image->Assign(image);
    m_coverart->SetImage(m_image);
    delete nr;
}

void MythApps::fileListMusicGridClickedCallback(MythUIButtonListItem *item) {
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

void MythApps::fileListMusicGridSelectedCallback(MythUIButtonListItem *item) {
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

void MythApps::fileListMusicGridVisibleCallback(MythUIButtonListItem *item) { displayImage(item, m_fileListMusicGrid); }

void MythApps::filterGridSelectedCallback(MythUIButtonListItem *item) {
    QString label = item->GetData().toString();

    if (label.compare("BlankR") == 0) {
        SetFocusWidget(m_filterOptionsList);
    }
}

/** \brief the filterGird (albums, artists, genres etc) had been clicked. load the corresponding filter */
void MythApps::filterGridClickedCallback(MythUIButtonListItem *item) {
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
void MythApps::filterOptionsListClickedCallback(MythUIButtonListItem *item) {
    QString label = item->GetText();

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
        goFullscreen();
    }
}

/** \brief is the song/label currently in the playlist?
 * \param label - name of the song
 * \return -1 not found. number of matches*/
int MythApps::isInPlaylist(QString label) {
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
void MythApps::removeFromPlaylist(QString label) {
    int inPlaylist = isInPlaylist(label);
    if (inPlaylist != -1) {
        controls->removeFromPlaylist(inPlaylist);
    }
    refreshGuiPlaylist();
}

void MythApps::addToPlaylistClickedCallback(MythUIButtonListItem *item) {
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
void MythApps::refreshGuiPlaylist() {
    LOG(VB_GENERAL, LOG_DEBUG, "refreshGuiPlaylist()");

    QJsonArray properties;
    QJsonObject params;
    params["playlistid"] = controls->getActivePlayer();

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
QStringList MythApps::addSpacingToList(QMap<QString, QStringList> map, bool listview) {
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

QMap<QString, QStringList> MythApps::getMusicHelper(QString methodValue, QString type, QString filterKey, QString filterValue, QString operatorValue) {
    LOG(VB_GENERAL, LOG_DEBUG, "getMusicHelper()");

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
QMap<QString, QStringList> MythApps::getByArtist(QString artist) {

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
QMap<QString, QStringList> MythApps::getByAlbums(QString artist) {
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
QMap<QString, QStringList> MythApps::getByAlbumsSearch(QString search) {
    if (musicMode == 1) {
        return getMusicHelper("AudioLibrary.GetAlbums", "albums", "album", search, "contains");
    } else { // musicMode == 2
        return filterQMap(getMusicHelper("VideoLibrary.GetMusicVideos", "musicvideos", "album", search, "contains"), "album");
    }
}

QMap<QString, QStringList> MythApps::getByAlbumsWithGenre(QString genres) {
    if (musicMode == 1) {
        return getMusicHelper("AudioLibrary.GetAlbums", "albums", "genre", genres, "is");
    } else { // musicMode == 2
        return filterQMap(getMusicHelper("VideoLibrary.GetMusicVideos", "musicvideos", "genre", genres, "is"), "album");
    }
}

/** \brief helper to display music by genres */
QMap<QString, QStringList> MythApps::getByGenres() {
    if (musicMode == 1) {
        return getMusicHelper("AudioLibrary.GetGenres", "genres", "", "", "is");
    } else { // musicMode == 2
        return filterQMap(getMusicHelper("VideoLibrary.GetMusicVideos", "musicvideos", "", "", ""), "genre");
    }
}

/** \brief display music by artist */
void MythApps::loadArtists(bool listview) {
    LOG(VB_GENERAL, LOG_DEBUG, "loadArtists()");
    showMusicUI(false);
    uiCtx->fileListGrid->Reset();
    loadBackButton();
    toggleSearchVisible(false);

    QStringListIterator list = addSpacingToList(getByArtist(""), listview);
    while (list.hasNext()) {
        QStringList split = list.next().split(",");
        QString label = split.at(0);
        QString image = split.at(1);
        loadProgram(label, createProgramData("artists~", "", image, false, ""), image);
    }
}

/** \brief display music by album */
void MythApps::loadAlbums(bool listview) {
    LOG(VB_GENERAL, LOG_DEBUG, "loadAlbums()");
    showMusicUI(false);
    uiCtx->fileListGrid->Reset();
    loadBackButton();
    toggleSearchVisible(false);

    QStringListIterator list = addSpacingToList(getByAlbums(""), listview);
    while (list.hasNext()) {
        QStringList split = list.next().split(",");
        QString label = split.at(0);
        QString image = split.at(1);
        loadProgram(label, createProgramData("albums", "", image, false, ""), image);
    }
}
/** \brief display music by genre */
void MythApps::loadGenres() {
    LOG(VB_GENERAL, LOG_DEBUG, "loadGenres()");
    showMusicUI(false);
    uiCtx->fileListGrid->Reset();
    loadBackButton();
    toggleSearchVisible(false);

    if (musicMode == 2 || musicMode == 3) {
        loadMusicHelper("[Video,Genre] ", filterQMap(getMusicHelper("VideoLibrary.GetMusicVideos", "musicvideos", "", "", "contains"), "genre"), uiCtx->fileListGrid);
    }
    if (!(musicMode == 1 || musicMode == 3)) {
        return;
    }

    loadMusicHelper("", getByGenres(), uiCtx->fileListGrid);
}
void MythApps::loadMusicHelper(QString labelPrefix, QMap<QString, QStringList> loadMusicType, MythUIButtonList *m_fileList) {
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
            setData = createProgramData("genres~", "", image, false, "");
        } else {
            setData = i.value().at(1) + "~|";
        }

        loadProgram(labelPrefix + label, setData, image, m_fileList);

        if (count % 3 == 0 and m_fileListMusicGrid == m_fileList) { // load an invisible colum to help with navigation
            loadProgram("BlankR", "", "", m_fileListMusicGrid);
        }
    }
}

/** \brief search music by artist */
void MythApps::musicSearch(QString search) {
    LOG(VB_GENERAL, LOG_DEBUG, "musicSearch()");
    m_fileListMusicGrid->Reset();
    m_fileListSongs->Reset();

    QString vidPrefix = "";
    if (musicMode == 2) {
        vidPrefix = "Video,";
    }

    loadMusicHelper("[" + vidPrefix + "Artist] ", getByArtist(search), m_fileListMusicGrid);
    loadMusicHelper("[" + vidPrefix + "Album] ", getByAlbumsSearch(search), m_fileListMusicGrid);

    if (musicMode == 1) {
        loadMusicHelper("[Song] ", getMusicHelper("AudioLibrary.GetSongs", "songs", "title", search, "contains"), m_fileListMusicGrid);
    }
    if (musicMode == 2) {
        loadMusicHelper("[Video] ", filterQMap(getMusicHelper("VideoLibrary.GetMusicVideos", "musicvideos", "title", search, "contains"), ""), m_fileListMusicGrid);
    }

    delayMilli(100);
    SetFocusWidget(m_fileListMusicGrid);
}

void MythApps::loadSongs(QString album) {
    m_musicTitle->SetText(album);
    toggleSearchVisible(true);

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
        loadProgram(number + label, payload, thumbnail, m_fileListSongs);
    }
}

void MythApps::loadMusicSetup() {
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

/** \brief default view when loading the music app */
void MythApps::loadSongsMain(QString value, QString type) {
    loadMusicSetup();

    if (type.compare("playlists") == 0) {
        play(value);
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

        loadProgram(label, createProgramData("", "", image, false, ""), image, m_fileListMusicGrid);
        if (count % 3 == 0) { // load an invisible colum to help with navigation
            loadProgram("BlankR", "", "", m_fileListMusicGrid);
        }
    }
    SetFocusWidget(m_fileListMusicGrid);

    updateMusicPlayingBarStatus();
}

void MythApps::loadMusic() {
    musicOpen = true;
    controls->setActivePlayer(1);
    controls->setAudioLibraryScan();
    loadSongsMain("", "artists");
    exitToMainMenuSleepTimer->stop();
}

void MythApps::clearAndStopPlaylist() {
    dialog->createBusyDialog(tr("Stopping Music..."));
    controls->stopPlayBack();
    controls->playListClear();
    delayMilli(100);
    dialog->closeBusyDialog();
}

/** \brief hide the helpful hint text after it has displayed once. */
void MythApps::hintTimerSlot() {
    m_hint->SetVisible(false);
    hintTimer->stop();
}

QMap<QString, QStringList> MythApps::filterQMap(QMap<QString, QStringList> map, QString filterKey) {
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

void MythApps::musicBarOnStopTimerSlot() {
    musicBarOnStopTimer->stop();
    m_textArtist->SetText("");
    m_textSong->SetText("");
    m_textAlbum->SetText("");
    showMusicPlayingBar(false);
    returnFocus();
}

/** \brief confirmation dialog.
 * \param description - text of the dialog
 * \param type - dialog type - text,password etc */
void MythApps::confirmDialog(QString description, QString type) {
    MythScreenStack *popupStack = GetMythMainWindow()->GetStack("popup stack");
    m_menuPopup = new MythDialogBox(description, popupStack, "mythappsmenupopup");

    if (m_menuPopup->Create()) {
        popupStack->AddScreen(m_menuPopup);
        m_menuPopup->SetReturnEvent(this, type);

        m_menuPopup->AddButton(tr("No"));
        m_menuPopup->AddButton(tr("Yes"));

        delay(6);
        if (m_menuPopup) {
            m_menuPopup->Close();
        }
    } else {
        delete m_menuPopup;
        m_menuPopup = nullptr;
    }
}

void MythApps::loadPlaylists() {
    LOG(VB_GENERAL, LOG_DEBUG, "loadPlaylists()");
    toggleSearchVisible(false);

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
        loadProgram(label, createProgramData("playlists", url, image, false, ""), image, uiCtx->fileListGrid);
    }
}

void MythApps::updateMusicPlaylistUI() {
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

void MythApps::playbackTimerSlot() {
    m_musicDuration->SetText(getPlayBackTimeString(false, true) + " / " + removeHoursIfZero(playbackDuration));
    m_trackProgress->SetUsed(getPlaybackPercentage());

    if (videoStopReceived)
        playbackTimer->stop();

    if (musicOpen)
        updateMusicPlaylistUI();
}

int MythApps::getPlaybackPercentage() const {
    QTime durationTime = QTime::fromString(playbackDuration, "hh:mm:ss");

    if (!durationTime.isValid()) {
        LOG(VB_GENERAL, LOG_DEBUG, "getPlaybackPercentage(): Invaild durationTime");
        return 0;
    }

    qint64 totalMs = QTime(0, 0).msecsTo(durationTime);
    int percent = static_cast<int>((static_cast<double>(getCurrentPlaybackTimeMs()) / totalMs) * 100.0);
    return qBound(0, percent, 100);
}

bool MythApps::handleMusicAction(const QString &action) {
    if (!musicOpen)
        return false;

    bool musicPlayerFullscreenOpen = false; // todo: not working

    if (action == "NEXTTRACK") {
        nextTrack();
    } else if (action == "PREVTRACK") {
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
    } else if ((action == "BACK" || action == "ESCAPE") and musicPlayerFullscreenOpen) {
        stopPlayBack();
    } else if ((action == "BACK" || action == "ESCAPE") and GetFocusWidget() == m_fileListMusicGrid) {
        m_filterGrid->SetItemCurrent(0);
        SetFocusWidget(m_filterGrid);
    } else if ((action == "BACK" || action == "ESCAPE") and GetFocusWidget() == m_filterGrid) {
        if (uiCtx->SearchTextEdit->IsVisible()) {
            SetFocusWidget(uiCtx->SearchTextEdit);
        } else {
            goBack();
        }

    } else if (action == "SEEKFFWD") {
        controls->seekFoward();
    } else if (action == "SEEKRWND") {
        controls->seekBack();
    } else if (action == "SELECT") {
        if (GetFocusWidget() == m_playingOn) {
            pauseToggle();
        } else if (GetFocusWidget() == m_ff_buttonOn) {
            controls->seekFoward();
        } else if (GetFocusWidget() == m_next_buttonOn) {
            nextTrack();
        }
    } else if (action == "UP" && GetFocusWidget() == uiCtx->SearchTextEdit) {
        SetFocusWidget(m_fileListMusicGrid);
    } else if ((action == "DOWN" || action == "RIGHT") && GetFocusWidget() == uiCtx->searchButtonList) {
        SetFocusWidget(m_fileListMusicGrid);
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
    } else {
        return false; // Not handled
    }
    return true; // Handled
}
