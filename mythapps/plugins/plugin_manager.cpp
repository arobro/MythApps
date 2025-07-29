#include "plugin_manager.h"

// QT headers
#include <QDebug>
#include <QDir>
#include <QPluginLoader>

// MythApps headers
#include "shared.h"

PluginManager::PluginManager() {
    initializePlugin(m_favourites, "Favourites");
    initializePlugin(m_videos, "Videos");
    initializePlugin(m_watchlist, "WatchList");
}

PluginManager::~PluginManager(){};

template <typename T> bool PluginManager::initializePlugin(QScopedPointer<T> &pluginInstance, const QString &pluginName) {
    if (!pluginInstance)
        pluginInstance.reset(new T);
    if (pluginInstance) {
        m_plugins[pluginName] = pluginInstance.data();
        return true;
    }
    return false;
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

QList<PluginDisplayInfo> PluginManager::getPluginsForDisplay(bool start) const {
    QList<PluginDisplayInfo> list;

    auto addDisplayInfo = [&](PluginAPI *plugin) {
        if (!plugin)
            return;

        // Respect plugin start position
        if (plugin->getPluginStartPos() != start)
            return;

        // Special condition for Videos plugin
        if (plugin == m_videos.data() && start == plugin->getPluginStartPos()) {
            if (gCoreContext->GetSetting("MythAppsmyVideo") != "1")
                return;
        }

        PluginDisplayInfo info;
        info.name = plugin->getPluginDisplayName();
        info.iconPath = createImageCachePath(plugin->getPluginIcon());
        info.setData = "app://" + plugin->getPluginName() + "/~";
        list.append(info);
    };

    for (auto plugin : m_plugins.values()) {
        addDisplayInfo(plugin);
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
    if (m_plugins.contains(name)) {
        return m_plugins.value(name);
    }
    return nullptr;
}

void PluginManager::setControls(Controls *c) {
    for (auto plugin : m_plugins.values())
        plugin->setControls(c);
}

void PluginManager::setDialog(Dialog *d) {
    for (auto plugin : m_plugins.values())
        plugin->setDialog(d);
}

QList<QString> PluginManager::getOptionsMenuLabels(ProgramData *currentSelectionDetails, const QString &currentFilePath) const {
    QList<QString> labels;
    for (PluginAPI *plugin : m_plugins.values()) {
        labels.append(plugin->getOptionsMenuItems(currentSelectionDetails, currentFilePath));
    }
    return labels;
}

bool PluginManager::menuCallBack(const QString &menuText, ProgramData *currentSelectionDetails) {
    bool reload = false;
    for (PluginAPI *plugin : m_plugins.values()) {
        if (plugin->menuCallback(menuText, currentSelectionDetails)) {
            reload = true;
        }
    }
    return reload;
}

void PluginManager::handleAction(const QString action, ProgramData *currentSelectionDetails) {
    for (auto plugin : m_plugins.values())
        plugin->handleAction(action, currentSelectionDetails);
}

void PluginManager::appendWatchedLink(FileFolderContainer data) {
    if (m_watchlist)
        m_watchlist->appendWatchedLink(data);
}
