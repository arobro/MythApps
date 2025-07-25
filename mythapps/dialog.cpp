#include "dialog.h"

// MythTV headers
#include "dialog.h"
#include <libmyth/mythcontext.h>
#include <libmythui/mythdialogbox.h>
#include <libmythui/mythmainwindow.h>
#include <libmythui/mythprogressdialog.h>
#include <libmythui/mythscreentype.h>

// MythApp headers
#include "shared.h"

Dialog::Dialog(QObject *parent, MythUIImage *loaderImage) : QObject(parent), m_busyPopup(nullptr), m_loaderImage(loaderImage) {}

Dialog::~Dialog() {
    closeBusyDialog();
    m_loaderImage = nullptr;
}

/** \brief  creates a text dialog that auto closes
 *  \param  dialogText to display onscreen
 *  \param  delaySeconds seconds to autoclose */
void Dialog::createAutoClosingBusyDialog(QString dialogText, int delaySeconds) {
    createBusyDialog(dialogText);
    delay(delaySeconds);
    closeBusyDialog();
}

/** \brief create the busy dialog
 *  \param title lablel for the dialog */
void Dialog::createBusyDialog(const QString &title) {
    if (m_busyPopup)
        return;

    MythScreenStack *popupStack = GetMythMainWindow()->GetStack("popup stack");
    m_busyPopup = new MythUIBusyDialog(title, popupStack, "mythvideobusydialog");

    if (m_busyPopup->Create())
        popupStack->AddScreen(m_busyPopup, false);
    else
        m_busyPopup = nullptr;
}

/** \brief close the busy dialoge */
void Dialog::closeBusyDialog() {
    if (m_busyPopup) {
        m_busyPopup->Close();
        m_busyPopup = nullptr;
    }
}

MythUIImage *Dialog::getLoader() { return m_loaderImage; }
