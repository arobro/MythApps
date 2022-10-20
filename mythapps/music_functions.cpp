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
    m_fileListGrid->SetVisible(!show);
    m_fileListGrid->SetEnabled(!show);
    m_musicTitle->SetVisible(show);
    m_title->SetVisible(!show);
    m_plot->SetVisible(!show);
    m_filterOptionsList->SetVisible(show);
    m_playlistVertical->SetVisible(show);
    m_playlistVertical->SetEnabled(true);

    m_androidMenuBtn->SetVisible(false);
    m_androidMenuBtn->SetEnabled(false);
}

void MythApps::previousTrack() { controls->inputActionHelper("skipprevious"); }

void MythApps::nextTrack() { controls->inputActionHelper("skipnext"); }

/** \brief update the music playing bar labels. song playing etc */
void MythApps::updateMusicPlayingBarStatus() {
    LOG(VB_GENERAL, LOG_DEBUG, "updateMusicPlayingBarStatus()");
    globalActivePlayer = controls->getActivePlayer();
    QString answer = controls->playerGetItem(globalActivePlayer);

    QJsonDocument jsonDocument = QJsonDocument::fromJson(answer.toLocal8Bit().data());
    QJsonObject jObject = jsonDocument.object();
    QVariantMap mainMap = jObject.toVariantMap();

    QVariantMap map2 = mainMap["result"].toMap();
    QVariantMap map3 = map2["item"].toMap();

    QString thumbnail = map3["thumbnail"].toString();

    QList listArtist = map3["artist"].toList();
    if (listArtist.size() > 0) {
        m_textArtist->SetText(listArtist.at(0).toString());
    }

    QString label = map3["label"].toString();
    m_textSong->SetText(label);
    m_textAlbum->SetText(map3["album"].toString());

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
    image.loadFromData(nr->downloadImage(urlencode(thumbnailPath), false));
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
    SetFocusWidget(m_fileListGrid);
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
    if (m_loaderImage->IsVisible()) { // button debounce
        return;
    }

    m_loaderImage->SetVisible(true);

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

    QStringList threadInfo = item->GetData().toStringList();

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
        if (threadInfo.size() < 4) {
            LOG(VB_GENERAL, LOG_ERR, "error - addToPlaylistClickedCallback");

        } else {
            QStringList paramsList = threadInfo.at(4).split('~');
            controls->playListAdd(friendlyUrl(paramsList.at(0)));
        }

        if (!isPlaying(0)) {
            controls->playListOpen(1);
        }
        refreshGuiPlaylist();
    } else { // already in playlist
        controls->playListOpen(inPlaylist);
    }
    m_loaderImage->SetVisible(false);
}

