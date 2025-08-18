#include "mythapps.h"

#include "SafeDelete.h"

// Not just a forward declaration!
#include "libmythui/mythuistatetracker.h"
#include <libmyth/mythcontext.h>
#include <libmythui/mythdialogbox.h>
#include <libmythui/mythmainwindow.h>
#include <libmythui/mythprogressdialog.h>
#include <libmythui/mythuibutton.h>
#include <libmythui/mythuiimage.h>
#include <libmythui/mythuitext.h>

void MythApps::CleanupResources() {
    SafeDelete(pluginManager);

    exitToMainMenuSleepTimer->disconnect();
    minimizeTimer->disconnect();
    isKodiConnectedTimer->disconnect();
    searchTimer->disconnect();
    searchSuggestTimer->disconnect();
    searchFocusTimer->disconnect();
    nextPageTimer->disconnect();
    openSettingTimer->disconnect();
    managerFileBrowser->disconnect();

    SafeDelete(controls);
    SafeDelete(previouslyPlayedLink);
    SafeDelete(searchListLink);
    SafeDelete(m_thumbnailImage);
    SafeDelete(currentSelectionDetails);
    SafeDelete(lastPlayedDetails);
    SafeDelete(browser);
    SafeDelete(fileBrowserHistory);
    SafeDelete(dialog);

    // uiCtx
    SafeDelete(uiCtx->filepath);
    SafeDelete(uiCtx->title);
    SafeDelete(uiCtx->fileListGrid);
    SafeDelete(uiCtx->screenshotMainMythImage);
    SafeDelete(uiCtx->SearchTextEdit);
    SafeDelete(uiCtx->searchButtonList);
    SafeDelete(uiCtx->androidMenuBtn);
    SafeDelete(uiCtx);

    currentLoadId.fetchAndAddOrdered(0);

    clearThreads();

    QList<ImageThread *> liveWorkers;
    for (ImageThread *worker : liveWorkers)
        worker->requestAbort();
}
