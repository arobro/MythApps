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

// MythApps headers
#include "plugin_api.h"
#include "shared.h"

// MythTV headers
#include "libmyth/mythcontext.h"
#include "libmythui/mythmainwindow.h"

// Globals
const QString appPathName = "app://Videos/";
const QStringList videoFilters = {"*.mp4", "*.mkv", "*.avi", "*.mov", "*.flv", "*.wmv"};
int m_activeThumbnailTasks = 0;
const int m_maxConcurrentTasks = QThread::idealThreadCount();

Videos::Videos() : pluginName("Videos"), pluginIcon("ma_video.png") {}
Videos::~Videos() = default;

QString Videos::getPluginName() const { return pluginName; }

QString Videos::getPluginDisplayName() const { return pluginName; }

QString Videos::getPluginIcon() const { return pluginIcon; }

void Videos::setDialog(Dialog *d) { dialog = d; }

void Videos::setFileBrowserHistory(FileBrowserHistory *f) { fileBrowserHistory = f; }

void Videos::load(const QString filePath) {
    LOG(VB_GENERAL, LOG_DEBUG, "Videos::load()");
    QString cleanedFilePath = filePath;
    cleanedFilePath.remove(appPathName);

    m_toggleSearchVisibleCallback(false);

    if (cleanedFilePath == "") {
        loadDirectory(QDir::homePath() + "/Videos", /* recursive = */ false);
        loadVideos();
    } else {
        updateMediaListCallback(cleanedFilePath);
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
        QString payload = makeAppPayload(fullPath, /*play=*/false);
        QString title = sd;
        QString thumb;

        bool isDir = true;
        QString data = createProgramData(payload, QString(), thumb, isDir, QString());
        m_loadProgramCallback(title, data, thumb);
    }

    // Handle video files
    QDirIterator it(dir.absolutePath(), videoFilters, QDir::Files, recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);

    while (it.hasNext()) {
        it.next();
        const QString filePath = it.filePath();
        QFileInfo fi(filePath);

        // isDir should always be false for files discovered by QDirIterator + videoFilters
        bool isDir = false;
        generateMythTVThumbnailAsync(filePath, isDir);
    }

    if (m_activeThumbnailTasks > 0) {
        dialog->getLoader()->SetVisible(true);
    }
}

void Videos::setControls(Controls *c) { controls = c; }

void Videos::generateMythTVThumbnailAsync(const QString &videoFilePath, bool isDir) {
    QString previewDir = getGlobalPathPrefix() + "preview/";

    QDir dir(previewDir);
    if (!dir.exists()) {
        if (!dir.mkpath("."))
            return;
    }

    QFileInfo fi(videoFilePath);
    QString outputThumbnail = dir.filePath(fi.completeBaseName() + "_thumb.jpg");

    if (QFile::exists(outputThumbnail)) {
        handleThumbnailReady(videoFilePath, outputThumbnail, isDir);
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
            handleThumbnailReady(videoFilePath, outputThumbnail, isDir);
        } else {
            LOG(VB_GENERAL, LOG_DEBUG, "Thumbnail generation failed: " + videoFilePath);
        }
        process->deleteLater();
    });

    process->start("mythpreviewgen", args);
}

void Videos::handleThumbnailReady(const QString &videoFilePath, const QString &thumbnailPath, bool isDir) {
    QFileInfo fi(videoFilePath);
    QString payload = makeAppPayload(videoFilePath, true);
    QString title = fi.completeBaseName();
    QString thumbnail = QStringLiteral("file://%1").arg(thumbnailPath);

    QString data = createProgramData(payload, QString(), thumbnail, !isDir, QString());
    m_loadProgramCallback(title, data, thumbnail);
    m_activeThumbnailTasks--;

    if (m_activeThumbnailTasks == 0) {
        dialog->getLoader()->SetVisible(false);
    }
}

void Videos::updateMediaListCallback(const QString filePath) {
    LOG(VB_GENERAL, LOG_DEBUG, "updateMediaListCallback(): " + filePath);

    QJsonDocument doc = QJsonDocument::fromJson(filePath.toUtf8());
    if (!doc.isObject())
        return;

    QVariantMap map = doc.object().toVariantMap();
    bool play = map.value("play").toBool();
    QString path = map.value("filePath").toString();

    if (play) {
        internalPlay(path);
        m_GoBackCallback();
    } else {
        // fileBrowserHistory->append(appPathName + filePath, appPathName + filePath);
        loadDirectory(path, false);
    }

    fileBrowserHistory->debug();
}

void Videos::internalPlay(const QString url) {
    LOG(VB_GENERAL, LOG_DEBUG, "internalPlay()");
    GetMythMainWindow()->HandleMedia("Internal", url, "", "", "", "", 0, 0, "", 0min, "", "", false);
}

QString Videos::makeAppPayload(const QString &filePath, bool play) {
    QJsonObject payloadObj;
    payloadObj["filePath"] = filePath;
    payloadObj["play"] = play;

    QJsonDocument doc(payloadObj);
    return appPathName + QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}
