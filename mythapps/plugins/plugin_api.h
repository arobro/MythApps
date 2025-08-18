#ifndef PLUGIN_API_H
#define PLUGIN_API_H

// QT headers
#include <QObject>
#include <QString>
#include <functional>

// MythApps headers
#include "container.h"
#include "controls.h"
#include "dialog.h"
#include "programData.h"
#include "uiContext.h"

class PluginAPI : public QObject {
    Q_OBJECT
  public:
    using QObject::QObject;
    virtual ~PluginAPI() {}

    using LoadProgramCallback = std::function<void(const QString, const QString, const QString, MythUIButtonList *)>;
    using DisplayImageCallback = std::function<void(MythUIButtonListItem *item, MythUIButtonList *)>;

    using ToggleSearchVisibleCallback = std::function<void(bool)>;
    using GoBackCallback = std::function<void()>;
    using SetFocusWidgetCallback = std::function<void(MythUIType *)>;
    using SetPlay_KodiCallback = std::function<void(QString)>;
    using PlaybackInfoCallback = std::function<PlaybackInfo()>;
    using FullscreenCallback = std::function<void()>;

    virtual QString getPluginName() const { return ""; }
    virtual QString getPluginIcon() const { return ""; }
    virtual QString getPluginDisplayName() { return ""; }
    virtual bool getPluginStartPos() const { return true; }
    virtual void load(const QString = "", const QString = "") { return; }
    virtual void displayHomeScreenItems() { return; }

    virtual void setControls(Controls *c) { controls = c; }
    virtual void setDialog(Dialog *d) { dialog = d; }
    virtual void setUIContext(UIContext *uiC) { uiCtx = uiC; }

    virtual void setLoadProgramCallback(LoadProgramCallback cb) { m_loadProgramCallback = cb; }
    virtual void setDisplayImageCallback(DisplayImageCallback cb) { m_DisplayImageCallback = cb; }

    virtual void setToggleSearchVisibleCallback(ToggleSearchVisibleCallback cb) { m_toggleSearchVisibleCallback = cb; }
    virtual void setGoBackCallback(GoBackCallback cb) { m_GoBackCallback = cb; }
    virtual void setFocusWidgetCallback(SetFocusWidgetCallback cb) { m_SetFocusWidgetCallback = cb; }
    virtual void setPlay_KodiCallback(SetPlay_KodiCallback cb) { m_SetPlay_KodiCallback = cb; };
    virtual void setGoFullscreenCallback(FullscreenCallback cb) { m_fullscreenCallback = cb; };

    virtual void setPlaybackInfoCallback(PlaybackInfoCallback cb) { m_PlaybackInfoCallback = cb; }
    virtual void onTextMessageReceived(const QString &method, const QString &message) { return; }
    virtual QStringList getOptionsMenuItems(ProgramData *, const QString &) { return {}; }

    virtual bool menuCallback(const QString &, ProgramData *) { return false; }
    virtual void handleAction(const QString, ProgramData *) { return; }

    virtual QString hidePlugin() { return ""; }
    virtual void search(const QString &) { return; }
    virtual bool handleSuggestion(const QString) { return false; }

    virtual bool useBasicMenu() { return false; }
    virtual bool initializeUI(MythUIType *) { return true; }

    void setExitToMainMenuSleepTimer(QTimer *timer) { exitToMainMenuSleepTimer = timer; }

    virtual void SetFocusWidget(MythUIType *widget) {
        if (m_SetFocusWidgetCallback)
            m_SetFocusWidgetCallback(widget);
    }

    virtual void exitPlugin() { return; }

  private:
    SetFocusWidgetCallback m_SetFocusWidgetCallback;

  protected:
    LoadProgramCallback m_loadProgramCallback;
    DisplayImageCallback m_DisplayImageCallback;

    ToggleSearchVisibleCallback m_toggleSearchVisibleCallback;
    GoBackCallback m_GoBackCallback;
    SetPlay_KodiCallback m_SetPlay_KodiCallback;
    FullscreenCallback m_fullscreenCallback;

    Controls *controls{nullptr};
    Dialog *dialog{nullptr};
    UIContext *uiCtx{nullptr};

    QTimer *exitToMainMenuSleepTimer;

    PlaybackInfoCallback m_PlaybackInfoCallback;
};

#endif // PLUGIN_API_H
