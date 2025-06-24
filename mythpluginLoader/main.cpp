#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <QApplication>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QKeyEvent>
#include <QString>
#include <QWidget>

#include "../../../mythtv/libs/libmythbase/cleanupguard.h"
#include "commandlineparser.h"
#include "compat.h"
#include "exitcodes.h"
#include "langsettings.h"
#include "mythcontext.h"
#include "mythcorecontext.h"
#include "mythdbcon.h"
#include "mythlogging.h"
#include "mythmainwindow.h"
#include "mythtranslation.h"
#include "mythuihelper.h"
#include "mythversion.h"
#include "signalhandling.h"

#include "../../../mythtv/libs/libmythui/mythdisplay.h"

#include "../mythapps/mythapps.h"

#define LOC QString("MythScreenWizard: ")
#define LOC_WARN QString("MythScreenWizard, Warning: ")
#define LOC_ERR QString("MythScreenWizard, Error: ")

namespace {
void cleanup() {
    DestroyMythMainWindow();

    delete gContext;
    gContext = nullptr;

    ReferenceCounter::PrintDebug();

    SignalHandler::Done();
}
} // namespace

static void startAppearWiz(int _x, int _y, int _w, int _h) {
    MythScreenStack *mainStack = GetMythMainWindow()->GetMainStack();
    auto *mythapps = new MythApps(mainStack, "mythapps");

    if (mythapps->Create()) {
        mainStack->AddScreen(mythapps);
        return;
    }
    delete mythapps;
}

int main(int argc, char **argv) {
    MythScreenWizardCommandLineParser cmdline;
    if (!cmdline.Parse(argc, argv)) {
        cmdline.PrintHelp();
        return GENERIC_EXIT_INVALID_CMDLINE;
    }

    if (cmdline.toBool("showhelp")) {
        cmdline.PrintHelp();
        return GENERIC_EXIT_OK;
    }

    if (cmdline.toBool("showversion")) {
        MythScreenWizardCommandLineParser::PrintVersion();
        return GENERIC_EXIT_OK;
    }

    MythDisplay::ConfigureQtGUI(1, cmdline);
    QApplication a(argc, argv);
    QCoreApplication::setApplicationName(MYTH_APPNAME_MYTHSCREENWIZARD);

    QString mask("general");
    int retval = cmdline.ConfigureLogging(mask, false);
    if (retval != GENERIC_EXIT_OK)
        return retval;

    CleanupGuard callCleanup(cleanup);

#ifndef _WIN32
    QList<int> signallist;
    signallist << SIGINT << SIGTERM << SIGSEGV << SIGABRT << SIGBUS << SIGFPE << SIGILL;
#if !CONFIG_DARWIN
    signallist << SIGRTMIN;
#endif
    SignalHandler::Init(signallist);
    signal(SIGHUP, SIG_IGN);
#endif

    if ((retval = cmdline.ConfigureLogging()) != GENERIC_EXIT_OK)
        return retval;

    gContext = new MythContext(MYTH_BINARY_VERSION);
    if (!gContext->Init(true, false, false)) {
        LOG(VB_GENERAL, LOG_ERR, LOC + "Failed to init MythContext, exiting.");
        return GENERIC_EXIT_NO_MYTHCONTEXT;
    }

    int GuiOffsetX = gCoreContext->GetNumSetting("GuiOffsetX", 0);
    int GuiOffsetY = gCoreContext->GetNumSetting("GuiOffsetY", 0);
    int GuiWidth = gCoreContext->GetNumSetting("GuiWidth", 0);
    int GuiHeight = gCoreContext->GetNumSetting("GuiHeight", 0);

    cmdline.ApplySettingsOverride();

    QString themename = gCoreContext->GetSetting("Theme", DEFAULT_UI_THEME);
    QString themedir = GetMythUI()->FindThemeDir(themename);
    if (themedir.isEmpty()) {
        LOG(VB_GENERAL, LOG_ERR, QString("Couldn't find theme '%1'").arg(themename));
        return GENERIC_EXIT_NO_THEME;
    }

    MythMainWindow *mainWindow = GetMythMainWindow();
    mainWindow->Init();
    mainWindow->setWindowTitle(QObject::tr("MythApps"));

    startAppearWiz(GuiOffsetX, GuiOffsetY, GuiWidth, GuiHeight);
    int exitCode = QCoreApplication::exec();

    return exitCode ? exitCode : GENERIC_EXIT_OK;
}
