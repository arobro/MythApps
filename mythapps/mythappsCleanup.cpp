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
    exitToMainMenuSleepTimer->disconnect();
    minimizeTimer->disconnect();
    isKodiConnectedTimer->disconnect();
    playbackTimer->disconnect();
    searchTimer->disconnect();
    hintTimer->disconnect();
    musicBarOnStopTimer->disconnect();
    searchSuggestTimer->disconnect();
    searchFocusTimer->disconnect();
    nextPageTimer->disconnect();
    openSettingTimer->disconnect();
    managerFileBrowser->disconnect();

    SafeDelete(controls);
    SafeDelete(previouslyPlayedLink);
    SafeDelete(searchListLink);
    SafeDelete(m_thumbnailImage);
    SafeDelete(m_fileListMusicGrid);
    SafeDelete(m_fileListSongs);
    SafeDelete(m_filterGrid);
    SafeDelete(m_filterOptionsList);
    SafeDelete(m_playlistVertical);
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

    // Music app
    SafeDelete(m_textSong);
    SafeDelete(m_textArtist);
    SafeDelete(m_textAlbum);
    SafeDelete(m_musicDuration);
    SafeDelete(m_hint);
    SafeDelete(m_seekbar);
    SafeDelete(m_musicTitle);
    SafeDelete(m_trackProgress);
    SafeDelete(m_coverart);
    SafeDelete(m_blackhole_border);
    SafeDelete(m_playingOn);
    SafeDelete(m_next_buttonOn);
    SafeDelete(m_ff_buttonOn);
    SafeDelete(m_playingOff);
    SafeDelete(m_next_buttonOff);
    SafeDelete(m_ff_buttonOff);

    currentLoadId.fetchAndAddOrdered(0);

    clearThreads();

    QList<ImageThread *> liveWorkers;
    for (ImageThread *worker : liveWorkers)
        worker->requestAbort();
}
