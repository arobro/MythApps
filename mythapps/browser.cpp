#include "browser.h"

// MythApps headers
#include "shared.h"

Browser::Browser(Controls *m_controls) { controls = m_controls; }

/** \brief open the web browser
 * 	\param websiteUrl url of the website */
void Browser::openBrowser(QString website) {
    LOGS(0, "", "website", website);
    browserOpen = true;
#ifdef __ANDROID__
    if (!website.contains("http")) {
        website = "http://" + website;
    }
    controls->androidAppSwitch(website);
#elif _WIN32
    system("start msedge " + website.toLocal8Bit());
#else
    if (system("command -v chromium >/dev/null 2>&1 || { exit 1; }") == 0) {
        system("chromium -kiosk " + website.toLocal8Bit() + " &");
        browserName = "chromium";
    } else if (system("command -v firefox >/dev/null 2>&1 || { exit 1; }") == 0) {
        system("firefox --kiosk " + website.toLocal8Bit() + " &");
        browserName = "firefox";
    } else {
        system("sensible-browser" + website.toLocal8Bit() + " &");
    }
#endif
}

void Browser::setOpenStatus(bool open) {
    LOGS(0, "", "open", open);
    browserOpen = open;
}

/** \brief updates the status of browserOpen varaible and returns the focus if required */
void Browser::updateBrowserOpenStatus() {
    LOGS(0, "");
    if (browserOpen) {
        FILE *cmd = popen("pgrep -c --full 'chrome*.*kios*.*'", "r");
        char result[24] = {0x0};
        while (fgets(result, sizeof(result), cmd) != NULL) {
        }

        QString string(result);
        if (string.toInt() < 2) {
            browserOpen = false;
            emit setFocusWidgetSignal("m_searchButtonList");
        }
        pclose(cmd);
    }
}

/** \brief proccess keyboard / remote commands for the browser */
bool Browser::proccessRemote(QString action) {
    LOGS(0, "", "action", action);
    bool found = false;
#ifndef _WIN32
#ifndef __ANDROID__

    found = true;

    if (isGnomeWayland()) {
        if ((action == "BACK" || action == "ESCAPE") and browserOpen) { // browser key events
            activateWindowWayland(browserName);
            system("ydotool key 56:1 62:1 62:0 56:0"); // Alt+F4
        } else if ((action == "RIGHT") and browserOpen) {
            activateWindowWayland(browserName);
            system("ydotool key 15:1 15:0"); // tab
        } else if ((action == "PLAY" || action == "PAUSE") and browserOpen) {
            activateWindowWayland(browserName);
            system("ydotool key 57:1 57:0"); // space
        } else if ((action == "DETAILS" || action == "MENU" || action == "LEFT") and browserOpen) {
            activateWindowWayland(browserName);
            system("ydotool type F");
        } else if ((action == "UP") and browserOpen) {
            activateWindowWayland(browserName);
            system("ydotool key 104:1 104:0"); // page up
        } else if ((action == "DOWN") and browserOpen) {
            activateWindowWayland(browserName);
            system("ydotool key 109:1 109:0"); // page down
        } else if (browserOpen) {              // do nothing.

        } else {
            found = false;
        }
    } else {                                                            // x11
        if ((action == "BACK" || action == "ESCAPE") and browserOpen) { // browser key events
            system("xdotool search --class Chromium windowactivate --sync %1 key alt+F4 windowactivate $(xdotool getactivewindow)");
            setOpenStatus(false);
            emit setFocusWidgetSignal("m_fileListGrid");
        } else if ((action == "RIGHT") and browserOpen) {
            system("xdotool search --onlyvisible --classname chromium windowactivate --sync key Tab");
        } else if ((action == "PLAY" || action == "PAUSE") and browserOpen) {
            system("xdotool search --onlyvisible --classname chromium windowactivate --sync key space");
        } else if ((action == "DETAILS" || action == "MENU" || action == "LEFT") and browserOpen) {
            system("xdotool search --onlyvisible --classname chromium windowactivate --sync key F");
        } else if ((action == "UP") and browserOpen) {
            system("xdotool search --onlyvisible --classname chromium windowactivate --sync key Page_Up");
        } else if ((action == "DOWN") and browserOpen) {
            system("xdotool search --onlyvisible --classname chromium windowactivate --sync key Page_Down");
        } else if (browserOpen) { // do nothing.

        } else {
            found = false;
        }
    }
#endif
#endif
    return found;
}

/** \brief Bring the webbrowser to the front if open */
void Browser::bringToFrontIfOpen() {
    LOGS(0, "");
#ifndef _WIN32
#ifndef __ANDROID__
    if (browserOpen) {
        if (isGnomeWayland()) {
            activateWindowWayland(browserName);
        } else {
            system("xdotool search --onlyvisible --classname chromium windowactivate --sync key Return");
        }
    }
#endif
#endif
}
