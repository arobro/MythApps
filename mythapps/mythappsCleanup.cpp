#include "mythapps.h"
#include <type_traits>

// Not just a forward declaration!
#include "libmythui/mythuistatetracker.h"
#include <libmyth/mythcontext.h>
#include <libmythui/mythdialogbox.h>
#include <libmythui/mythmainwindow.h>
#include <libmythui/mythprogressdialog.h>
#include <libmythui/mythuibutton.h>
#include <libmythui/mythuiimage.h>
#include <libmythui/mythuitext.h>

template <typename T> void SafeDelete(T *&ptr) {
    if constexpr (std::is_base_of_v<QObject, T>) {
        if (ptr && !ptr->parent()) {
            delete ptr;
            ptr = nullptr;
        }
    } else {
        delete ptr;
        ptr = nullptr;
    }
}

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
    SafeDelete(m_filepath);
    SafeDelete(m_plot);
    SafeDelete(m_streamDetails);
    SafeDelete(m_streamDetailsbackground);
    SafeDelete(m_title);
    SafeDelete(m_SearchTextEdit);
    SafeDelete(m_screenshotMainMythImage);
    SafeDelete(m_loaderImage);
    SafeDelete(previouslyPlayedLink);
    SafeDelete(favLink);
    SafeDelete(watchedLink);
    SafeDelete(searchListLink);
    SafeDelete(m_thumbnailImage);
    SafeDelete(m_fileListGrid);
    SafeDelete(m_searchButtonList);
    SafeDelete(m_searchSettingsButtonList);
    SafeDelete(m_fileListMusicGrid);
    SafeDelete(m_fileListSongs);
    SafeDelete(m_filterGrid);
    SafeDelete(m_filterOptionsList);
    SafeDelete(m_playlistVertical);
    SafeDelete(m_androidMenuBtn);
    SafeDelete(currentselectionDetails);
    SafeDelete(lastPlayedDetails);
    SafeDelete(browser);
    SafeDelete(fileBrowserHistory);

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
    SafeDelete(ytNative);

    // Clean up QThreads in imageThreadList
    for (QThread *thread : imageThreadList) {
        thread->quit();
        if (!thread->wait(2000)) {
            thread->terminate(); // last resort
            thread->wait();
        }
        delete thread;
    }
    imageThreadList.clear();
}
