#ifndef PLUGIN_API_H
#define PLUGIN_API_H

// QT headers
#include <QObject>
#include <QString>
#include <functional>

class PluginAPI : public QObject {
    Q_OBJECT
  public:
    using QObject::QObject;
    virtual ~PluginAPI() {}

    // Callback type: name, setdata, thumbnailPath
    using LoadProgramCallback = std::function<void(const QString &, const QString &, const QString &)>;
    using ToggleSearchVisibleCallback = std::function<void(bool)>;

    virtual QString getPluginName() const = 0;
    virtual QString getPluginIcon() const = 0;
    virtual QString getPluginDisplayName() const = 0;
    virtual void load() = 0;
    virtual void displayHomeScreenItems() = 0;

    virtual void setLoadProgramCallback(LoadProgramCallback cb) { m_loadProgramCallback = cb; }
    virtual void setToggleSearchVisibleCallback(ToggleSearchVisibleCallback cb) { m_toggleSearchVisibleCallback = cb; }

  protected:
    LoadProgramCallback m_loadProgramCallback;
    ToggleSearchVisibleCallback m_toggleSearchVisibleCallback;
};

#endif // PLUGIN_API_H