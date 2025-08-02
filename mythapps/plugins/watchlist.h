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
    void load(const QString label, const QString data) override;
    void displayHomeScreenItems() override;

    QStringList getOptionsMenuItems(ProgramData *currentSelectionDetails, const QString &currentFilePath) override;
    bool menuCallback(const QString &menuText, ProgramData *currentSelectionDetails) override;

    void handleAction(const QString action, ProgramData *currentSelectionDetails);
    void appendWatchedLink(FileFolderContainer &data);

  private:
    void loadWatchList(bool unwatched);
    void addToUnWatchedList(bool menu, ProgramData *currentSelectionDetails);

    QString pluginName;
    QString pluginIcon;
    ProgramLink watchedLink;

    QString recent_icon;

    Dialog *dialog{nullptr};
};

#endif // WATCHLIST_H
