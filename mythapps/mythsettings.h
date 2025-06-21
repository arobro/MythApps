// MythTV headers
#include <libmythui/mythscreentype.h>
#include <libmythui/mythuibutton.h>
#include <libmythui/mythuibuttonlist.h>
#include <libmythui/mythuicheckbox.h>
#include <libmythui/mythuishape.h>
#include <libmythui/mythuitextedit.h>

// QT
#include <QList>
#include <QPair>

class MythSettingsPriv;

class MythSettings : public MythScreenType {
    Q_OBJECT

  public:
    MythSettings(MythScreenStack *parent, const QString &name);
    ~MythSettings() override;

    bool Create() override;
    bool keyPressEvent(QKeyEvent *event) override;

  private:
    bool pageOne = false;
    MythUIType *m_pageOne;
    MythUIType *m_pageTwo;

    MythUITextEdit *m_settingUser;
    MythUITextEdit *m_settingPassword;
    MythUITextEdit *m_settingIP;
    MythUITextEdit *m_settingPort;

    MythUITextEdit *m_settingAppPass;
    MythUITextEdit *m_settingWeb;

    MythUITextEdit *m_settingSuggestUrl;
    MythUIShape *m_settingSuggestUrlShape;
    MythUIShape *m_settingSuggestUrlSettingsShape;
    MythUIText *m_settingSuggestUrlTextarea;

    MythUITextEdit *m_YTapi;
    MythUITextEdit *m_YTid;
    MythUITextEdit *m_YTcs;

    MythUIText *m_commands;

    MythUIText *m_texteditHelp;
    MythUIShape *m_shapeHelp;

    MythUICheckBox *m_myVideoCheckbox;
    MythUICheckBox *m_closeKodiOnExitCheckbox;
    MythUICheckBox *m_MuteCheckbox;
    MythUICheckBox *m_VolCheckbox;
    MythUICheckBox *m_RemoteCheckbox;
    MythUICheckBox *m_MusicCheckbox;

    MythUIButton *m_cancelBtn;
    MythUIButton *m_saveBtn;
    MythUIButton *m_ResetSearchList;
    MythUIButton *m_ResetImages;
    MythUIButtonList *m_searchSourcesList;

    MythUIText *m_searchSourceTextarea;

    QString allWebsites = "";

    QString savedWebSite(QString website);
    QString mCommand;

    QList<QPair<MythUICheckBox *, QString>> checkboxCollection;

    MythSettingsPriv *m_priv{nullptr};

    void togglePage();
    void save();
    void updateApikey(QString appfilePath);
    void checkProgramInstalled(QString programName, bool displayMessageIfInstalled);
    void setCheckboxFromSetting(MythUICheckBox *checkbox, const QString &settingName);
    void saveAllCheckboxSettings();

  private slots:
    void m_searchListCallback(MythUIButtonListItem *item);
    void button_cancell();
    void button_save();
    void button_ResetSearchList();
    void button_ResetImageCache();
};
