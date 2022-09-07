#include "mythinput.h"
#include "shared.h"

// MythTV headers
#include <libmyth/mythcontext.h>
#include <mythdbcon.h>
#include <mythmainwindow.h>
#include <mythuibuttonlist.h>
#include <mythuicheckbox.h>
#include <mythuitext.h>

#include <QThread>

MythInput::MythInput(MythScreenStack *parent, const QString &name) : MythScreenType(parent, name) {}

MythInput::~MythInput() {}

bool MythInput::Create() {
    bool foundtheme = LoadWindowFromXML("mythapps-input.xml", "osd_input",
                                        this); // Load the theme for this screen
    bool err = false;

    UIUtilE::Assign(this, m_title, "title", &err);
    UIUtilE::Assign(this, m_TextEditValue, "TextEditValue", &err);
    UIUtilE::Assign(this, m_okBtn, "okButton", &err);

    connect(m_okBtn, SIGNAL(Clicked()), this, SLOT(button_ok()));

    SetFocusWidget(m_TextEditValue);

    if (!foundtheme)
        return false;

    return true;
}

/** \brief handle key press events  */
bool MythInput::keyPressEvent(QKeyEvent *event) {
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
            keyOk = true;
            cancel = true;
        } else if ((action == "UP" || action == "DOWN") and GetFocusWidget() == m_TextEditValue) {
            SetFocusWidget(m_okBtn);
        } else if ((action == "UP" || action == "DOWN" || action == "LEFT" || action == "RIGHT") and GetFocusWidget() == m_okBtn) {
            SetFocusWidget(m_TextEditValue);
        } else {
            handled = false;
        }
    }

    if (!handled && MythScreenType::keyPressEvent(event)) {
        handled = true;
    }

    return handled;
}

/** \brief set the title
 * 	\param title title of the inputbox */
void MythInput::setTitle(QString title) { m_title->SetText(title); }

/** \brief set the inputbox text value
 * 	\param value value of the textbox */
void MythInput::setTextValue(QString value) {
    if (!passwordActive) {
        m_TextEditValue->SetText(value);
    }
    if (!value.compare("") == 0) { // set to ok button if pre-populated with text
        SetFocusWidget(m_okBtn);
    }
}

/** \brief set the type and autopopulate the password if required?
 * 	\param type is a password?
 *  \return contains password */
bool MythInput::setType(QString type) {
    bool containsPassword = false;
    if (type.compare("password") == 0) {
        m_TextEditValue->SetPassword(true);
        QString pass = ROT13(gCoreContext->GetSettingOnHost("MythAppsAppPass", gCoreContext->GetMasterHostName()));
        if (!pass.compare("") == 0) {
            m_TextEditValue->SetText(pass);
            passwordActive = true;
            containsPassword = true;
            SetFocusWidget(m_okBtn);
        }
        pass = "";
    }
    return containsPassword;
}

QString MythInput::getTextValue() { return m_TextEditValue->GetText(); }

/** \brief wait for the text to be entered
 *  \return was the text entered? */
bool MythInput::waitforKey() {
    while (!keyOk) {

        QTime dieTime = QTime::currentTime().addMSecs(300);
        while (QTime::currentTime() < dieTime) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
            QThread::msleep(100);
        }

        if (keyOk) {
            return false;
        }
    }
    return true;
}

/** \brief is the input box cancelled?
 *  \return was the input box cancelled? */
bool MythInput::isCancelled() { return cancel; };

/** \brief close the inputbox */
void MythInput::closeWindow() { Close(); }

/** \brief set the ok button as pressed */
void MythInput::button_ok() { keyOk = true; }
