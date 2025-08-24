#pragma once
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QTextStream>
#include <QVariant>

// Runtime toggle variables (external linkage)
extern QFile logFile;
extern QTextStream logStream;
extern QMutex logMutex;
extern bool logInitialized;

extern bool logToFile;
extern bool logDebug;

void initLogFile();

class FunctionLogger {
public:
    template <typename... Args>
    FunctionLogger(int level,
                   const char *funcName,
                   const char *fileName,
                   int lineNumber,
                   Args &&...args)
        : m_level(level),
          m_funcName(funcName),
          m_fileName(fileName),
          m_lineNumber(lineNumber),
          m_extraInfo(formatArgs(std::forward<Args>(args)...))
    {
        if (shouldLog()) {
            log(QStringLiteral("%1 START %2:%3 %4")
                .arg(m_funcName)
                .arg(m_fileName ? m_fileName : "")
                .arg(m_lineNumber)
                .arg(m_extraInfo));
        }
    }

    ~FunctionLogger() {
        if (shouldLog() && logDebug) {
            log(QStringLiteral("%1 END").arg(m_funcName));
        }
    }

private:
    bool shouldLog() const {
        switch (m_level) {
        case 0: return logDebug;
        case 1: return true;
        default: return false;
        }
    }

    void log(const QString &message) const;

    QString formatArgs() { return {}; }

    template <typename T>
    QString formatArgs(T &&single) {
        return QVariant(std::forward<T>(single)).toString();
    }

    template <typename Name, typename Value, typename... Rest>
    QString formatArgs(Name &&name, Value &&value, Rest &&...rest) {
        QString pair = QStringLiteral("%1:%2,")
                           .arg(QVariant(std::forward<Name>(name)).toString())
                           .arg(QVariant(std::forward<Value>(value)).toString());
        QString remaining = formatArgs(std::forward<Rest>(rest)...);
        return remaining.isEmpty() ? pair : pair + " " + remaining;
    }

    int m_level;
    const char *m_funcName;
    const char *m_fileName;
    int m_lineNumber;
    QString m_extraInfo;
};

// Scoped logging macro
#define LOGS(level, ...) \
    do { \
        FunctionLogger(level, __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)
    
//#define LOGS(level, ...) \
    do { (void)sizeof...( __VA_ARGS__ ); } while (0)

