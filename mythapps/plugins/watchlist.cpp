#include "watchlist.h"

// QT headers
#include <QObject>
#include <QString>

// MythApps headers
#include "plugin_api.h"
#include "shared.h"

// Globals
const QString appPathName = "app://WatchList/";

WatchList::WatchList() : pluginName("WatchList"), pluginIcon("ma_recent.png"), watchedLink("allWatched") { recent_icon = createImageCachePath("ma_recent.png"); }

WatchList::~WatchList() = default;

QString WatchList::getPluginName() const { return pluginName; }

QString WatchList::getPluginDisplayName() { return pluginName; }

bool WatchList::getPluginStartPos() const { return true; }

QString WatchList::getPluginIcon() const { return pluginIcon; }

void WatchList::load(const QString label, const QString data) {
    m_toggleSearchVisibleCallback(false);

    if (data.length() < 2) {
        loadWatchList(false);
    } else {
        loadWatchList(true);
    }
}

void WatchList::displayHomeScreenItems() {}

void WatchList::loadWatchList(bool unwatched) {
    int limit = 0;

    if (!unwatched) {
        QString data = createProgramData("Unwatched", tr("Unwatched"), recent_icon, "", "");
        m_loadProgramCallback(tr("Unwatched") + watchedLink.getUnWatchedSize(), appPathName + data, recent_icon, nullptr);

        limit = 22;
    }

    Q_FOREACH (const FileFolderContainer &watched, watchedLink.getList(true, limit)) {
        QString seek = watched.seek;
        if (seek.compare("false") == 0 and !unwatched) {
            seek = "";
            continue;
        } else if (!seek.compare("false") == 0 and unwatched) {
            continue;
        }

        m_loadProgramCallback(watched.title, createProgramData(watched.url, watched.plot, watched.image, watched.autoPlay, seek), watched.image, nullptr);
    }
}

QStringList WatchList::getOptionsMenuItems(ProgramData *currentSelectionDetails, const QString &currentFilePath) {
    Q_UNUSED(currentFilePath);

    QStringList options;
    bool inWatchList = watchedLink.contains(currentSelectionDetails->getUrl());

    if (inWatchList) {
        options << tr("Remove from Watch List");
    } else if (currentSelectionDetails->isPlayRequest()) {
        options << tr("Add to Watch List for Later Viewing");
    }

    if (currentSelectionDetails->getPreviouslyPlayed()) {
        options << tr("Clear Previously Played");
    }

    return options;
}

bool WatchList::menuCallback(const QString &menuText, ProgramData *currentSelectionDetails) {
    if (menuText == tr("Remove from Watch List")) {
        watchedLink.listRemove(currentSelectionDetails->get());
        return true; // reload

    } else if (menuText == tr("Add to Watch List for Later Viewing")) {
        addToUnWatchedList(false, currentSelectionDetails);
    }
    return false;
}

/** \brief add to the watched list as unwatched for later viewing  */
void WatchList::addToUnWatchedList(bool menu, ProgramData *currentSelectionDetails) {
    LOG(VB_GENERAL, LOG_INFO, "addToUnWatchedList");
    if (currentSelectionDetails->isPlayRequest()) {
        if (!watchedLink.contains(currentSelectionDetails->getUrl())) {
            currentSelectionDetails->setUnWatched();
            watchedLink.append(currentSelectionDetails->get());
            if (menu) {
                dialog->createAutoClosingBusyDialog(tr("Added to Watch List for Later Viewing"), 3);
            }
        } else {
            dialog->createAutoClosingBusyDialog(tr("Already in Watch List"), 3);
        }
    } else {
        dialog->createAutoClosingBusyDialog(tr("Unable to add to Watch List as not a video"), 3);
    }
}

void WatchList::handleAction(const QString action, ProgramData *currentSelectionDetails) {
    if (action == "TOGGLERECORD")
        addToUnWatchedList(true, currentSelectionDetails);
}

void WatchList::appendWatchedLink(FileFolderContainer &data) { watchedLink.append(data); }
