#include "favourites.h"

// QT headers
#include <QObject>
#include <QString>

// MythApps headers
#include "plugin_api.h"
#include "shared.h"

Favourites::Favourites() : pluginName("Favourites"), pluginIcon("ma_favourites.png"), favLink("allFavourites") {}

Favourites::~Favourites() {}

QString Favourites::getPluginName() const { return pluginName; }

QString Favourites::getPluginDisplayName() const {
    ProgramLink favLink("allFavourites");
    return getPluginName() + favLink.getListSize();
}

QString Favourites::getPluginIcon() const { return pluginIcon; }

void Favourites::setDialog(Dialog *d) {}

void Favourites::load(const QString filePath) {
    m_toggleSearchVisibleCallback(false);
    loadFavourites(false);
}

void Favourites::displayHomeScreenItems() { loadFavourites(true); }

void Favourites::loadFavourites(bool displayOnHome) {
    Q_FOREACH (const FileFolderContainer &favourite, favLink.getList(true, 0)) {
        if (displayOnHome && !favourite.pinnedToHome) // Skip items not pinned to home
            continue;

        if (m_loadProgramCallback) {
            m_loadProgramCallback(favourite.title, createProgramData(favourite.url, favourite.plot, favourite.image, favourite.autoPlay, ""), favourite.image);
        }
    }
}
