#ifndef Browser_h
#define Browser_h

// QT headers
#include <QObject>
#include <QString>

// MythApps headers
#include "controls.h"

class Browser : public QObject {
    Q_OBJECT

  public:
    Browser(Controls *m_controls);

    void openBrowser(QString website);
    void setOpenStatus(bool open);
    void updateBrowserOpenStatus();

    bool proccessRemote(QString action);

    void bringToFrontIfOpen();

  signals:
    void setFocusWidgetSignal(QString);

  private:
    bool browserOpen = false; /*!< is the web browser open? */
    Controls *controls;
    QString browserName = "";
};
#endif
