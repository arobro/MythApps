#include "FunctionLogger.h"

// MythTV headers
#include "libmythbase/mythlogging.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

QFile logFile;
QTextStream logStream;
QMutex logMutex;
bool logInitialized = false;

bool logToFile = false;
bool logDebug = VERBOSE_LEVEL_CHECK(VB_GENERAL, LOG_DEBUG); // VERBOSE_LEVEL_CHECK(VB_GENERAL, LOG_DEBUG);, true, false

void initLogFile() {
    if (!logToFile)
        return;

    QMutexLocker locker(&logMutex);

    if (!logInitialized) {
        QString logDirPath = QDir::homePath() + "/.mythtv/MythApps";
        QDir dir;
        if (!dir.exists(logDirPath))
            dir.mkpath(logDirPath);

        logFile.setFileName(logDirPath + "/00mythapps.log");
        if (!logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << "Failed to open log file:" << logFile.fileName();
            return;
        }

        logStream.setDevice(&logFile);
        logInitialized = true;
    }
}

void FunctionLogger::log(const QString &message) const {
    if (logToFile) {
        initLogFile();
        if (logInitialized) {
            QMutexLocker locker(&logMutex);
            logStream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") << " - " << message << "\n";
            logStream.flush();
        }
    }

    switch (m_level) {
    case 0:
        if (logDebug)
            qDebug().noquote() << message;
        break;
    case 1:
        qDebug().noquote() << message;
        break;
    default:
        break;
    }
}
