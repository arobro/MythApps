#include "mythsettings.h"

// QT headers
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QJsonDocument>

// MythTV headers
#include <libmyth/mythcontext.h>
#include <libmythbase/mythdbcon.h>
#include <libmythbase/mythdirs.h>
#include <libmythui/mythmainwindow.h>
#include <libmythui/mythuibuttonlist.h>
#include <libmythui/mythuicheckbox.h>
#include <libmythui/mythuitext.h>

// MythApps headers
#include "mythappsdbcheck.h"
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

    m_settingUser->SetText(gCoreContext->GetSetting("MythAppsusername"));
    m_settingPassword->SetText(gCoreContext->GetSetting("MythAppspassword"));
    m_settingIP->SetText(gCoreContext->GetSetting("MythAppsip"));
    m_settingPort->SetText(gCoreContext->GetSetting("MythAppsport"));

    m_settingAppPass->SetPassword(true);
    m_settingAppPass->SetText(ROT13(gCoreContext->GetSetting("MythAppsAppPass")));

    QString websites = gCoreContext->GetSetting("MythAppsweb");

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

    m_settingSuggestUrl->SetText(gCoreContext->GetSetting("MythAppsCustomSearchSuggestUrl"));

    m_YTapi->SetText(gCoreContext->GetSetting("MythAppsYTapi"));
    m_YTid->SetText(gCoreContext->GetSetting("MythAppsYTID"));
    m_YTcs->SetText(gCoreContext->GetSetting("MythAppsYTCS"));

    m_texteditHelp->SetVisible(false);
    m_shapeHelp->SetVisible(false);

    SetFocusWidget(m_settingUser);

    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare("SELECT url,enabled FROM mythconverg.mythapps_programlink where type = 'searchList'");

    if (!query.exec()) {
        MythDB::DBError(LOG_ERR + "Could not load sites from DB", query);
    }

    while (query.next()) {
        QString search = query.value(0).toString();
        bool enabled = query.value(1).toBool();

        auto *item = new MythUIButtonListItem(m_searchSourcesList, search);
        item->SetData(search);
        if (!enabled) {
            item->SetText("");
            item->SetText(search, "buttontext2");
        }
    }

    setCheckboxFromSetting(m_myVideoCheckbox, "MythAppsmyVideo");
    setCheckboxFromSetting(m_closeKodiOnExitCheckbox, "MythAppsCloseOnExit");
    setCheckboxFromSetting(m_MuteCheckbox, "MythAppsInternalMute");
    setCheckboxFromSetting(m_VolCheckbox, "MythAppsInternalVol");
    setCheckboxFromSetting(m_RemoteCheckbox, "MythAppsInternalRemote");
    setCheckboxFromSetting(m_MusicCheckbox, "MythAppsMusic");

    mCommand = "Install Status: ";

    appendProgramInstallStatus("Kodi", true);

    if (isX11()) {
        appendProgramInstallStatus("xdotool", true);
    } else if (isGnome()) {
        appendProgramInstallStatus("git", false);
        appendProgramInstallStatus("gsettings", false);

        if (QFile(QDir::homePath() + "/.local/share/gnome-shell/extensions/activate-window-by-title@lucaswerkmeister.de/extension.js").exists()) {
            system("gsettings set org.gnome.shell disable-user-extensions false");

            if (system("gnome-extensions show activate-window-by-title@lucaswerkmeister.de | grep -c ENABLED") == 0) {
                mCommand = mCommand + tr(" activate-window Installed,");
            } else {
                system("gnome-extensions enable activate-window-by-title@lucaswerkmeister.de");
                mCommand = mCommand + tr(" Logout to enable activate-window,");
            }
        } else {
            mCommand = mCommand + tr("gnome activate-window-by-title not installed,");

            system("mkdir -p ~/.local/share/gnome-shell/extensions/activate-window-by-title@lucaswerkmeister.de");
            system("cd ~/.local/share/gnome-shell/extensions/activate-window-by-title@lucaswerkmeister.de && "
                   "git clone https://github.com/lucaswerkmeister/activate-window-by-title . ");
        }
    } else {
        mCommand = mCommand + tr(" Please use Gnome or X11");
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
    if (auto current = GetFocusWidget()) {
        if (current->keyPressEvent(event))
            return true;
    }

    bool handled = false;
    QStringList actions;
    handled = GetMythMainWindow()->TranslateKeyPress("mythapps", event, actions);

    for (const QString &action : actions) { // Process each action key.
        if (action == "ESCAPE") {           // Global commands.
            Close();
            return true;
        }
        if (pageOne && (action == "HELP" || action == "Menu")) {
            m_texteditHelp->SetVisible(!m_texteditHelp->IsVisible());
            m_shapeHelp->SetVisible(!m_shapeHelp->IsVisible());
            return true;
        }

        if (action == "DOWN" || action == "RIGHT" || action == "UP" || action == "LEFT") { // Process directional navigation.
            bool forward = (action == "DOWN" || action == "RIGHT");

            QList<MythUIType *> order; // Define the navigation order for each page.
            if (pageOne) {
                order = {m_settingUser,    m_settingPassword, m_settingIP,       m_settingPort,  m_settingSuggestUrl,
                         m_settingAppPass, m_settingWeb,      m_myVideoCheckbox, m_MuteCheckbox, m_closeKodiOnExitCheckbox,
                         m_VolCheckbox,    m_RemoteCheckbox,  m_MusicCheckbox,   m_YTapi,        m_YTid,
                         m_YTcs,           m_cancelBtn,       m_saveBtn};

            } else { // Page two.
                order = {m_cancelBtn, m_saveBtn, m_searchSourcesList, m_ResetSearchList};
            }

            MythUIType *current = GetFocusWidget(); // Get current index and compute the new index cyclically.
            int idx = order.indexOf(current);
            if (idx != -1) {
                int n = order.size();
                int newIdx = forward ? (idx + 1) % n : (idx - 1 + n) % n;
                SetFocusWidget(order[newIdx]);
                return true;
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

void MythSettings::button_ResetSearchList() {
    m_searchSourcesList->Reset();
    MSqlQuery query(MSqlQuery::InitCon());
    query.prepare("DELETE FROM mythconverg.mythapps_programlink WHERE type = 'searchList'");
    if (!query.exec() || !query.isActive()) {
        MythDB::DBError("mythapps: delete searchlist from db", query);
    }
}

QString MythSettings::savedWebSite(QString website) {
    NetRequest *netRequest = new NetRequest("", "", "", "", false);
    QString favIconUrl = netRequest->getFavIconUrl(website);

    return website + "|" + urlEncode(favIconUrl) + "~";
}

void MythSettings::button_save() {
    if (m_saveBtn->GetText().compare("Save") == 0) {
        save();
        Close();
    } else {
        togglePage();
    }
}

void MythSettings::save() {
    saveAllCheckboxSettings();

    saveSetting("MythAppsusername", m_settingUser->GetText());
    saveSetting("MythAppspassword", m_settingPassword->GetText());
    saveSetting("MythAppsip", m_settingIP->GetText());
    saveSetting("MythAppsport", m_settingPort->GetText());
    saveSetting("MythAppsAppPass", ROT13(m_settingAppPass->GetText()));

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
        saveSetting("MythAppsweb", allSavedWebsites);
    }

    saveSetting("MythAppsCustomSearchSuggestUrl", m_settingSuggestUrl->GetText());
    saveSetting("MythAppsYTapi", m_YTapi->GetText());
    saveSetting("MythAppsYTID", m_YTid->GetText());
    saveSetting("MythAppsYTCS", m_YTcs->GetText());

    for (int i = 0; i < m_searchSourcesList->GetCount(); i++) {
        QString isEnabled;
        if (m_searchSourcesList->GetItemAt(i)->GetText("buttontext2").isEmpty()) {
            isEnabled = "1";
        } else {
            isEnabled = "0";
        }

        MSqlQuery query(MSqlQuery::InitCon());
        query.prepare("UPDATE mythapps_programlink SET enabled = :ENABLED WHERE type = 'MythAppsSearchList' AND url = :URL");
        query.bindValue(":ENABLED", isEnabled);
        query.bindValue(":URL", m_searchSourcesList->GetItemAt(i)->GetData().toString());

        if (!query.exec()) {
            MythDB::DBError(LOG_ERR + "Could not update SearchList in DB", query);
        }
    }

    if (m_YTapi->GetText().length() > 5) { // update the api config file if a value is entered
#ifdef _WIN32
        updateApikey(QDir().homePath() + "/AppData/Roaming/Kodi/userdata/addon_data/"));
#else
        updateApikey(QDir().homePath() + "/.kodi/userdata/addon_data/");
        updateApikey(QDir().homePath() + "/.var/app/tv.kodi.Kodi/data/userdata/addon_data/"); // flatpak
#endif
    }
}

void MythSettings::m_searchListCallback(MythUIButtonListItem *item) {
    if (item->GetText("buttontext2").isEmpty()) {
        item->SetText(item->GetData().toString(), "buttontext2");
        item->SetText("");
    } else {
        item->SetText("", "buttontext2");
        item->SetText(item->GetData().toString());
    }
}

/** \brief search directories for the .json config file and update the api keys if found */
void MythSettings::updateApikey(QString appfilePath) {
    QString apiYTJson = QString("{\"keys\":{\"developer\":{},\"personal\":{\"api_key\": \"") + m_YTapi->GetText() + QString("\",\"client_id\": \"") + m_YTid->GetText() +
                        QString("\",\"client_secret\":\"") + m_YTcs->GetText() + QString("\"}}}");

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

/** \brief Checks whether a specific program is installed on a Linux system and appends the status.
 * @param programName The name of the program to check.
 * @param displayMessageIfInstalled Whether to display a installed message if the program is found. */
void MythSettings::appendProgramInstallStatus(QString programName, bool displayMessageIfInstalled) {
#ifdef __linux__
    QString command = "command -v " + programName + " >/dev/null 2>&1 || { exit 1; }";

    if (system(command.toLatin1().constData()) == 0) {
        if (displayMessageIfInstalled) {
            mCommand += tr(" %1 Installed,").arg(programName);
        }
    } else {
        mCommand += tr(" %1 not found,").arg(programName);
    }
#endif
}

void MythSettings::setCheckboxFromSetting(MythUICheckBox *checkbox, const QString &settingName) {
    checkboxCollection.append(qMakePair(checkbox, settingName)); // Remember for saving later

    bool isChecked = (gCoreContext->GetSetting(settingName).compare("1") == 0);
    checkbox->SetCheckState(isChecked);
}

void MythSettings::saveAllCheckboxSettings() {
    for (const auto &entry : checkboxCollection) {
        if (entry.first) {
            saveSetting(entry.second, entry.first->GetBooleanCheckState());
        }
    }
}
