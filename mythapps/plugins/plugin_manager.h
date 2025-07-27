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

// Plugins
#include "favourites.h"
#include "videos.h"
#include "watchlist.h"

struct PluginDisplayInfo {
    QString name;
    QString iconPath;
    QString setData;
};

class PluginManager {
  public:
    PluginManager();
    ~PluginManager();

    QList<PluginDisplayInfo> getPluginsForDisplay(bool start) const;

    bool loadPlugin(const QString &pluginPath);
    void setLoadProgramCallback(PluginAPI::LoadProgramCallback cb);
    void setToggleSearchVisibleCallback(PluginAPI::ToggleSearchVisibleCallback cb);
    void setGoBackCallback(PluginAPI::GoBackCallback cb);

    void setControls(Controls *c);
    void setDialog(Dialog *d);

    bool isFavouritesPluginOpen(bool isHome);

    PluginAPI *getPluginByName(const QString &name);

  private:
    template <typename T> bool initializePlugin(QScopedPointer<T> &pluginInstance, const QString &pluginName);

    QMap<QString, PluginAPI *> m_plugins;
    QMap<QString, QString> m_pluginIcons;
    QString openPluginName;

    QScopedPointer<Favourites> m_favourites;
    QScopedPointer<Videos> m_videos;
    QScopedPointer<WatchList> m_watchlist;
};

#endif /* PLUGIN_MANAGER_H */
