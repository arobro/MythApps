#include "videos.h"

// QT headers
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QJsonDocument>
#include <QProcess>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

// MythTV headers
#include "libmyth/mythcontext.h"
#include "libmythui/mythmainwindow.h"

// MythApps headers
#include "plugin_api.h"
#include "programData.h"
#include "shared.h"

// Globals
const QString appPathName = "app://Videos/";
const QStringList videoFilters = {"*.mp4", "*.mkv", "*.avi", "*.mov", "*.flv", "*.wmv", "*.ts"};
int m_activeThumbnailTasks = 0;
const int m_maxConcurrentTasks = QThread::idealThreadCount();

Videos::Videos() : pluginName("Videos"), pluginIcon("ma_video.png") { videos_icon = createImageCachePath("ma_video.png"); }

Videos::~Videos() = default;

QString Videos::getPluginName() const { return pluginName; }

QString Videos::getPluginDisplayName() { return pluginName; }

QString Videos::getPluginIcon() const { return pluginIcon; }

bool Videos::getPluginStartPos() const { return false; }

void Videos::setDialog(Dialog *d) { dialog = d; }

void Videos::load(const QString data) {
    m_toggleSearchVisibleCallback(false);

    if (data.length() < 2) {
        loadDirectory(QDir::homePath() + "/Videos", /* recursive = */ false);
        loadVideos();
    } else {
        updateMediaListCallback(data);
    }
}

void Videos::displayHomeScreenItems() { return; }

void Videos::loadVideos() {
    QVariantMap vids = controls->getVideos();
    QVariantList sources = vids.value("sources").toList();

    for (const QVariant &entry : sources) {
        QVariantMap map = entry.toMap();
        QString title = map.value("label").toString();
        QString url = map.value("file").toString();
        QString image = map.value("image").toString();

        if (image == "")
            image = videos_icon;

        QString data = createProgramData(url, "", image, false, "");
        m_loadProgramCallback(title, data, image);
    }
}

void Videos::loadDirectory(const QString &folderPath, bool recursive) {
    QDir dir(folderPath);
    if (!dir.exists()) {
        LOG(VB_GENERAL, LOG_WARNING, "Videos::loadDirectory: invalid path: " + folderPath);
        return;
    }

    // Handle subdirectories
    auto subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &sd : subdirs) {
        QString fullPath = dir.absoluteFilePath(sd);
        QString title = sd;

        QString data = createProgramData(fullPath, QString(), videos_icon, false, QString());
        m_loadProgramCallback(title, appPathName + data, videos_icon);
    }

    // Handle video files
    QDirIterator it(dir.absolutePath(), videoFilters, QDir::Files, recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);

    while (it.hasNext()) {
        it.next();
        const QString filePath = it.filePath();
        QFileInfo fi(filePath);

        bool isDir = false;
        generateMythTVThumbnailAsync(filePath, true);
    }

    if (m_activeThumbnailTasks > 0) {
        dialog->getLoader()->SetVisible(true);
    }
}

void Videos::setControls(Controls *c) { controls = c; }

void Videos::generateMythTVThumbnailAsync(const QString &videoFilePath, bool isPlay) {
    QString previewDir = getGlobalPathPrefix() + "preview/";

    QDir dir(previewDir);
    if (!dir.exists()) {
        if (!dir.mkpath("."))
            return;
    }

    QFileInfo fi(videoFilePath);
    QString outputThumbnail = dir.filePath(fi.completeBaseName() + "_thumb.jpg");

    if (QFile::exists(outputThumbnail)) {
        handleThumbnailReady(videoFilePath, outputThumbnail, isPlay);
        return;
    }

    while (m_activeThumbnailTasks >= m_maxConcurrentTasks) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        QThread::msleep(50);
    }
    m_activeThumbnailTasks++;

    QStringList args = {"--infile", videoFilePath, "--outfile", outputThumbnail, "--seconds", "5"};
    QProcess *process = new QProcess(this);

    connect(process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, [=](int exitCode, QProcess::ExitStatus exitStatus) {
        if (exitStatus == QProcess::NormalExit && QFile::exists(outputThumbnail)) {
            handleThumbnailReady(videoFilePath, outputThumbnail, isPlay);
        } else {
            LOG(VB_GENERAL, LOG_DEBUG, "Thumbnail generation failed: " + videoFilePath);
        }
        process->deleteLater();
    });

    process->start("mythpreviewgen", args);
}

void Videos::handleThumbnailReady(const QString &videoFilePath, const QString &thumbnailPath, bool isPlay) {
    QFileInfo fi(videoFilePath);
    QString title = fi.completeBaseName();
    QString thumbnail = QStringLiteral("file://%1").arg(thumbnailPath);

    QString data = createProgramData(videoFilePath, QString(), thumbnail, isPlay, QString());
    m_loadProgramCallback(title, appPathName + data, thumbnail);
    m_activeThumbnailTasks--;

    if (m_activeThumbnailTasks == 0) {
        dialog->getLoader()->SetVisible(false);
    }
}

void Videos::updateMediaListCallback(const QString &data) {
    LOG(VB_GENERAL, LOG_DEBUG, "updateMediaListCallback(): " + data);

    ProgramData programData("", data);
    QString path = programData.getFilePathParam();

    if (programData.isPlayRequest()) {
        internalPlay(path);
        m_GoBackCallback();
    } else {
        loadDirectory(path, false);
    }
}

void Videos::internalPlay(const QString url) {
    LOG(VB_GENERAL, LOG_DEBUG, "internalPlay()");
    QCoreApplication::processEvents();
    GetMythMainWindow()->HandleMedia("Internal", url, "", "", "", "", 0, 0, "", 0min, "", "", false);
    QCoreApplication::processEvents();
}
