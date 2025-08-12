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
#include "uiContext.h"

// Plugins
#include "favourites.h"
#include "videos.h"
#include "watchlist.h"
#include "ytCustom.h"

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
    void setFocusWidgetCallback(PluginAPI::SetFocusWidgetCallback cb);
    void setPlay_KodiCallback(PluginAPI::SetPlay_KodiCallback cb);

    void setControls(Controls *c);
    void setDialog(Dialog *d);
    void setUIContext(UIContext *uiC);

    PluginAPI *getPluginByName(const QString &name);

    QList<QString> getOptionsMenuLabels(ProgramData *currentSelectionDetails, const QString &currentFilePath) const;
    bool menuCallBack(const QString &menuText, ProgramData *currentSelectionDetails);

    void handleAction(const QString action, ProgramData *currentSelectionDetails);

    QStringList hidePlugins();
    void search(QString searchText, QString appName);
    bool handleSuggestion(const QString &);

    bool useBasicMenu(QString appName);

    // non API
    void appendWatchedLink(FileFolderContainer data);
    void searchSettingsClicked(MythUIButtonListItem *item);

  private:
    template <typename T> bool initializePlugin(QScopedPointer<T> &pluginInstance, const QString &pluginName);

    QMap<QString, PluginAPI *> m_plugins;
    QMap<QString, QString> m_pluginIcons;

    QScopedPointer<Favourites> m_favourites;
    QScopedPointer<Videos> m_videos;
    QScopedPointer<WatchList> m_watchlist;
    QScopedPointer<ytCustom> m_ytCustom;
};

#endif /* PLUGIN_MANAGER_H */
