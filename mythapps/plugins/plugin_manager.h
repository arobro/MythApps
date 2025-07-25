#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include "plugin_api.h"

// QT headers
#include <QList>
#include <QMap>
#include <QString>
#include <functional>

// MythApps headers
#include "controls.h"
#include "dialog.h"
#include "fileBrowserHistory.h"

// Plugins
#include "favourites.h"
#include "videos.h"

struct PluginDisplayInfo {
    QString name;
    QString iconPath;
    QString setData;
};

class PluginManager {
  public:
    PluginManager();
    ~PluginManager();

    bool loadFavouritesPlugin();
    bool loadVideosPlugin();

    QList<PluginDisplayInfo> getPluginsForDisplay(bool start) const;

    bool loadPlugin(const QString &pluginPath);
    void setLoadProgramCallback(PluginAPI::LoadProgramCallback cb);
    void setToggleSearchVisibleCallback(PluginAPI::ToggleSearchVisibleCallback cb);
    void setGoBackCallback(PluginAPI::GoBackCallback cb);

    void setControls(Controls *c);
    void setDialog(Dialog *d);
    void setFileBrowserHistory(FileBrowserHistory *f);

    bool isFavouritesPluginOpen(bool isHome);

    PluginAPI *getPluginByName(const QString &name);

  private:
    QMap<QString, PluginAPI *> m_plugins;
    QMap<QString, QString> m_pluginIcons;
    QString openPluginName;

    Favourites *m_favourites{nullptr};
    Videos *m_videos{nullptr};
};

#endif /* PLUGIN_MANAGER_H */
