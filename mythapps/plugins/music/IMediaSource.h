#ifndef IMEDIASOURCE_H
#define IMEDIASOURCE_H

#include <QMap>
#include <QString>
#include <QStringList>

#include "musicContainer.h"

class IMediaSource {
  public:
    virtual ~IMediaSource() {}

    virtual QMap<QString, MediaItem> getArtists(const QString &) { return {}; }

    virtual QMap<QString, MediaItem> getArtistsVids(const QString &) { return {}; }

    virtual QMap<QString, MediaItem> getAlbums(const QString &) { return {}; }

    virtual QMap<QString, MediaItem> getAlbumsVids(const QString &) { return {}; }

    virtual QMap<QString, MediaItem> getGenres(const QString &) { return {}; }

    virtual QMap<QString, MediaItem> getGenresVids(const QString &) { return {}; }

    virtual QMap<QString, MediaItem> searchSongs(const QString &) { return {}; }

    virtual QMap<QString, MediaItem> searchSongsVids(const QString &) { return {}; }

    virtual QMap<QString, MediaItem> getPlaylist() { return {}; }

    virtual QStringList getPlaylistSongs(const QString &) { return {}; }
};

#endif // IMEDIASOURCE_H
