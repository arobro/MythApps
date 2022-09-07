// MythTV headers
#include <libmyth/mythcontext.h>
#include <mythdbcon.h>
#include <mythdirs.h>
#include <mythmainwindow.h>
#include <mythuibuttonlist.h>
#include <mythuicheckbox.h>
#include <mythuitext.h>

// QT headers
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QJsonDocument>

#include "mythsettings.h"
#include "netRequest.h"
#include "shared.h"

#define LOC QString("MythSettings: ")
#define LOC_WARN QString("MythSettings, Warning: ")
#define LOC_ERR QString("MythSettings, Error: ")

// ---------------------------------------------------

class MythSettingsPriv {
  public:
};

// ---------------------------------------------------

MythSettings::MythSettings(MythScreenStack *parent, const QString &name) : MythScreenType(parent, name), m_priv(new MythSettingsPriv) {}

MythSettings::~MythSettings() = default;

bool MythSettings::Create() // _videoUrl,_seek
{
    // Load the theme for this screen
    bool foundtheme = LoadWindowFromXML("mythapps-settings.xml", "settings", this);

    if (!foundtheme)
        return false;

    bool err = false;

    UIUtilE::Assign(this, m_pageOne, "page1", &err);
    UIUtilE::Assign(this, m_pageTwo, "page2", &err);

    UIUtilE::Assign(this, m_settingUser, "User-Settings-Textedit", &err);
    UIUtilE::Assign(this, m_settingPassword, "Password-Settings-Textedit", &err);
    UIUtilE::Assign(this, m_settingIP, "IP-Settings-Textedit", &err);
    UIUtilE::Assign(this, m_settingPort, "Port-Settings-Textedit", &err);

    UIUtilE::Assign(this, m_settingAppPass, "AppPass-Settings-Textedit", &err);
    UIUtilE::Assign(this, m_settingWeb, "Web-Settings-Textedit", &err);
    UIUtilE::Assign(this, m_settingSuggestUrl, "SuggestUrl-Settings-Textedit", &err);

    UIUtilE::Assign(this, m_commands, "commands", &err);
    UIUtilE::Assign(this, m_cancelBtn, "cancel", &err);
    UIUtilE::Assign(this, m_saveBtn, "save", &err);
    UIUtilE::Assign(this, m_ResetSearchList, "ResetSearchList", &err);
    UIUtilE::Assign(this, m_ResetImages, "ResetImages", &err);

    UIUtilE::Assign(this, m_myVideoCheckbox, "MyVideo-Settings-Checkbox", &err);
    UIUtilE::Assign(this, m_closeKodiOnExitCheckbox, "CloseKodiOnExit-Settings-Checkbox", &err);
    UIUtilE::Assign(this, m_MuteCheckbox, "Mute-Settings-Checkbox", &err);
    UIUtilE::Assign(this, m_VolCheckbox, "Vol-Settings-Checkbox", &err);
    UIUtilE::Assign(this, m_RemoteCheckbox, "Remote-Settings-Checkbox", &err);
    UIUtilE::Assign(this, m_MusicCheckbox, "Music-Settings-Checkbox", &err);

    UIUtilE::Assign(this, m_YTapi, "YTapi-Settings-Textedit", &err);
    UIUtilE::Assign(this, m_YTid, "YTid-Settings-Textedit", &err);
    UIUtilE::Assign(this, m_YTcs, "YTcs-Settings-Textedit", &err);
    UIUtilE::Assign(this, m_searchSourceTextarea, "searchSource-Textarea", &err);

    UIUtilE::Assign(this, m_texteditHelp, "texteditHelp", &err);
    UIUtilE::Assign(this, m_shapeHelp, "shapeHelp", &err);

    UIUtilE::Assign(this, m_searchSourcesList, "apps", &err);

    connect(m_cancelBtn, SIGNAL(Clicked()), this, SLOT(button_cancell()));
    connect(m_saveBtn, SIGNAL(Clicked()), this, SLOT(button_save()));
    connect(m_ResetSearchList, SIGNAL(Clicked()), this, SLOT(button_ResetSearchList()));
    connect(m_ResetImages, SIGNAL(Clicked()), this, SLOT(button_ResetImageCache()));

    connect(m_searchSourcesList, SIGNAL(itemClicked(MythUIButtonListItem *)), this, SLOT(m_searchListCallback(MythUIButtonListItem *)));

    backendHostName = gCoreContext->GetMasterHostName();

    m_settingUser->SetText(gCoreContext->GetSetting("MythAppsusername"));
    m_settingPassword->SetText(gCoreContext->GetSetting("MythAppspassword"));
    m_settingIP->SetText(gCoreContext->GetSetting("MythAppsip"));
    m_settingPort->SetText(gCoreContext->GetSetting("MythAppsport"));

    m_settingAppPass->SetPassword(true);
    m_settingAppPass->SetText(ROT13(gCoreContext->GetSettingOnHost("MythAppsAppPass", backendHostName)));

    QString websites = gCoreContext->GetSettingOnHost("MythAppsweb", backendHostName);

    // parse and create websites list
    if (websites.contains("~")) {
        QStringList kodiWebList = websites.split("~");

        foreach (QString website, kodiWebList) {
            if (website.contains("|")) {
                allWebsites = allWebsites + website.split("|").at(0) + "~";
            }
        }
    }

    m_settingWeb->SetText(allWebsites);

    m_settingSuggestUrl->SetText(gCoreContext->GetSettingOnHost("MythAppsCustomSearchSuggestUrl", backendHostName));

    m_YTapi->SetText(gCoreContext->GetSettingOnHost("MythAppsYTapi", backendHostName));
    m_YTid->SetText(gCoreContext->GetSettingOnHost("MythAppsYTID", backendHostName));
    m_YTcs->SetText(gCoreContext->GetSettingOnHost("MythAppsYTCS", backendHostName));

    m_texteditHelp->SetVisible(false);
    m_shapeHelp->SetVisible(false);

    SetFocusWidget(m_settingUser);

    QStringList allSearchList = gCoreContext->GetSetting("MythAppsSearchList").split("~~");

    foreach (QString search, allSearchList) {
        if (!search.compare("") == 0) {
            if (!search.compare("") == 0) {
                auto *item = new MythUIButtonListItem(m_searchSourcesList, search);
                item->SetData(search);
                if (search.contains("!")) {
                    item->SetText("");
                    item->SetText(search.replace("!", ""), "buttontext2");
                }
            }
        }
    }

    if (gCoreContext->GetSetting("MythAppsmyVideo").compare("1") == 0) {
        m_myVideoCheckbox->SetCheckState(true);
    } else {
        m_myVideoCheckbox->SetCheckState(false);
    }

    if (gCoreContext->GetSetting("MythAppsCloseOnExit").compare("1") == 0) {
        m_closeKodiOnExitCheckbox->SetCheckState(true);
    } else {
        m_closeKodiOnExitCheckbox->SetCheckState(false);
    }

    if (gCoreContext->GetSetting("MythAppsInternalMute").compare("1") == 0) {
        m_MuteCheckbox->SetCheckState(true);
    } else {
        m_MuteCheckbox->SetCheckState(false);
    }
    if (gCoreContext->GetSetting("MythAppsInternalVol").compare("1") == 0) {
        m_VolCheckbox->SetCheckState(true);
    } else {
        m_VolCheckbox->SetCheckState(false);
    }
    if (gCoreContext->GetSetting("MythAppsInternalRemote").compare("1") == 0) {
        m_RemoteCheckbox->SetCheckState(true);
    } else {
        m_RemoteCheckbox->SetCheckState(false);
    }

    if (gCoreContext->GetSetting("MythAppsMusic").compare("1") == 0) {
        m_MusicCheckbox->SetCheckState(true);
    } else {
        m_MusicCheckbox->SetCheckState(false);
    }

    QString mCommand = "Install Status: ";

    if (system("command -v kodi >/dev/null 2>&1 || { exit 1; }") == 0) {
        mCommand = mCommand + tr("Kodi Installed,");
    } else {
        mCommand = mCommand + tr("Kodi not found,");
    }
    if (system("command -v nc >/dev/null 2>&1 || { exit 1; }") == 0) {
        mCommand = mCommand + tr(" nc Installed,");
    } else {
        mCommand = mCommand + tr(" nc not found,");
    }
    if (system("command -v kodi-send >/dev/null 2>&1 || { exit 1; }") == 0) {
        mCommand = mCommand + tr(" kodi-send Installed,");
    } else {
        mCommand = mCommand + tr(" kodi-send not found,");
    }
    if(isX11()){
		if (system("command -v xdotool >/dev/null 2>&1 || { exit 1; }") == 0) {
			mCommand = mCommand + tr(" xdotool Installed,");
		} else {
			mCommand = mCommand + tr(" xdotool not found,");
		}
	}
    if (system("command -v vnstat >/dev/null 2>&1 || { exit 1; }") == 0) {
        mCommand = mCommand + tr(" vnstat Installed");
    } else {
        mCommand = mCommand + tr(" vnstat not found");
    }

    m_commands->SetText(mCommand);

    if (err) {
        LOG(VB_GENERAL, LOG_ERR, "Cannot load screen 'config'");
        return false;
    }

    togglePage();

    return true;
}

