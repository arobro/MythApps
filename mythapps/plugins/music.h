#ifndef MUSIC_H
#define MUSIC_H

#include "plugin_api.h"

// QT headers
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMap>
#include <QStringList>
#include <QVariantMap>

// MythApps headers
#include "controls.h"

// Music
#include "music/IMediaSource.h"
#include "music/KodiMediaSource.h"

class Music : public PluginAPI {
    Q_OBJECT
  public:
    Music();
    ~Music() override;

    QString getPluginName() const override;
    QString getPluginDisplayName() override;
    bool getPluginStartPos() const override;
    QString getPluginIcon() const override;
    bool useBasicMenu() override;
    void search(const QString &searchText) override;

    void load(const QString label, const QString data) override;
    void displayHomeScreenItems() override;

    bool onTextMessageReceived(const QString &method, const QString &message) override;
    void exitPlugin() override;
    QStringList getOptionsMenuItems(ProgramData *, const QString &, bool appIsOpen) override;
    bool menuCallback(const QString &menuText, ProgramData *currentSelectionDetails) override;

    bool handleAction(const QString, MythUIType *, ProgramData *) override;

  private:
    void loadMusic();
    void updateMediaListCallback(const QString &label, const QString &data);

    QMap<QString, MediaItem> getByArtist(QString artist);
    QMap<QString, MediaItem> getByAlbums(QString artist);
    QMap<QString, MediaItem> getByAlbumsSearch(QString search);
    QMap<QString, MediaItem> getByGenres(QString searchText);
    QMap<QString, MediaItem> getByPlaylist();

    void loadCategory(const QMap<QString, MediaItem> &items, const QString &subPath, bool listview);

    void loadArtists(bool listview);
    void loadAlbums(bool listview);
    void loadGenres();
    void loadSongs(QString album);
    void loadPlaylists();
    void loadMusicHelper(QString labelPrefix, QMap<QString, QStringList> loadMusicType, MythUIButtonList *m_fileList);

    void loadSongsMain(QString value, QString type);
    void updateMusicPlayingBarStatus();
    void downloadImage(QString thumbnailPath);

    void setPartyMode();
    void previousTrack();
    void nextTrack();

    QStringList addSpacingToList(QMap<QString, QStringList> inputMap, bool listview);
    QString createProgramData(QString category, QString extra, QString image, bool flag, QString info);
    void loadProgram(QString label, QString data, QString image, MythUIButtonList *list);

    void refreshGuiPlaylist();
    void clearAndStopPlaylist();
    int isInPlaylist(QString label);
    void removeFromPlaylist(QString label);
    void updateMusicPlaylistUI();

    QString pluginName;
    QString pluginIcon;

    QString music_icon;

    void showMusicUI(bool show);
    bool initializeUI(MythUIType *ui);
    bool partyMode;

    MythImage *albumArtImage{nullptr};

    MythUIType *m_musicDetailsUIGroup{nullptr};
    MythUIText *m_textSong{nullptr};
    MythUIText *m_textArtist{nullptr};
    MythUIText *m_textAlbum{nullptr};
    MythUIText *m_musicDuration{nullptr};
    MythUIText *m_hint; /*!< hint used in music app */
    MythUIShape *m_seekbar{nullptr};
    MythUIText *m_musicTitle{nullptr};
    MythUIProgressBar *m_trackProgress{nullptr};
    MythUIImage *m_coverart{nullptr};
    MythUIImage *m_blackhole_border{nullptr};
    MythUIImage *m_playingOn{nullptr};
    MythUIImage *m_next_buttonOn{nullptr};
    MythUIImage *m_ff_buttonOn{nullptr};
    MythUIImage *m_playingOff{nullptr};
    MythUIImage *m_next_buttonOff{nullptr};
    MythUIImage *m_ff_buttonOff{nullptr};

    MythUIButtonList *m_fileListMusicGrid{nullptr};
    MythUIButtonList *m_songList{nullptr};
    MythUIButtonList *m_filterGrid{nullptr};
    MythUIButtonList *m_filterOptionsList{nullptr};
    MythUIButtonList *m_playlist{nullptr};

    int musicMode; /*!< 1 for music, 2 for video in the music app */

    QString mm_albums_icon;    /*!< stores physical image location for the corresponding button */
    QString mm_alltracks_icon; /*!< stores physical image location for the corresponding button */
    QString mm_artists_icon;   /*!< stores physical image location for the corresponding button */
    QString mm_genres_icon;    /*!< stores physical image location for the corresponding button */
    QString mm_playlist_icon;  /*!< stores physical image location for the corresponding button */

    int m_currentMusicButton = 0;

    void showMusicPlayingBar(bool show);
    void loadMusicSetup();

    QTimer *hintTimer;     /*!< stops any hung searchs */
    QTimer *playbackTimer; /*!< update the playback status in the music app */

    QString username; /*!< username for Kodi */
    QString password; /*!< password for Kodi */
    QString ip;       /*!< ip for Kodi */
    QString port;     /*!< port for Kodi */

    QScopedPointer<IMediaSource> mediaSource;

  private slots:
    void fileListMusicGridClickedCallback(MythUIButtonListItem *item);
    void fileListMusicGridSelectedCallback(MythUIButtonListItem *item);
    void fileListMusicGridVisibleCallback(MythUIButtonListItem *item);
    void filterGridClickedCallback(MythUIButtonListItem *item);
    void filterGridSelectedCallback(MythUIButtonListItem *item);
    void filterOptionsListClickedCallback(MythUIButtonListItem *item);

    void addToPlaylistClickedCallback(MythUIButtonListItem *item);

    void hintTimerSlot();
    void playbackTimerSlot();
};

#endif // MUSIC_H
