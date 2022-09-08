// MythTV headers
#include <libmyth/mythcontext.h>
#include <libmythbase/mythdbcon.h>
#include <libmythui/mythmainwindow.h>
#include <libmythui/mythuibuttonlist.h>
#include <libmythui/mythuicheckbox.h>
#include <libmythui/mythuitext.h>

#include "mythosd.h"

#define LOC QString("MythOSD: ")
#define LOC_WARN QString("MythOSD, Warning: ")
#define LOC_ERR QString("MythOSD, Error: ")

#include <QThread>

MythOSD::MythOSD(MythScreenStack *parent, const QString &name) : MythScreenType(parent, name) {}

MythOSD::~MythOSD() = default;

bool MythOSD::Create() {
    // Load the theme for this screen
    bool foundtheme = LoadWindowFromXML("osd.xml", "osd_status", this);
    bool err = false;

    UIUtilE::Assign(this, m_title, "title", &err);
    UIUtilE::Assign(this, m_description, "description", &err);

    m_title->SetText("Pause");

    if (!foundtheme) {
        return false;
    }

    return true;
}

/** \brief handle key press events  */
bool MythOSD::keyPressEvent(QKeyEvent *event) {
    if (GetFocusWidget() && GetFocusWidget()->keyPressEvent(event)) {
        return true;
    }

    bool handled = false;
    QStringList actions;
    handled = GetMythMainWindow()->TranslateKeyPress("mythapps", event, actions);

    for (int i = 0; i < actions.size() && !handled; i++) {
        QString action = actions[i];
        handled = true;

        if (action == "ESCAPE") {
            keyEscape = true;
        } else if (action == "PLAY" || action == "PAUSE") {
            keyPlay = true;
        } else {
            handled = false;
        }
    }

    if (!handled && MythScreenType::keyPressEvent(event)) {
        handled = true;
    }

    return handled;
}

/** \brief set the playback time in the pause menu
 * 	\param playback time */
void MythOSD::setPlayBackTimeOSD(QString playbackTime) { m_description->SetText(playbackTime); }

/** \brief wait for the pause to be unpaused
 *  \return was escape key pressed? */
bool MythOSD::waitforKey() {
    while (!keyPlay) {

        QTime dieTime = QTime::currentTime().addMSecs(300);
        while (QTime::currentTime() < dieTime) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
            QThread::msleep(100);
        }

        if (keyEscape) {
            return false;
        }
    }
    return true;
}

/** \brief close the inputbox */
void MythOSD::closeWindow() { Close(); }
