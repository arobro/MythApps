// MythTV headers
#include "mythuistatetracker.h"
#include <mythcontext.h>
#include <mythmainwindow.h>
#include <mythplugin.h>
#include <mythpluginapi.h>
#include <mythversion.h>

#include "mythapps.h"
#include "mythsettings.h"

static int RunMythApps(void) {
    MythScreenStack *mainStack = GetMythMainWindow()->GetMainStack();
    auto *mythapps = new MythApps(mainStack, "mythapps");

    if (mythapps->Create()) {
        mainStack->AddScreen(mythapps);
        return 0;
    }
    delete mythapps;
    return -1;
}

static int RunMythSettings(void) {
    MythScreenStack *mainStack = GetMythMainWindow()->GetMainStack();
    auto *mythsettings = new MythSettings(mainStack, "mythsettings");

    if (mythsettings->Create()) {
        mainStack->AddScreen(mythsettings);
        return 0;
    }
    delete mythsettings;
    return -1;
}

static void setupKeys(void) {
    REG_KEY("mythapps", "PLAY", QT_TRANSLATE_NOOP("MythControls", "Play"), "Ctrl+P");
    REG_KEY("mythapps", "PAUSE", QT_TRANSLATE_NOOP("MythControls", "Pause"), "P,Space");
    REG_KEY("mythapps", "STOP", QT_TRANSLATE_NOOP("MythControls", "Stop playback"), "O");
    REG_KEY("mythapps", "DETAILS", QT_TRANSLATE_NOOP("MythControls", "Show details"), "U");
    REG_KEY("mythapps", "MUTE", QT_TRANSLATE_NOOP("MythControls", "Mute"), "|,\\,F9,Volume Mute");
    REG_KEY("mythapps", "VOLUMEDOWN", QT_TRANSLATE_NOOP("MythControls", "Volume down"), "[,{,F10,Volume Down");
    REG_KEY("mythapps", "VOLUMEUP", QT_TRANSLATE_NOOP("MythControls", "Volume up"), "],},F11,Volume Up");
    REG_KEY("mythapps", "FFWD", QT_TRANSLATE_NOOP("MythControls", "Fast forward"), "PgUp");
    REG_KEY("mythapps", "RWND", QT_TRANSLATE_NOOP("MythControls", "Rewind"), "PgDown");
    REG_KEY("mythapps", "SEEKFFWD", QT_TRANSLATE_NOOP("MythControls", "Seek forward"), "Right");
    REG_KEY("mythapps", "SEEKRWND", QT_TRANSLATE_NOOP("MythControls", "Seek rewind"), "Left");
    REG_KEY("mythapps", "LEFT", QT_TRANSLATE_NOOP("MythControls", "Left"), "Left");
    REG_KEY("mythapps", "RIGHT", QT_TRANSLATE_NOOP("MythControls", "Right"), "Right");
    REG_KEY("mythapps", "NEXTTRACK", QT_TRANSLATE_NOOP("MythControls", "Move to the next track"), ">,.,Z,End");
    REG_KEY("mythapps", "PREVTRACK", QT_TRANSLATE_NOOP("MythControls", "Move to the previous track"), ",,<,Q,Home");
    REG_KEY("mythapps", "TOGGLERECORD", QT_TRANSLATE_NOOP("MythControls", "Add to Watch List for later viewing"), "R");
    REG_KEY("mythapps", "FULLSCREEN", QT_TRANSLATE_NOOP("MythControls", "Toggle Fullscreen"), "F2");
    REG_KEY("mythapps", "MINIMIZE", QT_TRANSLATE_NOOP("MythControls", "Toggle Auto Minimize"), "F3");
    REG_KEY("mythapps", "CLOSE", QT_TRANSLATE_NOOP("MythControls", "Force Close and Return to Main Menu"), "Pause,F4");
    REG_KEY("mythapps", "REFRESH", QT_TRANSLATE_NOOP("MythControls", "Refresh"), "F5");
    REG_KEY("mythapps", "TOGGLEHIDDEN", QT_TRANSLATE_NOOP("MythControls", "Toggle Hidden Folders"), "F6");
}

int mythplugin_init(const char *libversion) {
    if (!MythCoreContext::TestPluginVersion("mythapps", libversion, MYTH_BINARY_VERSION)) {
        return -1;
    }

    setupKeys();
    return 0;
}

/** \brief Creates the plugin */
int mythplugin_run(void) { return RunMythApps(); }

int mythplugin_config(void) { return RunMythSettings(); }
