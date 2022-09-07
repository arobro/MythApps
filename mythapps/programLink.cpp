// MythTV headers
#include <libmyth/mythcontext.h>

// MythApps headers
#include "programLink.h"
#include "shared.h"

/** \class ProgramLink
 *  \brief Find, Add and remove program links such as favourites, watchlist, previously played & search List */
ProgramLink::ProgramLink(QString _linkName, bool _remote, bool _mostRecentOnly) {
    linkName = _linkName;
    remote = _remote;
    mostRecentOnly = _mostRecentOnly;

    load();
}

/** \brief Load the links from the database into a map */
void ProgramLink::load() {
    QString programLinkDatabaseString;
    if (remote) {
        programLinkDatabaseString = gCoreContext->GetSettingOnHost(linkName, gCoreContext->GetMasterHostName());
    } else {
        programLinkDatabaseString = gCoreContext->GetSetting(linkName);
    }

    QStringList programLinkDatabase = programLinkDatabaseString.split("~~");
    LinkDataList.clear();

    foreach (QString link, programLinkDatabase) {
        if (!link.compare("") == 0) {
            LinkDataList.append(link);
            listSizeEnabled++;
        }
    }

    // remove if more than 30 items
    bool updatedlist = false;
    while (mostRecentOnly and listSizeEnabled > 30) {
        LinkDataList.takeFirst();
        listSizeEnabled--;
        updatedlist = true;
    }

    if (updatedlist) {
        saveList();
    }
}

/** \brief How many favourites/'program links' are there?
 *  \return number of program links */
QString ProgramLink::getListSize() { return " (" + QString::number(LinkDataList.size()) + ")"; }

/** \brief How many favourites/'program links' are there that are enabled?
 *  \return number of enabled program links */
int ProgramLink::getListSizeEnabled() { return listSizeEnabled; }

/** \brief Returns a list of all the favourites/'program links'
 *  \return favourites/'program links' */
QStringList ProgramLink::getList() { return LinkDataList; }

/** \brief Returns a list of all the enabled favourites/'program links'
 *  \return enabled favourites/'program links' */
QStringList ProgramLink::getListEnabled() {
    QStringList enabledList;

    foreach (QString listItem, LinkDataList) {
        if (!listItem.contains("!")) {
            enabledList.append(listItem);
        }
    }
    return enabledList;
}

/** \brief Does the favourite/'program link exist in the mythtv database
 * 	\param currentselectionDetails data of the favourite/'program link'
 *  \return if the favourite/'program link exist*/
bool ProgramLink::contains(QString currentselectionDetails) {
    if (LinkDataList.contains(currentselectionDetails)) {
        return true;
    }
    return false;
}

/** \brief Remove a favourite/'program link' from the mythtv database
 * 	\param currentselectionDetails remove by data of the favourite/'program
 * link' */
void ProgramLink::removeOne(QString currentselectionDetails) {
    LinkDataList.removeOne(currentselectionDetails);
    saveList();
}

/** \brief Remove a favourite/'program link' from the mythtv database
 * 	\param line remove if contains data in the favourite/'program link' */
QString ProgramLink::listRemoveIfContains(QString line) {
    int lineCount = 0;
    QString removedList = "";

    foreach (QString l, LinkDataList) {
        if (l.split(",").at(0).compare(line) == 0) {
            removedList = l;
            LinkDataList.removeAt(lineCount);
        }
        lineCount++;
    }
    return removedList;
}

/** \brief Add a favourite/'program link' to the mythtv database
 * 	\param currentselectionDetails data of the favourite/'program link' */
void ProgramLink::append(QString currentselectionDetails) {
    // replace search link with incognito search link if discovered
    if (currentselectionDetails.compare("?incognito=True")) {
        QString baseUrl = currentselectionDetails;
        baseUrl.replace("?incognito=True", "");

        QString listItem = listRemoveIfContains(baseUrl);
        if (!listItem.isEmpty()) {
            LinkDataList.append("!" + listItem);
        }
    }

    LinkDataList.append(currentselectionDetails);
    saveList();
}

/** \brief Save the favourite/'program link' to the mythtv database */
void ProgramLink::saveList() {
    QString allList = "";
    foreach (QString l, LinkDataList) { allList = allList + l + QString("~~"); }
    if (remote) {
        gCoreContext->SaveSettingOnHost(linkName, allList, gCoreContext->GetMasterHostName());
    } else {
        gCoreContext->SaveSetting(linkName, allList);
    }
}
/** \brief Removes the favourite/'program link' by VALUE from the mythtv
 * database \param selectionDetails remove by data of the favourite/'program
 * link' \return string of item removed */
void ProgramLink::listRemove(QString selectionDetails) {
    QList selectionDetailsList = selectionDetails.split("~");

    foreach (QString l, LinkDataList) {
        if (l.contains(friendlyUrl(selectionDetailsList.at(1)))) {
            LinkDataList.removeOne(l);
        }
    }
    saveList();
}

/** \brief Sort the favourite/'program links' */
void ProgramLink::sort() { LinkDataList.sort(); }

/** \brief is the program link pinned to the home screen?
 * 	\param currentselectionDetails data of the favourite/'program link'
 *  \return is the program link pinned to the home screen?  */
bool ProgramLink::isPinnedToHome(QString currentselectionDetails) {
    if (currentselectionDetails.contains("(On Home Screen) ")) {
        return true;
    }
    return false;
}

/** \brief remove the program link from the home screen
 * 	\param currentselectionDetails data of the favourite/'program link'  */
void ProgramLink::removeFromHomeScreen(QString currentselectionDetails) {
    int pos = LinkDataList.indexOf(currentselectionDetails);
    LinkDataList[pos] = currentselectionDetails.replace("(On Home Screen) ", "");

    saveList();
}

/** \brief add the program link from the home screen
 * 	\param currentselectionDetails data of the favourite/'program link'  */
void ProgramLink::addToHomeScreen(QString currentselectionDetails) {
    int pos = LinkDataList.indexOf(currentselectionDetails);

    QStringList currentselectionDetailsList = currentselectionDetails.split("~");
    QString plot = currentselectionDetailsList.at(2);
    currentselectionDetailsList[2] = "(On Home Screen) " + plot;
    LinkDataList[pos] = currentselectionDetailsList.join("~");
    saveList();
}

/** \brief get the unwatched size. (seek is false)  */
QString ProgramLink::getUnWatchedSize() {
    int count = 0;
    QListIterator<QString> watched(getList());
    watched.toBack();
    while (watched.hasPrevious()) {
        QStringList watchedItem = watched.previous().split("~");

        if (watchedItem.size() > 5) {
            QString seek = watchedItem.at(5);
            if (!seek.compare("false") == 0) {
                continue;
            }
            count++;
        }
    }
    return " (" + QString::number(count) + ")";
}
