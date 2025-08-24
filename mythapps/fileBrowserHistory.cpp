#include "fileBrowserHistory.h"

// MythTV headers
#include <libmyth/mythcontext.h>

// MythApps headers
#include "programData.h"

FileBrowserHistory::FileBrowserHistory() { LOGS(0, ""); }

/** \brief Removes the last url from the previousURL list. Used for the back button.  */
void FileBrowserHistory::removeCurrentUrlFromList() {
    LOGS(0, "");
    if (previousListItem.size() > 0)
        previousListItem.removeLast();
}

/** \brief go to the previous url in the file browser history. */
void FileBrowserHistory::goBack() {
    LOGS(0, "");
    removeCurrentUrlFromList();
}

void FileBrowserHistory::debug() {
    foreach (QStringList previousList, previousListItem) {
        LOG(VB_GENERAL, LOG_DEBUG, "FileBrowserHistory::debug(): " + previousList.at(1));
    }
}

/** \brief append the current url in the file browser */
void FileBrowserHistory::append(QString label, QString data) {
    LOGS(0, "", "label", label, "data", data);
    ProgramData programData(label, data);

    if (!programData.refreshGrid())
        return;

    if (!programData.isPlayRequest() || data.contains("app://")) {
        LOG(VB_GENERAL, LOG_DEBUG, "FileBrowserHistory:: Append(): " + data);
        QStringList previousList;
        previousList.append(label);
        previousList.append(data);

        previousListItem.append(previousList);
    } else {
        LOG(VB_GENERAL, LOG_DEBUG, "FileBrowserHistory:: Append(): no play file: " + data);
    }
}

/** \brief is the file browser hisotry empty? */
bool FileBrowserHistory::isEmpty() {
    LOGS(0, "");
    if (previousListItem.size() > 0) {
        return false;
    }
    return true;
}

/** \brief get the current label for the currrent url in the file browser */
QString FileBrowserHistory::getCurrentLabel() {
    LOGS(0, "");
    QStringList previousList = previousListItem.at(previousListItem.size() - 1);
    return previousList.at(0);
}

/** \brief get the current data for the currrent url in the file browser */
QString FileBrowserHistory::getCurrentData() {
    LOGS(0, "");
    if (previousListItem.isEmpty())
        return "";

    QStringList previousList = previousListItem.at(previousListItem.size() - 1);
    return previousList.at(1);
}

/** \brief Check if the current FileBrowser indicates an app is open */
bool FileBrowserHistory::isAppOpen() {
    LOGS(0, "");
    return getCurrentData().startsWith("app://");
}

bool FileBrowserHistory::isMusicAppOpen() { return getCurrentData().startsWith("app://Music"); }

QString FileBrowserHistory::getCurrentApp() {
    LOGS(0, "");
    if (isEmpty())
        return QString();

    QString currentData = getCurrentData();
    if (!currentData.startsWith("app://"))
        return QString();

    QStringList parts = currentData.split('/');
    if (parts.size() < 3)
        return QString();

    return parts[2];
}
