#include "programLink.h"

// MythApps headers
#include "shared.h"

/** \class ProgramLink
 *  \brief Find, Add and remove program links such as favourites, watchlist, previously played & search List */

ProgramLink::ProgramLink(QString _linkName) { linkName = _linkName; }

/** \brief How many favourites/'program links' are there?
 *  \return number of program links */
QString ProgramLink::getListSize() {
    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare("SELECT id FROM mythapps_programlink  WHERE type = :TYPE");
    query.bindValue(":TYPE", linkName);
    execQuery(query);

    return " (" + QString::number(query.size()) + ")";
}

/** \brief How many favourites/'program links' are there that are enabled?
 *  \return number of enabled program links */
int ProgramLink::getListSizeEnabled() { return getListEnabled().size(); }

/** \brief Returns a list of all the favourites/'program links'
 *  \param descending If true, results are ordered by id descending
 *  \param limit Maximum number of results to return (0 means no limit)
 *  \return list of favourites/'program links in a fileFolderContainer'
 */
QList<FileFolderContainer> ProgramLink::getList(bool descending, int limit) {
    LinkDataList.clear();

    MSqlQuery query(MSqlQuery::InitCon());
    QString sql = "SELECT title, url, plot, image, autoPlay, seek, pinnedToHome "
                  "FROM mythapps_programlink WHERE type = :TYPE ORDER BY id ";
    sql += descending ? "DESC " : "ASC ";

    if (limit > 0) {
        sql += QString("LIMIT %1 ").arg(limit);
    }

    query.prepare(sql);
    query.bindValue(":TYPE", linkName);
    execQuery(query);

    while (query.next()) {
        FileFolderContainer item;
        item.title = query.value(0).toString();
        item.url = query.value(1).toString();
        item.plot = query.value(2).toString();
        item.image = query.value(3).toString();
        item.autoPlay = query.value(4).toBool();
        item.seek = query.value(5).toString();
        item.pinnedToHome = query.value(6).toBool();

        LinkDataList.append(item);
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
    execQuery(query);

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
    execQuery(query);

    return query.size() > 0;
}

/** \brief Does part of the favourite/'program link exist in the mythtv database
 * 	\param url url of the favourite/'program link'
 *  \return if the favourite/'program link exist*/
bool ProgramLink::containsLike(QString url) {
    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare("SELECT id FROM mythapps_programlink  WHERE type = :TYPE AND url LIKE '" + url + "%'");
    query.bindValue(":TYPE", linkName);
    execQuery(query);

    return query.size() > 0;
}

/** \brief find the enabled search url from a base url
 * 	\param url base url of the favourite/'program link'
 *  \return search url*/
QString ProgramLink::findSearchUrl(QString url) {
    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare("SELECT url FROM mythapps_programlink  WHERE type = :TYPE AND enabled = 1 AND url LIKE '" + url + "%'");
    query.bindValue(":TYPE", linkName);
    execQuery(query);

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
    query.bindValue(":HOSTNAME", "");

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
    execQuery(query);
}

/** \brief is the program link pinned to the home screen?
 *  \param fileFolderContainerTemp program data container
 *  \return is the program link pinned to the home screen?  */
bool ProgramLink::isPinnedToHome(FileFolderContainer fileFolderContainerTemp) {
    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare("SELECT id FROM mythapps_programlink  WHERE type = :TYPE AND url = :URL AND pinnedToHome = true;");
    query.bindValue(":TYPE", linkName);
    query.bindValue(":URL", fileFolderContainerTemp.url);
    execQuery(query);

    return query.size() > 0;
}

/** \brief remove the program link from the home screen
 *  \param fileFolderContainerTemp program data container  */
void ProgramLink::removeFromHomeScreen(FileFolderContainer fileFolderContainerTemp) {
    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare("UPDATE mythapps_programlink SET pinnedToHome = false WHERE type = :TYPE AND url = :URL AND pinnedToHome = true;");
    query.bindValue(":TYPE", linkName);
    query.bindValue(":URL", fileFolderContainerTemp.url);
    execQuery(query);
}

/** \brief add the program link from the home screen
 *  \param fileFolderContainerTemp program data container */
void ProgramLink::addToHomeScreen(FileFolderContainer fileFolderContainerTemp) {
    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare("UPDATE mythapps_programlink SET pinnedToHome = true WHERE type = :TYPE AND url = :URL AND pinnedToHome = false;");
    query.bindValue(":TYPE", linkName);
    query.bindValue(":URL", fileFolderContainerTemp.url);
    execQuery(query);
}

/** \brief get the unwatched size. (seek is false)  */
QString ProgramLink::getUnWatchedSize() {
    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare("SELECT id FROM mythapps_programlink  WHERE type = :TYPE AND seek = 'false'");
    query.bindValue(":TYPE", linkName);
    execQuery(query);
    return " (" + QString::number(query.size()) + ")";
}

/** \brief Executes a query and logs an error if it fails
 *  \param query The prepared MSqlQuery object
 *  \param context Description or context for the error log */
void ProgramLink::execQuery(MSqlQuery &query) {
    if (!query.exec()) {
        MythDB::DBError(LOG_ERR + "programLink db error", query);
    }
}
