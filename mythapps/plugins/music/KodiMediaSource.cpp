#include "KodiMediaSource.h"

// QT headers
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QStandardPaths>

// MythTv headers
#include "libmythbase/mythlogging.h"

KodiMediaSource::KodiMediaSource(Controls *m_controls) : controls(m_controls) {}

KodiMediaSource::~KodiMediaSource() = default;
QMap<QString, MediaItem> KodiMediaSource::getArtists(const QString &searchText) {
    if (!searchText.isEmpty()) {
        return fetchMediaItems("AudioLibrary.GetArtists", "artists", "artist", searchText, "contains");
    } else {
        return fetchMediaItems("AudioLibrary.GetArtists", "artists", "", "", "is");
    }
}

QMap<QString, MediaItem> KodiMediaSource::getArtistsVids(const QString &searchText) {
    LOGS(0, "", "searchText", searchText);
    if (!searchText.isEmpty()) {
        return reindexByField(fetchMediaItems("VideoLibrary.GetMusicVideos", "musicvideos", "artist", searchText, "contains"), "artist");
    } else {
        return reindexByField(fetchMediaItems("VideoLibrary.GetMusicVideos", "musicvideos", "", "", "contains"), "artist");
    }
}

QMap<QString, MediaItem> KodiMediaSource::getAlbums(const QString &searchText) {
    LOGS(0, "", "searchText", searchText);
    QString filterKey = searchText.isEmpty() ? "" : "artist";
    return fetchMediaItems("AudioLibrary.GetAlbums", "albums", filterKey, searchText, "is");
}

QMap<QString, MediaItem> KodiMediaSource::getAlbumsVids(const QString &searchText) {
    LOGS(0, "", "searchText", searchText);
    QString filterKey = searchText.isEmpty() ? "" : "artist";
    return reindexByField(fetchMediaItems("VideoLibrary.GetMusicVideos", "musicvideos", filterKey, searchText, "is"), "album");
}

QMap<QString, MediaItem> KodiMediaSource::getGenres(const QString &searchText) {
    LOGS(0, "", "searchText", searchText);
    if (searchText.isEmpty()) {
        return fetchMediaItems("AudioLibrary.GetGenres", "genres", "", "", "is");
    }

    return fetchMediaItems("AudioLibrary.GetAlbums", "albums", "genre", searchText, "is");
}

QMap<QString, MediaItem> KodiMediaSource::getGenresVids(const QString &searchText) {
    LOGS(0, "", "searchText", searchText);
    if (searchText.isEmpty()) {
        return reindexByField(fetchMediaItems("VideoLibrary.GetMusicVideos", "musicvideos", "", "", ""), "genre");
    }

    return reindexByField(fetchMediaItems("VideoLibrary.GetMusicVideos", "musicvideos", "genre", searchText, "is"), "album");
}

QMap<QString, MediaItem> KodiMediaSource::searchSongs(const QString &query) { return fetchMediaItems("AudioLibrary.GetSongs", "songs", "title", query, "contains"); }

QMap<QString, MediaItem> KodiMediaSource::searchSongsVids(const QString &query) {
    return reindexByField(fetchMediaItems("VideoLibrary.GetMusicVideos", "musicvideos", "title", query, "contains"), "title");
}

QMap<QString, MediaItem> KodiMediaSource::getPlaylist() {
    LOGS(1, "");
    QMap<QString, MediaItem> playlistMap;

    QJsonObject params;
    params["directory"] = "special://musicplaylists";

    QJsonValue answer = controls->callJsonRpc("Files.GetDirectory", params);
    QJsonArray files = answer.toObject().value("files").toArray();

    for (const QJsonValue &v : files) {
        QJsonObject obj = v.toObject();
        QString label = obj.value("label").toString();
        if (label.isEmpty())
            continue;

        QString thumbnail = obj.value("thumbnail").toString();
        QString file = obj.value("file").toString();

        MediaItem item;
        item.thumbnail = thumbnail;
        item.file = file;
        item.album = "";
        item.artist = "";
        item.genre = "";

        playlistMap.insert(label, item);
    }

    return playlistMap;
}

QStringList KodiMediaSource::getPlaylistSongs(const QString &searchText) {
    LOGS(0, "", "searchText", searchText);
    return getSongsFromPlaylist(findPlaylistByName(searchText));
}

/** @brief Fetches media items
 * @param rpcMethodName     The name of the Kodi JSON-RPC method to call.
 * @param responseKey       The key in the JSON response containing the media array.
 * @param filterField       The field to filter on (e.g., "artist", "genre").
 * @param filterValue       The value to match against the filter field.
 * @param filterOperator    The comparison operator (e.g., "contains", "is").
 * @return QMap<QString, MediaItem> A map of media labels to MediaItem objects.
 */
