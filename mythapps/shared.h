#ifndef shared_h
#define shared_h

// QT headers
#include <QApplication>
#include <QJsonObject>
#include <QString>
#include <QThread>
#include <QUrlQuery>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

// MythTV headers
#include "FunctionLogger.h"
#include <libmythbase/mythdirs.h>

/** \class Shared
 *  \brief Contains shared functions or static functions */
QString createProgramData(const QString &file, const QString &plot, const QString &thumbnail, bool play, const QString &seek);

void delay(int seconds);
void delayMilli(int milliseconds);

QString friendlyUrl(QString url);

void createDirectoryIfDoesNotExist(QString _dir);
QString removeBBCode(QString text);

QString urlEncode(QString url);
QString urlDecode(QString url);

bool QListContains(QList<QString> list, QString search);
int QListContainsPos(QList<QString> list, QString search);

QString QListSearch(QList<QString> list, QString search);

bool isKodiPingable(QString ip, QString port);

int getChar(QString text, int positon);
QString ROT13(QString text);

QString removeTrailingChar(QString text, QChar characterToRemove);

bool overrideAppAZSearch(QString appHash);
bool excludePreviouslyPlayed(QString lastMediaLocation);

bool isSettingsDialog(QString systemCurrentWindow);

QString getWebSiteDomain(QString websiteUrl);
bool isX11();
bool isGnome();
bool isGnomeWayland();

void activateWindowWayland(QString windowName);

QString getGlobalPathPrefix();
QString createImageCachePath(const QString imageFileName);

bool checkIfProgramInstalled(const QString &programName);

QString GetThemeXmlFile(const QString &theme);

int getThreadCount();
void waitForThreads(int maxThreadsRunning);
void clearThreads();

QString getKodiLogPath();
void smartDelay();

QString formatTime(qint64 totalMs);
QString removeHoursIfZero(const QString &time);

void systemSafe(const char *command);

template <typename T> void setWidgetVisibility(T *widget, bool visible) {
    widget->SetVisible(visible);
    widget->SetEnabled(visible);
}

#endif
