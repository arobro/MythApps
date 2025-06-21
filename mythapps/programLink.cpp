#include "programLink.h"

// MythTV headers
#include <libmyth/mythcontext.h>

// MythApps headers
#include "shared.h"

/** \class ProgramLink
 *  \brief Find, Add and remove program links such as favourites, watchlist, previously played & search List */

ProgramLink::ProgramLink(QString _linkName, bool _remote, bool _mostRecentOnly) {
    linkName = _linkName;
    remote = _remote;
    mostRecentOnly = _mostRecentOnly;
}

/** \brief How many favourites/'program links' are there?
 *  \return number of program links */
QString ProgramLink::getListSize() {
    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare("SELECT id FROM mythapps_programlink  WHERE type = :TYPE");
    query.bindValue(":TYPE", linkName);

    if (!query.exec()) {
        MythDB::DBError(LOG_ERR + "Could not find in DB", query);
    }

    return " (" + QString::number(query.size()) + ")";
}

/** \brief How many favourites/'program links' are there that are enabled?
 *  \return number of enabled program links */
int ProgramLink::getListSizeEnabled() { return getListEnabled().size(); }

/** \brief Returns a list of all the favourites/'program links'
 *  \return list of favourites/'program links in a fileFolderContainer' */
QList<FileFolderContainer> ProgramLink::getList() {
    QString programLinkDatabaseString("");
    if (!remote) {
        programLinkDatabaseString = "AND hostname = :HOSTNAME;";
    }

    LinkDataList.clear();

    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare("SELECT title,url,plot,image,autoPlay,seek,pinnedToHome FROM mythapps_programlink WHERE type = :TYPE " + programLinkDatabaseString);
    query.bindValue(":TYPE", linkName);
    query.bindValue(":HOSTNAME", gCoreContext->GetHostName());

    if (!query.exec()) {
        MythDB::DBError(LOG_ERR + "Could not find in DB", query);
    }

    int count = 0;
    while (query.next()) {
        count++;

        if (mostRecentOnly and count > 30) {
            break;
        }

        FileFolderContainer fileFolderContainerTemp;
        fileFolderContainerTemp.title = query.value(0).toString();
        fileFolderContainerTemp.url = query.value(1).toString();
        fileFolderContainerTemp.plot = query.value(2).toString();
        fileFolderContainerTemp.image = query.value(3).toString();
        fileFolderContainerTemp.autoPlay = query.value(4).toBool();
        fileFolderContainerTemp.seek = query.value(5).toString();
        fileFolderContainerTemp.pinnedToHome = query.value(6).toBool();

        LinkDataList.append(fileFolderContainerTemp);
    }

    return LinkDataList;
}

/** \brief Returns a list of all the enabled searchlist links'
 *  \return enabled searchlist links' */
QStringList ProgramLink::getListEnabled() {
    QStringList enabledList;

    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare("SELECT url FROM mythapps_programlink  WHERE type = :TYPE AND enabled = 1");
    query.bindValue(":TYPE", linkName);

    if (!query.exec()) {
        MythDB::DBError(LOG_ERR + "Could not find in DB", query);
    }
    while (query.next()) {
        enabledList.append(query.value(0).toString());
    }
    return enabledList;
}

/** \brief Does the favourite/'program link exist in the mythtv database
 * 	\param url url of the favourite/'program link'
 *  \return if the favourite/'program link exist*/
bool ProgramLink::contains(QString url) {
    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare("SELECT id FROM mythapps_programlink  WHERE type = :TYPE AND url = :URL");
    query.bindValue(":TYPE", linkName);
    query.bindValue(":URL", url);

    if (!query.exec()) {
        MythDB::DBError(LOG_ERR + "Could not find in DB", query);
    }

    if (query.size() > 0) {
        return true;
    }
    return false;
}

/** \brief Does part of the favourite/'program link exist in the mythtv database
 * 	\param url url of the favourite/'program link'
 *  \return if the favourite/'program link exist*/
bool ProgramLink::containsLike(QString url) {
    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare("SELECT id FROM mythapps_programlink  WHERE type = :TYPE AND url LIKE '" + url + "%'");
    query.bindValue(":TYPE", linkName);

    if (!query.exec()) {
        MythDB::DBError(LOG_ERR + "Could not find in DB", query);
    }

    if (query.size() > 0) {
        return true;
    }
    return false;
}

/** \brief find the enabled search url from a base url
 * 	\param url base url of the favourite/'program link'
 *  \return search url*/
QString ProgramLink::findSearchUrl(QString url) {
    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare("SELECT url FROM mythapps_programlink  WHERE type = :TYPE AND enabled = 1 AND url LIKE '" + url + "%'");
    query.bindValue(":TYPE", linkName);

    if (!query.exec()) {
        MythDB::DBError(LOG_ERR + "Could not find in DB", query);
    }
    while (query.next()) {
        return query.value(0).toString();
    }
    return "";
}

/** \brief Add a favourite/'program link' to the mythtv database
 * 	\param fileFolderContainerTemp program data of the favourite/'program link' */
