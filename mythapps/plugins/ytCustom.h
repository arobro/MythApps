#ifndef YTCUSTOM_H
#define YTCUSTOM_H

#include "plugin_api.h"

// QT headers
#include <QString>
#include <QVariantMap>

// MythTV headers
#include <libmythui/mythuibuttonlist.h>

// MythApps headers
#include "controls.h"
#include "netRequest.h"
#include "uiContext.h"

class ytCustom : public PluginAPI {
    Q_OBJECT
  public:
    ytCustom();
    ~ytCustom() override;

    QString getPluginName() const override;
    QString getPluginDisplayName() override;
    bool getPluginStartPos() const override;
    QString getPluginIcon() const override;
    QString hidePlugin() override;
    bool useBasicMenu() override;

    void load(const QString label, const QString data) override;
    void displayHomeScreenItems() override;

    void search(const QString &searchText) override;
    bool handleSuggestion(const QString searchText);

  private:
    void loadYTCustom();
    void updateMediaListCallback(const QString &label, const QString &data);
    void initializeSettingsDialog();

    QString logoUrl() const;
    void processAndSaveImage(const QByteArray &imageData, const QString &outputPath) const;

    QString getKodiYTPlayUrl();
    QString getKodiYTPluginDomain();
    QString getAPIBaseUrl();

    QList<QVariant> getVideos(QString searchText, QString directory);
    void loadProgramList(QString searchText, QString directory);

    QString translateWordtoRFC3339Date(const QString &date);
    QStringList getlist(int currentPos, bool forward);
    QString getRegionCodeFromOS();

    NetRequest *netRequest{nullptr};

    QString ytApi;
    QString ytID;

    QString sortBy;
    QString videoDuration;
    QString type;
    QString eventType;

    QString dateBeforeUrlBuilder;
    QString dateAfterUrlBuilder;

    QStringList dateListBefore;
    QStringList dateListAfter;

    MythUIButtonListItem *dateBefore;
    MythUIButtonListItem *dateAfter;

    QString pluginName;
    QString pluginIcon;

    QString ma_search_icon;
    QString ma_popular_icon;

  private slots:
    void searchSettingsClicked(MythUIButtonListItem *item);
};

#endif // YTCUSTOM_H