void MythSettings::togglePage() {
    pageOne = !pageOne;

    m_pageOne->SetVisible(pageOne);
    m_pageOne->SetEnabled(pageOne);
    m_pageTwo->SetVisible(!pageOne);
    m_pageTwo->SetEnabled(!pageOne);

    if (pageOne) {
        m_cancelBtn->SetText("Cancel");
        m_saveBtn->SetText("Next");
    } else {
        m_cancelBtn->SetText("Back");
        m_saveBtn->SetText("Save");
    }
}

bool MythSettings::keyPressEvent(QKeyEvent *event) {
    if (GetFocusWidget() && GetFocusWidget()->keyPressEvent(event))
        return true;

    bool handled = false;
    QStringList actions;
    handled = GetMythMainWindow()->TranslateKeyPress("mythapps", event, actions);

    for (int i = 0; i < actions.size() && !handled; i++) {
        QString action = actions[i];
        handled = true;

        if (pageOne) {
            if (action == "ESCAPE") {
                Close();
            } else if (action == "HELP" || action == "Menu") {
                m_texteditHelp->SetVisible(!m_texteditHelp->IsVisible());
                m_shapeHelp->SetVisible(!m_shapeHelp->IsVisible());
            } else if ((action == "DOWN" || action == "RIGHT") and GetFocusWidget() == m_settingUser) {
                SetFocusWidget(m_settingPassword);
            } else if ((action == "DOWN" || action == "RIGHT") and GetFocusWidget() == m_settingPassword) {
                SetFocusWidget(m_settingIP);
            } else if ((action == "DOWN" || action == "RIGHT") and GetFocusWidget() == m_settingIP) {
                SetFocusWidget(m_settingPort);
            } else if ((action == "DOWN" || action == "RIGHT") and GetFocusWidget() == m_settingPort) {
                SetFocusWidget(m_settingAppPass);
            } else if ((action == "DOWN" || action == "RIGHT") and GetFocusWidget() == m_settingAppPass) {
                SetFocusWidget(m_settingWeb);
            } else if ((action == "DOWN" || action == "RIGHT") and GetFocusWidget() == m_settingWeb) {
                SetFocusWidget(m_myVideoCheckbox);
            } else if ((action == "DOWN" || action == "RIGHT") and GetFocusWidget() == m_myVideoCheckbox) {
                SetFocusWidget(m_MuteCheckbox);
            } else if ((action == "DOWN" || action == "RIGHT") and GetFocusWidget() == m_MuteCheckbox) {
                SetFocusWidget(m_closeKodiOnExitCheckbox);
            } else if ((action == "DOWN" || action == "RIGHT") and GetFocusWidget() == m_closeKodiOnExitCheckbox) {
                SetFocusWidget(m_VolCheckbox);
            } else if ((action == "DOWN" || action == "RIGHT") and GetFocusWidget() == m_VolCheckbox) {
                SetFocusWidget(m_RemoteCheckbox);
            } else if ((action == "DOWN" || action == "RIGHT") and GetFocusWidget() == m_RemoteCheckbox) {
                SetFocusWidget(m_MusicCheckbox);
            } else if ((action == "DOWN" || action == "RIGHT") and GetFocusWidget() == m_MusicCheckbox) {
                SetFocusWidget(m_cancelBtn);
            } else if ((action == "DOWN" || action == "RIGHT") and GetFocusWidget() == m_cancelBtn) {
                SetFocusWidget(m_saveBtn);
            } else if ((action == "DOWN" || action == "RIGHT") and GetFocusWidget() == m_saveBtn) {
                SetFocusWidget(m_settingUser);
            } else if ((action == "UP" || action == "LEFT") and GetFocusWidget() == m_settingUser) {
                SetFocusWidget(m_saveBtn);
            } else if ((action == "UP" || action == "LEFT") and GetFocusWidget() == m_saveBtn) {
                SetFocusWidget(m_cancelBtn);
            } else if ((action == "UP" || action == "LEFT") and GetFocusWidget() == m_cancelBtn) {
                SetFocusWidget(m_MusicCheckbox);
            } else if ((action == "UP" || action == "LEFT") and GetFocusWidget() == m_MusicCheckbox) {
                SetFocusWidget(m_RemoteCheckbox);
            } else if ((action == "UP" || action == "LEFT") and GetFocusWidget() == m_RemoteCheckbox) {
                SetFocusWidget(m_VolCheckbox);
            } else if ((action == "UP" || action == "LEFT") and GetFocusWidget() == m_VolCheckbox) {
                SetFocusWidget(m_closeKodiOnExitCheckbox);
            } else if ((action == "UP" || action == "LEFT") and GetFocusWidget() == m_closeKodiOnExitCheckbox) {
                SetFocusWidget(m_MuteCheckbox);
            } else if ((action == "UP" || action == "LEFT") and GetFocusWidget() == m_MuteCheckbox) {
                SetFocusWidget(m_myVideoCheckbox);
            } else if ((action == "UP" || action == "LEFT") and GetFocusWidget() == m_myVideoCheckbox) {
                SetFocusWidget(m_settingWeb);
            } else if ((action == "UP" || action == "LEFT") and GetFocusWidget() == m_settingWeb) {
                SetFocusWidget(m_settingAppPass);
            } else if ((action == "UP" || action == "LEFT") and GetFocusWidget() == m_settingAppPass) {
                SetFocusWidget(m_settingPort);
            } else if ((action == "UP" || action == "LEFT") and GetFocusWidget() == m_settingPort) {
                SetFocusWidget(m_settingIP);
            } else if ((action == "UP" || action == "LEFT") and GetFocusWidget() == m_settingIP) {
                SetFocusWidget(m_settingPassword);
            } else if ((action == "UP" || action == "LEFT") and GetFocusWidget() == m_settingPassword) {
                SetFocusWidget(m_settingUser);
            } else {
                handled = false;
            }
        } else { // page 2
            if (action == "ESCAPE") {
                Close();
            } else if (action == "LEFT" and GetFocusWidget() == m_saveBtn) {
                SetFocusWidget(m_cancelBtn);
            } else if ((action == "RIGHT" || action == "UP") and GetFocusWidget() == m_cancelBtn) {
                SetFocusWidget(m_saveBtn);
            } else if ((action == "RIGHT" || action == "UP") and GetFocusWidget() == m_saveBtn) {
                SetFocusWidget(m_searchSourcesList);
            } else if ((action == "RIGHT" || action == "UP") and GetFocusWidget() == m_searchSourcesList) {
                SetFocusWidget(m_ResetSearchList);
            } else if ((action == "RIGHT" || action == "UP") and GetFocusWidget() == m_ResetSearchList) {
                SetFocusWidget(m_cancelBtn);
            } else if ((action == "LEFT" || action == "DOWN") and GetFocusWidget() == m_ResetSearchList) {
                SetFocusWidget(m_searchSourcesList);
            } else if ((action == "LEFT" || action == "DOWN") and GetFocusWidget() == m_searchSourcesList) {
                SetFocusWidget(m_saveBtn);
            } else if ((action == "LEFT" || action == "DOWN") and GetFocusWidget() == m_saveBtn) {
                SetFocusWidget(m_cancelBtn);
            } else if ((action == "LEFT" || action == "DOWN") and GetFocusWidget() == m_cancelBtn) {
                SetFocusWidget(m_ResetSearchList);

            } else {
                handled = false;
            }
        }
    }

    if (!handled && MythScreenType::keyPressEvent(event))
        handled = true;

    return handled;
}

