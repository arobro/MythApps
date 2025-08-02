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

QString Favourites::getPluginDisplayName() { return getPluginName() + favLink.getListSize(); }

bool Favourites::getPluginStartPos() const { return true; }

QString Favourites::getPluginIcon() const { return pluginIcon; }

void Favourites::setDialog(Dialog *d) {}

void Favourites::load(const QString label, const QString data) {
    m_toggleSearchVisibleCallback(false);
    loadFavourites(false);
}

void Favourites::displayHomeScreenItems() { loadFavourites(true); }

void Favourites::loadFavourites(bool displayOnHome) {
    Q_FOREACH (const FileFolderContainer &favourite, favLink.getList(true, 0)) {
        if (displayOnHome && !favourite.pinnedToHome) // Skip items not pinned to home
            continue;

        m_loadProgramCallback(favourite.title, createProgramData(favourite.url, favourite.plot, favourite.image, favourite.autoPlay, ""), favourite.image);
    }
}

QStringList Favourites::getOptionsMenuItems(ProgramData *currentSelectionDetails, const QString &currentFilePath) {
    QStringList options;

    if (favLink.contains(currentSelectionDetails->getUrl())) {
        options << tr("Remove Selection from Favourites");
    } else {
        options << tr("Add Selection to Favourites");
    }

    if (favLink.isPinnedToHome(currentSelectionDetails->get())) {
        options << tr("Remove Selection from Home Screen");
    } else if (favLink.contains(currentSelectionDetails->getUrl()) && currentFilePath.contains("app://Favourites")) {
        options << tr("Pin Selection to Home Screen");
    }

    return options;
}

bool Favourites::menuCallback(const QString &menuText, ProgramData *currentSelectionDetails) {
    if (menuText == tr("Add Selection to Favourites")) {
        if (currentSelectionDetails->isPlayRequest()) { // remove seek time when saving to favourites
            currentSelectionDetails->resetSeek();
        }
        favLink.append(currentSelectionDetails->get()); // add to favourites
        return false;
    } else if (menuText == tr("Remove Selection from Favourites")) {
        if (favLink.contains(currentSelectionDetails->getUrl())) {
            favLink.listRemove(currentSelectionDetails->get()); // Remove from favourite
            return true;                                        // reload
        }
    } else if (menuText == tr("Remove Selection from Home Screen")) {
        favLink.removeFromHomeScreen(currentSelectionDetails->get());
        return true; // reload
    } else if (menuText == tr("Pin Selection to Home Screen")) {
        favLink.addToHomeScreen(currentSelectionDetails->get());
        return true; // reload
    }
    return false;
}
