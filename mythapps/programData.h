#ifndef ProgramData_h
#define ProgramData_h

#include <QDebug>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "container.h"

/** \class ProgramData
 *  \brief makes the deliminated data format attached to the button clicked/hover events easy to read */

class ProgramData {
  public:
    ProgramData(QString label, QString setData);

    QString createData(QString file, QString plot, QString thumbnail, bool play, QString seek);

    bool isPlayRequest();
    QString getPlot();
    QString getImageUrl();
    bool hasPlotandImageUrl();
    bool hasSeek();
    QString getSeek();
    bool hasPlot();
    QString getFilePathParam();

    bool hasBack();
    bool hasArtists();
    bool haAlbums();
    bool hasGenres();
    bool hasPlaylists();

    bool hasWeb();
    bool hasFavourites();
    bool hasShowsAZ();
    bool hasSearchShowsAZ();
    bool hasMusic();
    bool hasWatchedList();
    bool hasUnwatchedList();
    bool hasVideos();

    bool hasYTnative();
    QString getWebPage();

    void set(QString label, QString data);
    bool isEmpty();
    FileFolderContainer get();
    QString getUrl();
    void resetSeek();
    void setSeek(QString seek);
    void setUnWatched();

    bool isYTWrappedApp();
    void setFirstDirectory(bool m_FirstDirectory);
    QString getAppName(QString currentFirstDirectoryName);

  private:
    bool plotandImageUrl = false;
    bool hasPlotText = false;
    bool firstDirectory = false;

    FileFolderContainer fileFolderContainer;
};
#endif
