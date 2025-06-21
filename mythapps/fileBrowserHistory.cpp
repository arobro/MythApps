#include "fileBrowserHistory.h"

// MythTV headers
#include <libmyth/mythcontext.h>

// MythApps headers
#include "programData.h"

FileBrowserHistory::FileBrowserHistory() {}

/** \brief Removes the last url from the previousURL list. Used for the back button.  */
void FileBrowserHistory::removeCurrentUrlFromList() {
    if (previousListItem.size() > 0) {
        previousListItem.removeLast();
    }
}

/** \brief go to the previous url in the file browser history. */
void FileBrowserHistory::goBack() {
    LOG(VB_GENERAL, LOG_DEBUG, "FileBrowserHistory::goBack() Start");

    // foreach (QStringList previousList, previousListItem) {
    // LOG(VB_GENERAL, LOG_DEBUG, previousList.at(0) + " - " + previousList.at(1) );
    //}

    removeCurrentUrlFromList();

    LOG(VB_GENERAL, LOG_DEBUG, "FileBrowserHistory::goBack() Finished");
}

/** \brief append the current url in the file browser */
void FileBrowserHistory::append(QString label, QString data) {
    LOG(VB_GENERAL, LOG_DEBUG, "FileBrowserHistory:: Append()");

    ProgramData *programData = new ProgramData(label, data);
    if (!programData->isPlayRequest()) {
        QStringList previousList;
        previousList.append(label);
        previousList.append(data);

        previousListItem.append(previousList);
    }
    delete programData;
}

/** \brief is the file browser hisotry empty? */
bool FileBrowserHistory::isEmpty() {
    if (previousListItem.size() > 0) {
        return false;
    }
    return true;
}

/** \brief get the current label for the currrent url in the file browser */
QString FileBrowserHistory::getCurrentLabel() {
    QStringList previousList = previousListItem.at(previousListItem.size() - 1);
    return previousList.at(0);
}

/** \brief get the current data for the currrent url in the file browser */
QString FileBrowserHistory::getCurrentData() {
    QStringList previousList = previousListItem.at(previousListItem.size() - 1);
    return previousList.at(1);
}
