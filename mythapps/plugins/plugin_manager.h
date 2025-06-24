#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include "plugin_api.h"

// QT headers
#include <QList>
#include <QMap>
#include <QString>
#include <functional>

#include "favourites.h"

// Move this struct outside the class
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
    QList<PluginDisplayInfo> getPluginsForDisplay() const;

    // Add these lines:
    bool loadPlugin(const QString &pluginPath);

    // Add this method
    void setLoadProgramCallback(PluginAPI::LoadProgramCallback cb);

    // Add this method
    void setToggleSearchVisibleCallback(PluginAPI::ToggleSearchVisibleCallback cb);

    bool isFavouritesPluginOpen(bool isHome);

    PluginAPI *getPluginByName(const QString &name);

  private:
    QMap<QString, PluginAPI *> m_plugins;
    QMap<QString, QString> m_pluginIcons;
    Favourites *m_favourites = nullptr;
    QString openPluginName;
};

#endif /* PLUGIN_MANAGER_H */