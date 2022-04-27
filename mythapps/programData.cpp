#include "programData.h"

ProgramData::ProgramData(QString setData) {
    QStringList paramsList = setData.split('~');
    filePathParam = paramsList.at(0); // filepath

    if (paramsList.size() > 1) { // plot or app name
        hasPlotText = true;
        plot = paramsList.at(1);
    } else {
        plot = "";
    }

    if (paramsList.size() > 2) { // image
        imageUrl = paramsList.at(2);
        plotandImageUrl = true;
    } else {
        imageUrl = "";
    }

    if (paramsList.size() > 3) { // play. Is it a video or directory. optional parameter
        play = paramsList.at(3);
    } else {
        play = ""; // directory
    }

    if (paramsList.size() > 4) { // seek time. optional parameter
        seek = paramsList.at(4);
    } else {
        seek = ""; // no seek
    }
}

/** \brief is the folder a video or a directory?
 * \return can the folder be played? */
bool ProgramData::isPlayRequest() {
    if (play.compare("play") == 0) {
        return true;
    }
    return false;
}

/** \brief get the plot
 * \return plot or sometimes app name */
QString ProgramData::getPlot() { return plot; }

/** \brief get the image url
 * \return image url*/
QString ProgramData::getImageUrl() { return imageUrl; }

/** \brief get the seek time. optional parameter
 * \return seek time */
QString ProgramData::getSeek() { return seek; }

/** \brief get the entire deliminated string
 * \return entire deliminated string */
QString ProgramData::getFilePathParam() { return filePathParam; }

/** \brief does a plot and image url exist?
 * \return if it exists */
bool ProgramData::hasPlotandImageUrl() { return plotandImageUrl; }

/** \brief does a seek value exist?
 * \return if it exists */
bool ProgramData::hasSeek() {
    if (seek.compare("") == 0) {
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