/** \brief refresh/update the playlist. */
void MythApps::refreshGuiPlaylist() {
    LOG(VB_GENERAL, LOG_DEBUG, "refreshGuiPlaylist()");
    delayMilli(100);

    QJsonObject obj;
    obj["id"] = "1";
    obj["jsonrpc"] = "2.0";
    obj["method"] = "Playlist.GetItems";
    QJsonObject obj2;
    obj2["playlistid"] = globalActivePlayer;

    obj["params"] = obj2;

    QString s = controls->requestUrl(obj);

    QJsonDocument jsonDocument = QJsonDocument::fromJson(s.toLocal8Bit().data());
    QJsonObject jObject = jsonDocument.object();
    QVariantMap mainMap = jObject.toVariantMap();

    QVariantMap map2 = mainMap["result"].toMap();
    QList list3 = map2["items"].toList();

    m_playlistVertical->Reset();
    int pos = 0;
    foreach (QVariant T, list3) {
        pos++;
        QVariantMap map4 = T.toMap();
        new MythUIButtonListItem(m_playlistVertical, map4["label"].toString());
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
    QJsonObject obj;
    obj["jsonrpc"] = "2.0";
    obj["method"] = methodValue;

    QJsonArray array;
    array.push_back("thumbnail");

    if (methodValue.compare("AudioLibrary.GetSongs") == 0) {
        array.push_back("file");
    } else if (methodValue.compare("VideoLibrary.GetMusicVideos") == 0) {
        array.push_back("album");
        array.push_back("artist");
        array.push_back("genre");
        array.push_back("file");
    }

    QJsonObject obj2;
    obj2["properties"] = array;

    if (!filterKey.compare("") == 0) {
        QJsonObject obj3;
        obj3["field"] = filterKey;
        obj3["operator"] = operatorValue;
        obj3["value"] = filterValue;

        obj2["filter"] = obj3;
    }

    obj["params"] = obj2;
    obj["id"] = "1";

    QString answer = controls->requestUrl(obj);

    QByteArray br = answer.toUtf8();
    QJsonDocument doc = QJsonDocument::fromJson(br);
    QJsonObject addons = doc.object();
    QJsonObject addons2 = addons["result"].toObject();
    QJsonValue addons3 = addons2[type];

    QJsonArray array2 = addons3.toArray();
    foreach (const QJsonValue &v, array2) {
        QString label = v.toObject().value("label").toString();
        QString image = v.toObject().value("thumbnail").toString();
        QString file = "";
        QString album = "";
        QString artist = "";
        QString genre = "";

        if (methodValue.compare("AudioLibrary.GetSongs") == 0) {
            file = v.toObject().value("file").toString();
        } else if (methodValue.compare("VideoLibrary.GetMusicVideos") == 0) {
            file = v.toObject().value("file").toString();
            album = v.toObject().value("album").toString();
            artist = v.toObject().value("artist").toArray().takeAt(0).toString();
            genre = v.toObject().value("genre").toArray().takeAt(0).toString();
        }

        QStringList stringList;
        stringList.append(image);
        stringList.append(file);
        stringList.append(album);
        stringList.append(artist);
        stringList.append(genre);

        artistList.insert(label, stringList);
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
    m_fileListGrid->Reset();
    loadBackButton();
    toggleSearchVisible(false);

    QStringListIterator list = addSpacingToList(getByArtist(""), listview);
    while (list.hasNext()) {
        QStringList split = list.next().split(",");
        QString label = split.at(0);
        QString image = split.at(1);
        loadProgram(label, createProgramData("artists~", "", image, false, ""), image, false);
    }
}

/** \brief display music by album */
void MythApps::loadAlbums(bool listview) {
    LOG(VB_GENERAL, LOG_DEBUG, "loadAlbums()");
    showMusicUI(false);
    m_fileListGrid->Reset();
    loadBackButton();
    toggleSearchVisible(false);

    QStringListIterator list = addSpacingToList(getByAlbums(""), listview);
    while (list.hasNext()) {
        QStringList split = list.next().split(",");
        QString label = split.at(0);
        QString image = split.at(1);
        loadProgram(label, createProgramData("albums", "", image, false, ""), image, false);
    }
}
/** \brief display music by genre */
void MythApps::loadGenres() {
    LOG(VB_GENERAL, LOG_DEBUG, "loadGenres()");
    showMusicUI(false);
    m_fileListGrid->Reset();
    loadBackButton();
    toggleSearchVisible(false);

    if (musicMode == 2 || musicMode == 3) {
        loadMusicHelper("[Video,Genre] ", filterQMap(getMusicHelper("VideoLibrary.GetMusicVideos", "musicvideos", "", "", "contains"), "genre"), m_fileListGrid);
    }
    if (!(musicMode == 1 || musicMode == 3)) {
        return;
    }

    loadMusicHelper("", getByGenres(), m_fileListGrid);
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

        loadProgram(labelPrefix + label, setData, image, false, m_fileList);

        if (count % 3 == 0 and m_fileListMusicGrid == m_fileList) { // load an invisible colum to help with navigation
            loadProgram("BlankR", "", "", false, m_fileListMusicGrid);
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

    album = album.replace("[Video,Artist] ", "");
    album = album.replace("[Video,Album] ", "");
    album = album.replace("[Video,Genre] ", "");

    if (musicMode == 2) {
        loadMusicHelper("", filterQMap(getMusicHelper("VideoLibrary.GetMusicVideos", "musicvideos", "album", album, "is"), ""), m_fileListSongs);
        return;
    }

    QJsonObject obj;
    obj["jsonrpc"] = "2.0";
    obj["method"] = "AudioLibrary.GetSongs";

    QJsonObject obj3;
    obj3["order"] = "ascending";
    obj3["method"] = "artist";

    QJsonArray array;
    array.push_back("artist");
    array.push_back("thumbnail");
    array.push_back("file");
    array.push_back("album");

    QJsonObject obj2;
    obj2["properties"] = array;

    obj2["sort"] = obj3;

    if (!album.compare("") == 0) {
        QJsonObject obj4;
        obj4["album"] = album;
        obj2["filter"] = obj4;
    }

    obj["params"] = obj2;
    obj["id"] = "1";

    QString answer = controls->requestUrl(obj);

    QByteArray br = answer.toUtf8();
    QJsonDocument doc = QJsonDocument::fromJson(br);
    QJsonObject addons = doc.object();
    QJsonObject addons2 = addons["result"].toObject();
    QJsonValue addons3 = addons2["songs"];

    int count = 0;

    QJsonArray array2 = addons3.toArray();
    foreach (const QJsonValue &v, array2) {
        QString label = v.toObject().value("label").toString();
        QString image = v.toObject().value("thumbnail").toString();
        QString artist = v.toObject().value("artist").toArray().takeAt(0).toString();
        QString url = v.toObject().value("file").toString();

        count++;
        QString number = QString::number(count) + "   ";
        if (count < 10) {
            number = number + QString(" ");
        }

        loadProgram(number + label, url + "~" + "" + "|" + image, image, false, m_fileListSongs);
    }
}

void MythApps::loadMusicSetup() {
    m_fileListMusicGrid->Reset();
    m_filterGrid->Reset();
    m_filterOptionsList->Reset();
    m_playlistVertical->Reset();

    auto *item0 = new MythUIButtonListItem(m_filterGrid, "");
    item0->SetImage(mm_alltracks_filename.replace("file://", ""));
    item0->SetData("alltracks");
    auto *item1 = new MythUIButtonListItem(m_filterGrid, "");
    item1->SetImage(mm_artists_filename.replace("file://", ""));
    item1->SetData("artists");
    auto *item2 = new MythUIButtonListItem(m_filterGrid, "");
    item2->SetImage(mm_albums_filename.replace("file://", ""));
    item2->SetData("albums");
    auto *item3 = new MythUIButtonListItem(m_filterGrid, "");
    item3->SetImage(mm_genres_filename.replace("file://", ""));
    item3->SetData("genres");

    auto *itemI = new MythUIButtonListItem(m_filterGrid, "");
    itemI->SetData("BlankR");

    auto *item4 = new MythUIButtonListItem(m_filterGrid, "");
    item4->SetImage(mm_artists_filename.replace("file://", ""));
    item4->SetData("artists2");
    auto *item5 = new MythUIButtonListItem(m_filterGrid, "");
    item5->SetImage(mm_albums_filename.replace("file://", ""));
    item5->SetData("albums2");

    auto *item6 = new MythUIButtonListItem(m_filterGrid, "");
    item6->SetImage(mm_playlist_filename.replace("file://", ""));
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
    globalActivePlayer = 1;
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

        loadProgram(label, createProgramData("", "", image, false, ""), image, false, m_fileListMusicGrid);
        if (count % 3 == 0) { // load an invisible colum to help with navigation
            loadProgram("BlankR", "", "", false, m_fileListMusicGrid);
        }
    }
    SetFocusWidget(m_fileListMusicGrid);

    updateMusicPlayingBarStatus();
}

void MythApps::loadMusic() {
    musicOpen = true;
    controls->setAudioLibraryScan();
    loadSongsMain("", "artists");
    exitToMainMenuSleepTimer->stop();
}

void MythApps::clearAndStopPlaylist() {
    createBusyDialog(tr("Stopping Music..."));
    int playerID = controls->stopPlayBack();
    controls->playListClear(playerID);
    controls->playListClear(1);
    delayMilli(100);
    closeBusyDialog();
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

    QString answer = controls->requestUrl(controls->getDirectoryObject("special://musicplaylists"));
    QByteArray br = answer.toUtf8();
    QJsonDocument doc = QJsonDocument::fromJson(br);
    QJsonObject addons = doc.object();
    QJsonObject addons2 = addons["result"].toObject();

    QJsonArray array = addons2["files"].toArray();
    foreach (const QJsonValue &v, array) {
        QString label = v.toObject().value("label").toString();
        QString image = v.toObject().value("thumbnail").toString();
        QString url = v.toObject().value("file").toString();
        loadProgram(label, createProgramData("playlists", url, image, false, ""), image, false, m_fileListGrid);
    }
}

void MythApps::playbackTimerSlot() {
    static int count = 0;
    static int prevPlayBackTimeMap;

    if (count % 2 == 0) {
        playBackTimeMap = getPlayBackTime(globalActivePlayer);
        prevPlayBackTimeMap = playBackTimeMap["seconds"].toInt();
    } else {
        playBackTimeMap["seconds"] = prevPlayBackTimeMap + 1;
    }

    m_musicDuration->SetText(getPlayBackTimeString().replace("00:0", "") + " / " + controls->getGlobalDuration());

    count++;
    if (count == 4) {
        handleDialogs(false);
        count = 0;
    }

    if (musicOpen) { // refresh the playlist if the playlist is empty and content is playing
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
}