QMap<QString, MediaItem> KodiMediaSource::fetchMediaItems(const QString &rpcMethodName, const QString &responseKey, const QString &filterField, const QString &filterValue,
                                                          const QString &filterOperator) {
    LOGS(0, "", "rpcMethodName", rpcMethodName, "responseKey", responseKey, "filterField", filterField, "filterValue", filterValue, "filterOperator", filterOperator);

    QMap<QString, MediaItem> mediaMap;
    QJsonArray requestedProperties;
    requestedProperties.push_back("thumbnail");

    QJsonObject rpcParams;

    if (rpcMethodName == "AudioLibrary.GetSongs") {
        requestedProperties.push_back("file");
    } else if (rpcMethodName == "VideoLibrary.GetMusicVideos") {
        requestedProperties.push_back("album");
        requestedProperties.push_back("artist");
        requestedProperties.push_back("genre");
        requestedProperties.push_back("file");
    }

    if (!filterField.isEmpty()) {
        QJsonObject filterObject;
        filterObject["field"] = filterField;
        filterObject["operator"] = filterOperator;
        filterObject["value"] = filterValue;
        rpcParams["filter"] = filterObject;
    }

    QJsonValue rpcResponse = controls->callJsonRpc(rpcMethodName, rpcParams, requestedProperties);
    QJsonObject responseObject = rpcResponse.toObject();
    QJsonArray mediaItems = responseObject.value(responseKey).toArray();

    for (const QJsonValue &itemValue : mediaItems) {
        QJsonObject itemObject = itemValue.toObject();
        QString label = itemObject.value("label").toString();
        if (label.isEmpty())
            continue;

        MediaItem media;
        media.thumbnail = itemObject.value("thumbnail").toString();

        if (rpcMethodName == "AudioLibrary.GetSongs") {
            media.file = itemObject.value("file").toString();
        } else if (rpcMethodName == "VideoLibrary.GetMusicVideos") {
            media.file = itemObject.value("file").toString();
            media.album = itemObject.value("album").toString();

            QJsonArray artistArray = itemObject.value("artist").toArray();
            if (!artistArray.isEmpty())
                media.artist = artistArray.at(0).toString();

            QJsonArray genreArray = itemObject.value("genre").toArray();
            if (!genreArray.isEmpty())
                media.genre = genreArray.at(0).toString();
        } else if (rpcMethodName == "AudioLibrary.GetAlbums") {
            media.album = QString::number(itemObject.value("albumid").toInt());
        }

        mediaMap.insert(label, media);
    }

    return mediaMap;
}

/** \brief Reindex a map of MediaItem objects using a specified field as the new key. */
QMap<QString, MediaItem> KodiMediaSource::reindexByField(QMap<QString, MediaItem> map, QString filterKey) {
    LOGS(0, "");
    QMap<QString, MediaItem> list;

    QMapIterator<QString, MediaItem> i(map);
    while (i.hasNext()) {
        i.next();
        QString label = i.key();
        MediaItem item = i.value();

        QString key;
        if (filterKey.compare("artist") == 0) {
            key = item.artist;
        } else if (filterKey.compare("album") == 0) {
            key = item.album;
        } else if (filterKey.compare("genre") == 0) {
            key = item.genre;
        } else {
            key = label;
        }

        list.insert(key, item);
    }
    return list;
}

QStringList KodiMediaSource::getSongsFromPlaylist(const QString &playlistPath) {
    LOGS(0, "", "playlistPath", playlistPath);

    QStringList songs;
    QFile file(playlistPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return songs;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.startsWith("#") && !line.isEmpty()) {
            songs.append(line);
        }
    }

    file.close();
    return songs;
}

QString KodiMediaSource::findPlaylistByName(const QString &searchText) {
    LOGS(0, "", "searchText", searchText);
#ifdef __ANDROID__
    return QString();
#endif

    QStringList possiblePaths;

    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    possiblePaths << homePath + "/.var/app/tv.kodi.Kodi/data/userdata/playlists/";
    possiblePaths << homePath + "/.kodi/userdata/playlists/";

    for (const QString &path : possiblePaths) {
        QDir dir(path);

        if (!dir.exists())
            continue;

        QDirIterator it(path, {"*.m3u", "*.m3u8"}, QDir::Files | QDir::Readable, QDirIterator::Subdirectories);

        while (it.hasNext()) {
            QString filePath = it.next();
            QFileInfo fileInfo(filePath);

            if (fileInfo.fileName().compare(searchText, Qt::CaseInsensitive) == 0) {
                return fileInfo.absoluteFilePath();
            }
        }
    }

    return QString();
}
