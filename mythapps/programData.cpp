#include "programData.h"

ProgramData::ProgramData(QString setData) { set("", setData); }

/** \brief is the folder a video or a directory?
 * \return can the folder be played? */
bool ProgramData::isPlayRequest() { return fileFolderContainer.autoPlay; }

/** \brief get the plot
 * \return plot or sometimes app name */
QString ProgramData::getPlot() { return fileFolderContainer.plot; }

/** \brief get the image url
 * \return image url*/
QString ProgramData::getImageUrl() { return fileFolderContainer.image; }

/** \brief get the seek time. optional parameter
 * \return seek time */
QString ProgramData::getSeek() { return fileFolderContainer.seek; }

/** \brief get the entire deliminated string
 * \return entire deliminated string */
QString ProgramData::getFilePathParam() { return fileFolderContainer.url; }

/** \brief does a plot and image url exist?
 * \return if it exists */
bool ProgramData::hasPlotandImageUrl() { return plotandImageUrl; }

/** \brief does a seek value exist?
 * \return if it exists */
bool ProgramData::hasSeek() {
    if (fileFolderContainer.seek.compare("") == 0) {
        return false;
    }
    return true;
}

/** \brief does a plot value exist?
 * \return if it exists */
bool ProgramData::hasPlot() { return hasPlotText; }

/** \brief is the folder an app called Back?
 * \return is the corresponding folder type? */
bool ProgramData::hasBack() {
    if (getFilePathParam().compare(QString("Back")) == 0) {
        return true;
    }
    return false;
}

/** \brief is the folder an app called artists?
 * \return is the corresponding folder type? */
bool ProgramData::hasArtists() {
    if (getFilePathParam().compare(QString("artists")) == 0) {
        return true;
    }
    return false;
}

/** \brief is the folder an app called albums?
 * \return is the corresponding folder type? */
bool ProgramData::haAlbums() {
    if (getFilePathParam().compare(QString("albums")) == 0) {
        return true;
    }
    return false;
}

/** \brief is the folder an app called genres?
 * \return is the corresponding folder type? */
bool ProgramData::hasGenres() {
    if (getFilePathParam().compare(QString("genres")) == 0) {
        return true;
    }
    return false;
}

/** \brief is the folder an app called playlists?
 * \return is the corresponding folder type? */
bool ProgramData::hasPlaylists() {
    if (getFilePathParam().compare(QString("playlists")) == 0) {
        return true;
    }
    return false;
}

/** \brief is the folder an app called web?
 * \return is the corresponding folder type? */
bool ProgramData::hasWeb() {
    if (getPlot().compare(QString("web")) == 0) {
        return true;
    }
    return false;
}

/** \brief is the folder an app called Favourites?
 * \return is the corresponding folder type? */
bool ProgramData::hasFavourites() {
    if (getPlot().compare(QString("Favourites")) == 0) {
        return true;
    }
    return false;
}

/** \brief is the folder an app called Shows A-Z"?
 * \return is the corresponding folder type? */
bool ProgramData::hasShowsAZ() {
    if (getPlot().compare(QString("Shows A-Z")) == 0) {
        return true;
    }
    return false;
}

/** \brief is the folder an app called searchShowsAZ?
 * \return is the corresponding folder type? */
bool ProgramData::hasSearchShowsAZ() {
    if (getPlot().compare(QString("searchShowsAZ")) == 0) {
        return true;
    }
    return false;
}

/** \brief is the folder an app called Music?
 * \return is the corresponding folder type? */
bool ProgramData::hasMusic() {
    if (getPlot().compare(QString("Music")) == 0) {
        return true;
    }
    return false;
}

/** \brief is the folder an app called Watched List?
 * \return is the corresponding folder type? */
bool ProgramData::hasWatchedList() {
    if (getPlot().compare(QString("Watched List")) == 0) {
        return true;
    }
    return false;
}

/** \brief is the folder an app called Unwatched?
 * \return is the corresponding folder type? */
bool ProgramData::hasUnwatchedList() {
    if (getPlot().compare(QString("Unwatched")) == 0) {
        return true;
    }
    return false;
}

/** \brief is the folder an app called Back?
 * \return is the corresponding folder Videos? */
bool ProgramData::hasVideos() {
    if (getPlot().compare(QString("Videos")) == 0) {
        return true;
    }
    return false;
}

/** \brief set the program data
 * 	\param label title of the program data
 * 	\param data all program data  */
void ProgramData::set(QString label, QString data) {
    fileFolderContainer.url = "";
    fileFolderContainer.plot = "";
    fileFolderContainer.image = "";
    fileFolderContainer.autoPlay = false;
    fileFolderContainer.seek = "";

    fileFolderContainer.title = label;

    QStringList paramsList = data.split('~');
    fileFolderContainer.url = paramsList.at(0); // filepath

    if (paramsList.size() > 1) { // plot or app name
        fileFolderContainer.plot = paramsList.at(1);
        hasPlotText = true;
    }

    if (paramsList.size() > 2) { // image
        fileFolderContainer.image = paramsList.at(2);
        plotandImageUrl = true;
    }

    if (paramsList.size() > 3) {
        if (paramsList.at(3).compare("play") == 0) {
            fileFolderContainer.autoPlay = true;
        }
    }

    if (paramsList.size() > 4) { // seek time. optional parameter
        fileFolderContainer.seek = paramsList.at(4);
    }
}

/** \brief is the program data empty?
 * \return is the program data (url) empty? */
bool ProgramData::isEmpty() { return fileFolderContainer.url.isEmpty(); }

/** \brief get all the program data
 * \return FileFolderContainer of the program data */
FileFolderContainer ProgramData::get() { return fileFolderContainer; }

/** \brief get the program data url
 * \return url */
QString ProgramData::getUrl() { return fileFolderContainer.url; }

/** \brief reset the seek amount to zero*/
void ProgramData::resetSeek() { fileFolderContainer.seek = ""; }

/** \brief set the seek amount*/
void ProgramData::setSeek(QString seek) { fileFolderContainer.seek = seek; }

/** \brief set the un watched flag in program data*/
void ProgramData::setUnWatched() { fileFolderContainer.seek = "false"; }
