#include "plugin_manager.h"

// QT headers
#include <QDebug>
#include <QDir>
#include <QPluginLoader>

// MythApps headers
#include "shared.h"

PluginManager::PluginManager() {
    loadFavouritesPlugin();
    if (m_favourites)
        m_plugins[m_favourites->getPluginName()] = m_favourites;

    loadVideosPlugin();
    if (m_videos)
        m_plugins[m_videos->getPluginName()] = m_videos;
}

PluginManager::~PluginManager() {
    if (m_favourites)
        delete m_favourites;

    if (m_videos)
        delete m_videos;
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

bool PluginManager::loadVideosPlugin() {
    if (!m_videos)
        m_videos = new Videos();
    return m_videos != nullptr;
}

QList<PluginDisplayInfo> PluginManager::getPluginsForDisplay(bool start) const {
    QList<PluginDisplayInfo> list;
    if (m_favourites && start) {
        PluginDisplayInfo info;
        info.name = m_favourites->getPluginDisplayName();
        info.iconPath = createImageCachePath(m_favourites->getPluginIcon());
        info.setData = "app://" + m_favourites->getPluginName() + "/~";
        list.append(info);
    }

    if (m_videos && !start) {
        if (gCoreContext->GetSetting("MythAppsmyVideo").compare("1") == 0) {
            PluginDisplayInfo info;
            info.name = m_videos->getPluginDisplayName();
            info.iconPath = createImageCachePath(m_videos->getPluginIcon());
            info.setData = "app://" + m_videos->getPluginName() + "/~";
            list.append(info);
        }
    }

    return list;
}

void PluginManager::setLoadProgramCallback(PluginAPI::LoadProgramCallback cb) {
    for (auto plugin : m_plugins.values())
        plugin->setLoadProgramCallback(cb);
}

void PluginManager::setToggleSearchVisibleCallback(PluginAPI::ToggleSearchVisibleCallback cb) {
    for (auto plugin : m_plugins.values())
        plugin->setToggleSearchVisibleCallback(cb);
}

void PluginManager::setGoBackCallback(PluginAPI::GoBackCallback cb) {
    for (auto plugin : m_plugins.values())
        plugin->setGoBackCallback(cb);
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

void PluginManager::setControls(Controls *c) {
    if (m_videos)
        m_videos->setControls(c);
}

void PluginManager::setDialog(Dialog *d) {
    for (auto plugin : m_plugins.values())
        plugin->setDialog(d);
}

void PluginManager::setFileBrowserHistory(FileBrowserHistory *f) {
    for (auto plugin : m_plugins.values())
        plugin->setFileBrowserHistory(f);
}
