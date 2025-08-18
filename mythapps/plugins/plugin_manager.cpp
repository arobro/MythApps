#include "plugin_manager.h"

// QT headers
#include <QDebug>
#include <QDir>
#include <QPluginLoader>

// MythApps headers
#include "SafeDelete.h"
#include "shared.h"

PluginManager::PluginManager() {
    initializePlugin(m_favourites, "Favourites");
    initializePlugin(m_videos, "Videos");
    initializePlugin(m_watchlist, "WatchList");
    initializePlugin(m_ytCustom, "ytCustom");
    initializePlugin(m_music, "Music");
}

PluginManager::~PluginManager() {
    m_favourites.reset();
    m_videos.reset();
    m_watchlist.reset();
    m_ytCustom.reset();
    m_music.reset();

    m_plugins.clear();
    m_pluginIcons.clear();

    SafeDelete(m_lastOpenedPlugin);
};

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

        if (plugin->getPluginStartPos() != start) // Respect plugin start position
            return;

        if (plugin == m_videos.data() && start == plugin->getPluginStartPos()) {
            if (gCoreContext->GetSetting("MythAppsmyVideo") != "1")
                return;
        }

        if (plugin == m_ytCustom.data()) {
            if (gCoreContext->GetSetting("MythAppsYTnative") != "1")
                return;
        }

        if (plugin == m_music.data()) {
            if (gCoreContext->GetSetting("MythAppsMusic") != "1")
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

void PluginManager::setDisplayImageCallback(PluginAPI::DisplayImageCallback cb) {
    for (auto plugin : m_plugins.values())
        plugin->setDisplayImageCallback(cb);
}

void PluginManager::setToggleSearchVisibleCallback(PluginAPI::ToggleSearchVisibleCallback cb) {
    for (auto plugin : m_plugins.values())
        plugin->setToggleSearchVisibleCallback(cb);
}

void PluginManager::setGoBackCallback(PluginAPI::GoBackCallback cb) {
    for (auto plugin : m_plugins.values())
        plugin->setGoBackCallback(cb);
}

void PluginManager::setFocusWidgetCallback(PluginAPI::SetFocusWidgetCallback cb) {
    for (auto plugin : m_plugins.values())
        plugin->setFocusWidgetCallback(cb);
}

void PluginManager::setPlay_KodiCallback(PluginAPI::SetPlay_KodiCallback cb) {
    for (auto plugin : m_plugins.values())
        plugin->setPlay_KodiCallback(cb);
}

void PluginManager::setPlaybackInfoCallback(PluginAPI::PlaybackInfoCallback cb) {
    if (m_music)
        m_music->setPlaybackInfoCallback(cb);
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

void PluginManager::setUIContext(UIContext *uiC) {
    for (auto plugin : m_plugins.values())
        plugin->setUIContext(uiC);
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

QStringList PluginManager::hidePlugins() {
    QStringList hiddenPlugins;

    for (auto plugin : m_plugins.values()) {
        hiddenPlugins << plugin->hidePlugin();
    }

    return hiddenPlugins;
}

void PluginManager::search(QString searchText, QString appName) {
    for (auto plugin : m_plugins.values()) {
        if (plugin->getPluginName() == appName)
            plugin->search(searchText);
    }
}

bool PluginManager::handleSuggestion(const QString &searchText) {
    bool handled = false;
    for (auto plugin : m_plugins.values())
        handled = plugin->handleSuggestion(searchText);

    return handled;
}

bool PluginManager::useBasicMenu(QString appName) {
    for (auto plugin : m_plugins.values())
        if (plugin->getPluginName() == appName) {
            return plugin->useBasicMenu();
        }
    return true;
}

void PluginManager::initializeUI(MythUIType *ui) {
    for (auto plugin : m_plugins.values()) {
        if (!plugin->initializeUI(ui)) {
            LOG(VB_GENERAL, LOG_ERR, "Cannot load screen 'mythapps plugins'");
        }
    }
}

bool PluginManager::handleAction(const QString name, MythUIType *focusWidget) { return m_music->handleAction(name, focusWidget); }

void PluginManager::onTextMessageReceived(const QString &method, const QString &message) {
    for (auto plugin : m_plugins.values())
        plugin->onTextMessageReceived(method, message);
}

void PluginManager::setExitToMainMenuSleepTimer(QTimer *timer) {
    for (auto plugin : m_plugins.values())
        plugin->setExitToMainMenuSleepTimer(timer);
}

void PluginManager::setGoFullscreenCallback(PluginAPI::FullscreenCallback cb) {
    for (auto plugin : m_plugins.values())
        plugin->setGoFullscreenCallback(cb);
}

void PluginManager::exitPlugin() {
    if (m_lastOpenedPlugin) {
        m_lastOpenedPlugin->exitPlugin();
        m_lastOpenedPlugin = nullptr;
    }
}

void PluginManager::load(const QString &pluginName, const QString &label, const QString &data) {
    PluginAPI *plugin = getPluginByName(pluginName);
    if (!plugin)
        return;

    plugin->load(label, data);
    m_lastOpenedPlugin = plugin;
}