void MythSettings::button_cancell() {
    if (m_cancelBtn->GetText().compare("Back") == 0) {
        togglePage();
    } else {
        Close();
    }
}

void MythSettings::button_ResetImageCache() {
    m_ResetImages->SetVisible(false);
    QDir dir(GetConfDir() + "/MythApps/");
    dir.removeRecursively();
}

void MythSettings::button_ResetSearchList() { m_searchSourcesList->Reset(); }

QString MythSettings::savedWebSite(QString website) {
    NetRequest *netRequest = new NetRequest("", "", "", "", false);
    QString favIconUrl = netRequest->getFavIconUrl(website);

    return website + "|" + urlencode(favIconUrl) + "~";
}

void MythSettings::button_save() {
    if (m_saveBtn->GetText().compare("Save") == 0) {
        save();
    } else {
        togglePage();
    }
}

void MythSettings::save() {
    gCoreContext->SaveSetting("MythAppsmyVideo", m_myVideoCheckbox->GetBooleanCheckState());
    gCoreContext->SaveSetting("MythAppsCloseOnExit", m_closeKodiOnExitCheckbox->GetBooleanCheckState());

    gCoreContext->SaveSetting("MythAppsInternalMute", m_myVideoCheckbox->GetBooleanCheckState());
    gCoreContext->SaveSetting("MythAppsInternalVol", m_closeKodiOnExitCheckbox->GetBooleanCheckState());
    gCoreContext->SaveSetting("MythAppsInternalRemote", m_closeKodiOnExitCheckbox->GetBooleanCheckState());
    gCoreContext->SaveSetting("MythAppsMusic", m_MusicCheckbox->GetBooleanCheckState());

    gCoreContext->SaveSetting("MythAppsusername", m_settingUser->GetText());
    gCoreContext->SaveSetting("MythAppspassword", m_settingPassword->GetText());
    gCoreContext->SaveSetting("MythAppsip", m_settingIP->GetText());
    gCoreContext->SaveSetting("MythAppsport", m_settingPort->GetText());
    gCoreContext->SaveSettingOnHost("MythAppsAppPass", ROT13(m_settingAppPass->GetText()), backendHostName);

    QString savedWebsites = m_settingWeb->GetText();

    if (allWebsites.compare(savedWebsites) != 0) { // if change to website list, update the changes
        LOG(VB_GENERAL, LOG_DEBUG, "saving websites " + savedWebsites);
        QString allSavedWebsites = "";

        if (savedWebsites.contains("~")) {
            QStringList kodiWebList = savedWebsites.split("~");
            foreach (QString website, kodiWebList) {
                LOG(VB_GENERAL, LOG_DEBUG, "website " + website);
                allSavedWebsites = allSavedWebsites + savedWebSite(website);
            }
        } else {
            allSavedWebsites = savedWebSite(savedWebsites);
        }
        allSavedWebsites.replace("|~|~", "|~");
        allSavedWebsites.replace("~|~", "~");

        if (allSavedWebsites.compare("|~") == 0) {
            allSavedWebsites = "";
        }

        LOG(VB_GENERAL, LOG_DEBUG, "allSavedWebsites: " + allSavedWebsites);
        gCoreContext->SaveSettingOnHost("MythAppsweb", allSavedWebsites, backendHostName);
    }

    gCoreContext->SaveSettingOnHost("MythAppsCustomSearchSuggestUrl", m_settingSuggestUrl->GetText(), backendHostName);
    gCoreContext->SaveSettingOnHost("MythAppsYTapi", m_YTapi->GetText(), backendHostName);
    gCoreContext->SaveSettingOnHost("MythAppsYTID", m_YTid->GetText(), backendHostName);
    gCoreContext->SaveSettingOnHost("MythAppsYTCS", m_YTcs->GetText(), backendHostName);

    QString allList = "";
    for (int i = 0; i < m_searchSourcesList->GetCount(); i++) {
        allList = allList + m_searchSourcesList->GetItemAt(i)->GetData().toString() + QString("~~");
    }
    gCoreContext->SaveSetting("MythAppsSearchList", allList);

    if (m_YTapi->GetText().length() > 5) { // update the api config file if a value is entered
        QString apiYTJson = QString("{\"keys\":{\"developer\":{},\"personal\":{\"api_key\": \"") + m_YTapi->GetText() + QString("\",\"client_id\": \"") + m_YTid->GetText() +
                            QString("\",\"client_secret\":\"") + m_YTcs->GetText() + QString("\"}}}");

#ifdef _WIN32
        QString appfilePath(QDir().homePath() + "/AppData/Roaming/Kodi/userdata/addon_data/");
#else
        QString appfilePath(QDir().homePath() + "/.kodi/userdata/addon_data/");
#endif

        // search directories for the .json config file and update the api keys if found
        QDirIterator it(appfilePath, QStringList() << "api_keys.json", QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QFile jsonFile(it.next());
            jsonFile.remove();
            if (jsonFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                jsonFile.write(apiYTJson.toUtf8());
            }
            jsonFile.close();
            Close();
            break;
        }
    }
}

void MythSettings::m_searchListCallback(MythUIButtonListItem *item) {
    if (item->GetData().toString().contains("!")) {
        item->SetData(item->GetData().toString().replace("!", ""));

        item->SetText("", "buttontext2");
        item->SetText(item->GetData().toString());
    } else {
        item->SetText("");
        item->SetText(item->GetData().toString().replace("!", ""), "buttontext2");
        item->SetData("!" + item->GetData().toString());
    }
}
