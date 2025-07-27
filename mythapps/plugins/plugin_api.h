#ifndef PLUGIN_API_H
#define PLUGIN_API_H

// QT headers
#include <QObject>
#include <QString>
#include <functional>

// MythApps headers
#include "controls.h"
#include "dialog.h"

class PluginAPI : public QObject {
    Q_OBJECT
  public:
    using QObject::QObject;
    virtual ~PluginAPI() {}

    using LoadProgramCallback = std::function<void(const QString &, const QString &, const QString &)>;
    using ToggleSearchVisibleCallback = std::function<void(bool)>;
    using GoBackCallback = std::function<void()>;

    virtual QString getPluginName() const = 0;
    virtual QString getPluginIcon() const = 0;
    virtual QString getPluginDisplayName() = 0;
    virtual bool getPluginStartPos() const = 0;
    virtual void load(const QString data = "") = 0;
    virtual void displayHomeScreenItems() = 0;

    virtual void setDialog(Dialog *d) = 0;

    virtual void setLoadProgramCallback(LoadProgramCallback cb) { m_loadProgramCallback = cb; }
    virtual void setToggleSearchVisibleCallback(ToggleSearchVisibleCallback cb) { m_toggleSearchVisibleCallback = cb; }
    virtual void setGoBackCallback(GoBackCallback cb) { m_GoBackCallback = cb; }

  protected:
    LoadProgramCallback m_loadProgramCallback;
    ToggleSearchVisibleCallback m_toggleSearchVisibleCallback;
    GoBackCallback m_GoBackCallback;
};

#endif // PLUGIN_API_H
