#ifndef ytCustomApp_h
#define ytCustomApp_h

// QT headers
#include <QObject>

// MythTV headers
#include <libmythui/mythuibuttonlist.h>

// MythApps headers
#include "browser.h"
#include "netRequest.h"
#include "shared.h"

/** \class YT
 *  \brief */

class ytCustomApp : public QObject {
    Q_OBJECT
  public:
    ytCustomApp(QString m_username, QString m_password, QString m_ip, QString m_port, MythUIButtonList *m_searchSettingsButtonList, MythUIType *m_searchSettingsGroup, Browser *m_browser);

    QString getKodiYTPlayUrl();
    QString getKodiYTPluginDomain();
    void setKodiYTProgramData(QString pd);

    QString getYTnativeProgramData();
    QString getKodiYTProgramData();

    QString getKodiYTPluginAppName();
    QString getAppIcon();

    void loadProgramList(QString searchText, QString directory);
    void searchSettingsClicked(MythUIButtonListItem *item);
    void setSearchSettingsGroupVisibilty(bool visible);
    bool openInBrowserIfRequired(QString filePathParam);

  private:
    QList<QVariant> getVideos(QString searchText, QString directory);
    QStringList getlist(int currentPos, bool foward);
    QString translateWordtoRFC3339Date(QString date);

    NetRequest *netRequest{nullptr};

    QString ytApi;
    QString ytID;
    QString kodiYTProgramData;
    QString kodiYTPluginDomain;
    QString kodiYTPluginIcon;
    MythUIType *searchSettingsGroup{nullptr};

    QString sortBy;
    QString videodDuration;
    QString type;
    QString eventType;
    bool useBrowserPlayer;

    MythUIButtonListItem *dateBetween;
    MythUIButtonListItem *datePublishedBefore;

    QString dateBetweenUrlBuilder;
    QString datePublishedBeforeUrlBuilder;

    QStringList masterDateListdateBetween;
    QStringList masterDateListPublishedBefore;
    Browser *browser{nullptr};
    QString converKodiYTURLtoBrowserUrl(QString url);
    QString getAPIBaseUrl();

  signals:
    void loadProgramSignal(QString name, QString setdata, QString thumbnailPath);
};
#endif
