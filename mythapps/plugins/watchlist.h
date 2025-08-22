#ifndef WATCHLIST_H
#define WATCHLIST_H

#include "plugin_api.h"

// QT headers
#include <QStringList>

// MythApps headers
#include "programData.h"
#include "programLink.h"

class WatchList : public PluginAPI {
    Q_OBJECT
  public:
    WatchList();
    ~WatchList();

    QString getPluginName() const override;
    QString getPluginDisplayName() override;
    bool getPluginStartPos() const override;
    QString getPluginIcon() const override;

    void load(const QString label, const QString data) override;
    void displayHomeScreenItems() override;

    QStringList getOptionsMenuItems(ProgramData *, const QString &, bool appIsOpen) override;

    bool menuCallback(const QString &menuText, ProgramData *currentSelectionDetails) override;

    bool handleAction(const QString, MythUIType *, ProgramData *) override;

    void appendWatchedLink(FileFolderContainer &data);

  private:
    void loadWatchList(bool unwatched);
    void addToUnWatchedList(bool menu, ProgramData *currentSelectionDetails);

    QString pluginName;
    QString pluginIcon;
    ProgramLink watchedLink;

    QString recent_icon;
};

#endif // WATCHLIST_H
