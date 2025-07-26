#ifndef FAVOURITES_H
#define FAVOURITES_H

#include "plugin_api.h"

// QT headers
#include <QStringList>

// MythApps headers
#include "programData.h"
#include "programLink.h"

class Favourites : public PluginAPI {
  public:
    Favourites();
    ~Favourites();

    QString getPluginName() const override;
    QString getPluginDisplayName() const override;
    QString getPluginIcon() const override;
    void setDialog(Dialog *d) override;

    void load(const QString filePath) override;
    void displayHomeScreenItems() override;
    void loadFavourites(bool displayOnHome);

  private:
    QString pluginName;
    QString pluginIcon;
    ProgramLink favLink; /*!< load Favourites */
};

#endif /* FAVOURITES_H */
