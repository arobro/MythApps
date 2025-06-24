#include "plugin_manager.h"

// QT headers
#include <QDebug>
#include <QDir>
#include <QPluginLoader>

// MythApps headers
#include "favourites.h"
#include "shared.h"

PluginManager::PluginManager() {
    loadFavouritesPlugin();
    if (m_favourites)
        m_plugins[m_favourites->getPluginName()] = m_favourites;
}

PluginManager::~PluginManager() {
    if (m_favourites)
        delete m_favourites;
}

bool PluginManager::loadPlugin(const QString &pluginPath) {
    QPluginLoader loader(pluginPath);
    QObject *plugin = loader.instance();
    if (plugin) {
        PluginAPI *pluginAPI = qobject_cast<PluginAPI *>(plugin);
        if (pluginAPI) {
            m_plugins[pluginAPI->getPluginName()] = pluginAPI;
            m_pluginIcons[pluginAPI->getPluginIcon()] = pluginAPI->getPluginIcon();
            return true;
        }
    }
    return false;
}

bool PluginManager::loadFavouritesPlugin() {
    if (!m_favourites)
        m_favourites = new Favourites();
    return m_favourites != nullptr;
}

QList<PluginDisplayInfo> PluginManager::getPluginsForDisplay() const {
    QList<PluginDisplayInfo> list;
    if (m_favourites) {
        PluginDisplayInfo info;
        info.name = m_favourites->getPluginDisplayName();
        info.iconPath = createImageCachePath(m_favourites->getPluginIcon());
        info.setData = "app://" + m_favourites->getPluginName() + "/~";
        list.append(info);
    }
    return list;
}

void PluginManager::setLoadProgramCallback(PluginAPI::LoadProgramCallback cb) {
    if (m_favourites)
        m_favourites->setLoadProgramCallback(cb);
    // Add similar lines for other plugins if needed
}

void PluginManager::setToggleSearchVisibleCallback(PluginAPI::ToggleSearchVisibleCallback cb) {
    if (m_favourites)
        m_favourites->setToggleSearchVisibleCallback(cb);
    // Add similar lines for other plugins if needed
}

PluginAPI *PluginManager::getPluginByName(const QString &name) {
    openPluginName = name;

    if (m_plugins.contains(name)) {
        return m_plugins.value(name);
    }
    return nullptr;
}

bool PluginManager::isFavouritesPluginOpen(bool isHome) {
    if (!isHome) {
        return openPluginName == "Favourites";
    }
    return false;
}