void ProgramLink::append(FileFolderContainer fileFolderContainerTemp) {
    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare("INSERT INTO mythapps_programlink (type,title,url,plot,image,autoPlay,seek,pinnedToHome,hostname,enabled) "
                  " VALUES( :TYPE, :TITLE, :URL, :PLOT, :IMAGE, :AUTOPLAY, :SEEK, :PINNEDTOHOME, :HOSTNAME, :ENABLED);");
    query.bindValue(":TYPE", linkName);
    query.bindValue(":TITLE", fileFolderContainerTemp.title);
    query.bindValue(":URL", fileFolderContainerTemp.url);
    query.bindValue(":PLOT", fileFolderContainerTemp.plot);
    query.bindValue(":IMAGE", fileFolderContainerTemp.image);
    query.bindValue(":AUTOPLAY", fileFolderContainerTemp.autoPlay);
    query.bindValue(":SEEK", fileFolderContainerTemp.seek);
    query.bindValue(":PINNEDTOHOME", false);
    query.bindValue(":ENABLED", true);

    if (remote) {
        query.bindValue(":HOSTNAME", "");
    } else {
        query.bindValue(":HOSTNAME", gCoreContext->GetHostName());
    }

    if (!query.exec() || !query.isActive()) {
        MythDB::DBError("mythapps: inserting in DB", query);
    }
}

/** \brief Add a search url to the mythtv database
 * 	\param currentSearchUrl current search url */
void ProgramLink::appendSearchUrl(QString currentSearchUrl) {
    FileFolderContainer fileFolderContainerTemp;
    fileFolderContainerTemp.url = currentSearchUrl;
    fileFolderContainerTemp.autoPlay = false;

    // replace search link with incognito search link if discovered
    if (fileFolderContainerTemp.url.compare("?incognito=True")) {
        QString baseUrl = fileFolderContainerTemp.url.replace("?incognito=True", "");
        if (getListEnabled().contains(baseUrl)) { // update base url to disabled
            MSqlQuery query(MSqlQuery::InitCon());
            query.prepare("UPDATE mythapps_programlink SET enabled = false WHERE type = 'searchList' AND url = :URL");
            query.bindValue(":URL", baseUrl);
            query.exec();
        }
    }
}

/** \brief Removes the favourite/'program link' by URL from the mythtv database
 *  \param fileFolderContainerTemp program data container*/
void ProgramLink::listRemove(FileFolderContainer fileFolderContainerTemp) {
    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare("DELETE FROM mythconverg.mythapps_programlink WHERE type = :TYPE AND url = :URL;");
    query.bindValue(":TYPE", linkName);
    query.bindValue(":URL", fileFolderContainerTemp.url);
    if (!query.exec() || !query.isActive()) {
        MythDB::DBError("mythapps: delete from db", query);
    }
}

/** \brief is the program link pinned to the home screen?
 *  \param fileFolderContainerTemp program data container
 *  \return is the program link pinned to the home screen?  */
bool ProgramLink::isPinnedToHome(FileFolderContainer fileFolderContainerTemp) {
    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare("SELECT id FROM mythapps_programlink  WHERE type = :TYPE AND url = :URL AND pinnedToHome = true;");
    query.bindValue(":TYPE", linkName);
    query.bindValue(":URL", fileFolderContainerTemp.url);

    if (!query.exec()) {
        MythDB::DBError(LOG_ERR + "Could not find isPinnedToHome in DB", query);
    }

    if (query.size() > 0) {
        return true;
    }
    return false;
}

/** \brief remove the program link from the home screen
 *  \param fileFolderContainerTemp program data container  */
void ProgramLink::removeFromHomeScreen(FileFolderContainer fileFolderContainerTemp) {
    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare("UPDATE mythapps_programlink SET pinnedToHome = false WHERE type = :TYPE AND url = :URL AND pinnedToHome = true;");
    query.bindValue(":TYPE", linkName);
    query.bindValue(":URL", fileFolderContainerTemp.url);

    if (!query.exec()) {
        MythDB::DBError(LOG_ERR + "Could not update isPinnedToHome in DB", query);
    }
}

/** \brief add the program link from the home screen
 *  \param fileFolderContainerTemp program data container */
void ProgramLink::addToHomeScreen(FileFolderContainer fileFolderContainerTemp) {
    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare("UPDATE mythapps_programlink SET pinnedToHome = true WHERE type = :TYPE AND url = :URL AND pinnedToHome = false;");
    query.bindValue(":TYPE", linkName);
    query.bindValue(":URL", fileFolderContainerTemp.url);

    if (!query.exec()) {
        MythDB::DBError(LOG_ERR + "Could not update isPinnedToHome in DB", query);
    }
}

/** \brief get the unwatched size. (seek is false)  */
QString ProgramLink::getUnWatchedSize() {
    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare("SELECT id FROM mythapps_programlink  WHERE type = :TYPE AND seek = 'false'");
    query.bindValue(":TYPE", linkName);

    if (!query.exec()) {
        MythDB::DBError(LOG_ERR + "Could not find in DB", query);
    }

    return " (" + QString::number(query.size()) + ")";
}
