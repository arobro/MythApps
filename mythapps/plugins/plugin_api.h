#ifndef PLUGIN_API_H
#define PLUGIN_API_H

// QT headers
#include <QObject>
#include <QString>
#include <functional>

// MythApps headers
#include "controls.h"
#include "dialog.h"
#include "programData.h"
#include "uiContext.h"

class PluginAPI : public QObject {
    Q_OBJECT
  public:
    using QObject::QObject;
    virtual ~PluginAPI() {}

    using LoadProgramCallback = std::function<void(const QString &, const QString &, const QString &)>;
    using ToggleSearchVisibleCallback = std::function<void(bool)>;
    using GoBackCallback = std::function<void()>;
    using SetFocusWidgetCallback = std::function<void(MythUIType *)>;
    using SetPlay_KodiCallback = std::function<void(QString)>;

    virtual QString getPluginName() const { return ""; }
    virtual QString getPluginIcon() const { return ""; }
    virtual QString getPluginDisplayName() { return ""; }
    virtual bool getPluginStartPos() const { return true; }
    virtual void load(const QString = "", const QString = "") { return; }
    virtual void displayHomeScreenItems() { return; }

    virtual void setDialog(Dialog *) { return; }
    virtual void setControls(Controls *) { return; }
    virtual void setUIContext(UIContext *) { return; }

    virtual void setLoadProgramCallback(LoadProgramCallback cb) { m_loadProgramCallback = cb; }
    virtual void setToggleSearchVisibleCallback(ToggleSearchVisibleCallback cb) { m_toggleSearchVisibleCallback = cb; }
    virtual void setGoBackCallback(GoBackCallback cb) { m_GoBackCallback = cb; }
    virtual void setFocusWidgetCallback(SetFocusWidgetCallback cb) { m_SetFocusWidgetCallback = cb; }
    virtual void setPlay_KodiCallback(SetPlay_KodiCallback cb) { m_SetPlay_KodiCallback = cb; };

    virtual QStringList getOptionsMenuItems(ProgramData *, const QString &) { return {}; }

    virtual bool menuCallback(const QString &, ProgramData *) { return false; }

    virtual void handleAction(const QString, ProgramData *) { return; }

    virtual QString hidePlugin() { return ""; }

    virtual void search(const QString &) { return; }
    virtual bool handleSuggestion(const QString) { return false; }

    virtual bool useBasicMenu() { return false; }

  protected:
    LoadProgramCallback m_loadProgramCallback;
    ToggleSearchVisibleCallback m_toggleSearchVisibleCallback;
    GoBackCallback m_GoBackCallback;
    SetFocusWidgetCallback m_SetFocusWidgetCallback;
    SetPlay_KodiCallback m_SetPlay_KodiCallback;
};

#endif // PLUGIN_API_H
