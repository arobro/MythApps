#ifndef KODIMEDIASOURCE_H
#define KODIMEDIASOURCE_H

#include "IMediaSource.h"

// MythApps headers
#include "controls.h"
#include "musicContainer.h"

class NetRequest;

class KodiMediaSource : public IMediaSource {
  public:
    explicit KodiMediaSource(Controls *m_controls);
    ~KodiMediaSource() override;

    QMap<QString, MediaItem> getArtists(const QString &searchText = "") override;
    QMap<QString, MediaItem> getArtistsVids(const QString &searchText = "") override;

    QMap<QString, MediaItem> getAlbums(const QString &searchText = "") override;
    QMap<QString, MediaItem> getAlbumsVids(const QString &searchText = "") override;

    QMap<QString, MediaItem> getGenres(const QString &searchText = "") override;
    QMap<QString, MediaItem> getGenresVids(const QString &searchText = "") override;

    QMap<QString, MediaItem> searchSongs(const QString &query) override;
    QMap<QString, MediaItem> searchSongsVids(const QString &query) override;

    QMap<QString, MediaItem> getPlaylist() override;

    QStringList getPlaylistSongs(const QString &searchText = "") override;

  private:
    Controls *controls;
    QMap<QString, MediaItem> fetchMediaItems(const QString &rpcMethodName, const QString &responseKey, const QString &filterField, const QString &filterValue, const QString &filterOperator);
    QMap<QString, MediaItem> reindexByField(QMap<QString, MediaItem> inputMap, QString key);

    QStringList getSongsFromPlaylist(const QString &playlistPath);
    QString findPlaylistByName(const QString &searchText);
};

#endif // KODIMEDIASOURCE_H
