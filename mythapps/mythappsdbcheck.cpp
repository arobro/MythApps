// C++ headers
#include <iostream>

// QT headers
#include <QSqlError>
#include <QString>

// Myth headers
#include <libmyth/mythcontext.h>
#include <libmythbase/mythdb.h>
#include <libmythbase/mythdbcheck.h>

// MythApps headers
#include "mythappsdbcheck.h"

const QString currentDatabaseVersion = "1000";
const QString MythAppsVersionName = "MythAppsDBSchemaVer";

/** \brief Creates a setting in the MythTv Database table if it doesn't exist
 *  \param settingName Setting Name
 *  \param settingValue Default setting value. Can be a string or boolean.
 *  \param remote Is the setting a backend or local setting*/
template <typename T> void createSetting(QString settingName, T settingValue, bool remote) {
    if (remote) {
        if (gCoreContext->GetSettingOnHost(settingName, gCoreContext->GetMasterHostName()).isEmpty()) {
            gCoreContext->SaveSettingOnHost(settingName, QString(settingValue), gCoreContext->GetMasterHostName());
        }
    } else {
        if (gCoreContext->GetSetting(settingName).isEmpty()) {
            gCoreContext->SaveSetting(settingName, settingValue);
        }
    }
}

/** \brief *updates tthe database schema*/
bool UpgradeMythAppsDatabaseSchema(void) {
    createSetting("MythAppsusername", "kodi", false);
    createSetting("MythAppspassword", "kodi", false);
    createSetting("MythAppsip", "127.0.0.1", false);
    createSetting("MythAppsport", "8080", false);
    createSetting("MythAppsmyVideo", true, false);
    createSetting("MythAppsCloseOnExit", true, false);
    createSetting("MythAppsInternalMute", true, false);
    createSetting("MythAppsInternalVol", true, false);
    createSetting("MythAppsInternalRemote", true, false);
    createSetting("MythAppsMusic", true, false);
    createSetting("MythAppsAppPass", "", true);
    createSetting("MythAppsYTapi", "", true);
    createSetting("MythAppsYTID", "", true);
    createSetting("MythAppsYTCS", "", true);
    createSetting("MythAppsCustomSearchSuggestUrl", "", true);
    createSetting("MythAppsPreviouslyPlayedExclude", "be/play/?video_id=~", true);
    createSetting("MythAppsShowsAZfolderNames", "TV Shows~", true);

    QString dbver = gCoreContext->GetSetting("MythAppsDBSchemaVer");

    if (dbver == currentDatabaseVersion) {
        return true;
    }

    if (dbver.isEmpty()) {
        LOG(VB_GENERAL, LOG_NOTICE, "Inserting MythApps initial database information.");

        DBUpdates updates{"CREATE TABLE `mythapps_programlink` ("
                          "	`id` INT NOT NULL AUTO_INCREMENT,"
                          "	`type` VARCHAR(25) NOT NULL DEFAULT '',"
                          "	`title` TEXT NOT NULL DEFAULT '',"
                          "	`url` TEXT NOT NULL DEFAULT '',"
                          "	`plot` TEXT NOT NULL DEFAULT '',"
                          "	`image` TEXT NOT NULL DEFAULT '',"
                          "	`autoPlay` BOOLEAN NOT NULL,"
                          "	`seek` VARCHAR(25) NOT NULL DEFAULT '',"
                          "	`pinnedToHome` BOOLEAN NOT NULL,"
                          "	`hostname` TEXT NOT NULL DEFAULT '',"
                          "	`enabled` BOOLEAN,"
                          "	PRIMARY KEY (`id`)"
                          ");	"};

        if (!performActualUpdate("MythApps", MythAppsVersionName, updates, "1000", dbver)) {
            return false;
        }
    }

    return true;
}
