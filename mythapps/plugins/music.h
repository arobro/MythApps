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

    void onTextMessageReceived(const QString &method, const QString &message) override;
    void exitPlugin() override;

    bool handleAction(const QString &, MythUIType *focusWidget);

  private:
    void loadMusic();
    void updateMediaListCallback(const QString &label, const QString &data);

    QMap<QString, QStringList> getMusicHelper(QString methodValue, QString type, QString filterKey, QString filterValue, QString operatorValue);
    QMap<QString, QStringList> getByArtist(QString artist);
    QMap<QString, QStringList> getByAlbums(QString artist);
    QMap<QString, QStringList> getByAlbumsSearch(QString search);
    QMap<QString, QStringList> getByAlbumsWithGenre(QString genres);
    QMap<QString, QStringList> getByGenres();

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

    QMap<QString, QStringList> filterQMap(QMap<QString, QStringList> inputMap, QString key);
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
    MythUIButtonList *m_fileListSongs{nullptr};
    MythUIButtonList *m_filterGrid{nullptr};
    MythUIButtonList *m_filterOptionsList{nullptr};
    MythUIButtonList *m_playlistVertical{nullptr};

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
    
    QString username;              /*!< username for Kodi */
    QString password;              /*!< password for Kodi */
    QString ip;                    /*!< ip for Kodi */
    QString port;                  /*!< port for Kodi */

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
