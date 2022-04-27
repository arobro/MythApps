#ifndef ProgramData_h
#define ProgramData_h

#include <QString>
#include <QStringList>

/** \class ProgramData
 *  \brief makes the deliminated data format attached to the button clicked/hover events easy to read */

class ProgramData {
  public:
    ProgramData(QString setData);

    QString createData(QString file, QString plot, QString thumbnail, bool play, QString seek);

    bool isPlayRequest();
    QString getPlot();
    QString getImageUrl();
    bool hasPlotandImageUrl();
    bool hasSeek();
    QString getSeek();
    bool hasPlotText = false;
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

  private:
    QString filePathParam;
    QString plot;
    QString imageUrl;
    QString play;
    QString seek;
    bool plotandImageUrl = false;
};
#endif
