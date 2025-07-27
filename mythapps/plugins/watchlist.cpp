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

void WatchList::setDialog(Dialog *d) {}

void WatchList::load(const QString data) {
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
        m_loadProgramCallback(tr("Unwatched") + watchedLink.getUnWatchedSize(), appPathName + data, recent_icon);

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

        m_loadProgramCallback(watched.title, createProgramData(watched.url, watched.plot, watched.image, watched.autoPlay, seek), watched.image);
    }
}
