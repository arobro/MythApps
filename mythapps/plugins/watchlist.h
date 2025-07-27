#ifndef WATCHLIST_H
#define WATCHLIST_H

#include "plugin_api.h"

// QT headers
#include <QStringList>

// MythApps headers
#include "programData.h"
#include "programLink.h"

class WatchList : public PluginAPI {
  public:
    WatchList();
    ~WatchList();

    QString getPluginName() const override;
    QString getPluginDisplayName() override;
    bool getPluginStartPos() const override;
    QString getPluginIcon() const override;

    void setDialog(Dialog *d) override;
    void load(const QString data) override;
    void displayHomeScreenItems() override;

  private:
    void loadWatchList(bool unwatched);

    QString pluginName;
    QString pluginIcon;
    ProgramLink watchedLink;

    QString recent_icon;
};

#endif // WATCHLIST_H
