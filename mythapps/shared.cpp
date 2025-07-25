// MythApps headers
#include "shared.h"

// C++ standard library
#include <sstream>
#include <string>

// QT headers
#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <QThreadPool>
#include <QtNetwork/QTcpSocket>

// MythTV headers
#include <libmyth/mythcontext.h>

/** \brief  helper function that converts parameters into an ugly delimited string. Todo: Should be either a class or list.
 * \param   file - can be a directory or a video url.
 * \param 	plot
 * \param   thumbnail
 * \param 	play - should this addon open and play the file?
 * \param 	seek - the time to start playing the video from.
 * \return  Delimited string of file, plot,thumbnail, play, seek. */
QString createProgramData(QString file, QString plot, QString thumbnail, bool play, QString seek) {
    QString seekString = "";
    if (seek.compare("") != 0) {
        seekString = "~" + seek;
    }

    QString playString = "";
    if (play) {
        playString = "~play";
    }
    return file + "~" + plot + "~" + thumbnail + playString + seekString;
}

/** \brief dealy for the x seconds
 * \param seconds */
void delay(int seconds) {
    QTime dieTime = QTime::currentTime().addSecs(seconds);
    while (QTime::currentTime() < dieTime) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        QThread::msleep(50);
    }
}

/** \brief dealy for the x mili-seconds
 * \param mili-seconds */
void delayMilli(int milliseconds) {
    QTime dieTime = QTime::currentTime().addMSecs(milliseconds);
    while (QTime::currentTime() < dieTime) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 25);
        QThread::msleep(25);
    }
}

/** \brief the sockets has no reponse, on older versions of Kodi, a delay was requried here.  */
void smartDelay() {}

/** \brief remove the plugin prefix from the url
 * \param url - url address
 * \return url without plugin prefix */
QString friendlyUrl(QString url) { return url.replace("plugin://", ""); }

void createDirectoryIfDoesNotExist(QString _dir) {
    QDir dir(_dir);
    if (!dir.exists())
        dir.mkpath(_dir);
}

/** \brief remove all bbcode
 * \param text - bbcode
 * \return text without bbcode */
QString removeBBCode(QString text) {
    bool remove = false;
    QString newString = "";

    for (auto chr : text) {
        if (chr == '[') {
            remove = true;
        } else if (chr == ']') {
            remove = false;
            newString = newString + ' ';
        } else if (!remove) {
            newString = newString + chr;
        }
    }
    return newString.simplified();
}

/** \brief  encode the url
 * \param   url the url
 * \return  encoded url */
QString urlEncode(QString url) {
    std::string s = url.toStdString();

    static const char lookup[] = "0123456789abcdef";
    std::stringstream e;
    for (int i = 0, ix = s.length(); i < ix; i++) {
        const char &c = s[i];
        if ((48 <= c && c <= 57) ||  // 0-9
            (65 <= c && c <= 90) ||  // abc...xyz
            (97 <= c && c <= 122) || // ABC...XYZ
            (c == '-' || c == '_' || c == '.' || c == '~')) {
            e << c;
        } else {
            e << '%';
            e << lookup[(c & 0xF0) >> 4];
            e << lookup[(c & 0x0F)];
        }
    }

    return QString::fromStdString(e.str());
}

/** \brief  decode the url
 * \param   url the url
 * \return  decoded url */
QString urlDecode(QString url) {
    std::string str = url.toStdString();
    std::string ret;
    char ch;
    int i, ii, len = str.length();

    for (i = 0; i < len; i++) {
        if (str[i] != '%') {
            if (str[i] == '+')
                ret += ' ';
            else
                ret += str[i];
        } else {
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            ret += ch;
            i = i + 2;
        }
    }
    return QString::fromStdString(ret);
}

bool QListContains(QList<QString> list, QString search) {
    if (QListContainsPos(list, search) == -1) {
        return false;
    } else {
        return true;
    }
}

int QListContainsPos(QList<QString> list, QString search) {
    int pos = 0;
    foreach (QString l, list) {
        if (l.contains(search)) {
            return pos;
        }
        pos++;
    }
    return -1;
}

QString QListSearch(QList<QString> list, QString search) {
    foreach (QString l, list) {
        if (l.contains(search) and !l.contains("!")) {
            return l;
        }
    }
    return QString("");
}

/** \brief can Kodi be pinged? */
bool isKodiPingable(QString ip, QString port) {
    QTcpSocket messenger;
    messenger.connectToHost(ip, port.toInt());

    if (!messenger.waitForConnected(100)) {
        return false;
    }
    return true;
}

/** \brief helper function to get the character from a set position for the rotate 13 function */
int getChar(QString text, int positon) {
    QChar c = text.at(positon);
    return c.toLatin1();
}

/** \brief rotate 13 for reversable plain text passwords in the database. */
QString ROT13(QString text) {
    QString out = "";
    for (int i = 0; i < text.length(); i++) { // Rot13 Algorithm
        int textI = getChar(text, i);

        if (textI < 91 && textI > 64) { // uppercase
            if (textI < 78) {
                out.append(QChar(textI + 13));
            } else {
                out.append(QChar(textI - 13));
            }
        } else if (textI < 123 && textI > 96) { // lowercase
            if (textI < 110) {
                out.append(QChar(textI + 13));
            } else {
                out.append(QChar(textI - 13));
            }
        } else {
            out.append(QChar(textI));
        }
    }
    return out;
}

QString removeTrailingChar(QString text, QChar characterToRemove) {
    while (text.endsWith(characterToRemove)) {
        text.chop(1);
    }
    return text;
}

/** \brief turn off az search for an app id*/
bool overrideAppAZSearch(QString appHash) {
    if (appHash.compare("a2898d49b4cbae54c0b0c4e91f6e9f4a") == 0) {
        return true;
    }
    return false;
}

/** \brief urls to turn off previously feature played on*/
bool excludePreviouslyPlayed(QString lastMediaLocation) {
    QStringList urls = gCoreContext->GetSetting("MythAppsPreviouslyPlayedExclude").split("~");

    foreach (QString url, urls) {
        if (lastMediaLocation.contains(url) and !url.isEmpty()) {
            return true;
        }
    }
    return false;
}

/** \brief is the settings dialog open in Kodi? */
bool isSettingsDialog(QString systemCurrentWindow) {
    if (systemCurrentWindow.compare("Add-on settings") == 0 || systemCurrentWindow.compare("Select dialog") == 0) {
        return true;
    }
    return false;
}

/** \brief get the website domain from the url
 * 	\param websiteUrl url of the website
 *  \return website domain or orginal url if can't find the domain */
QString getWebSiteDomain(QString websiteUrl) {
    QUrl qu(websiteUrl.trimmed());
    QUrlQuery q;
    q.setQuery(qu.query());
    QString vUrl = q.queryItemValue("v", QUrl::FullyDecoded);

    QString domain = qu.host();
    if (domain.isEmpty()) {
        domain = websiteUrl;
    }
    return domain;
}

/** \brief is x11 or wayland? */
bool isX11() {
    QString compositorType = QString::fromLocal8Bit(qgetenv("$XDG_SESSION_TYPE").constData());
    if (compositorType.compare("x11") == 0) {
        return true;
    }
    return false;
}

/** \brief is gnome? */
bool isGnome() {
    QString compositorType = QString::fromLocal8Bit(qgetenv("$XDG_CURRENT_DESKTOP").constData());
    if (compositorType.contains("GNOME", Qt::CaseInsensitive) == 0) {
        return true;
    }
    return false;
}

/** \brief is gnome? */
bool isGnomeWayland() {
    if (isGnome() && !isX11()) {
        return true;
    }
    return false;
}

/** \brief Activate the window. Wayland on Linux only.
 * \param windowName the name of the window to activate */
void activateWindowWayland(QString windowName) {
    system("gdbus call --session --dest org.gnome.Shell --object-path /de/lucaswerkmeister/ActivateWindowByTitle"
           " --method de.lucaswerkmeister.ActivateWindowByTitle.activateBySubstring '" +
           windowName.toLocal8Bit() + "'");
}

/** \brief Returns the full cached image path based of the original image location. Will copy the image to the cache if required. Used to copy mythapp
 * icons such as favourites to the cache directory.
 * \param imageFileName filename of the image
 * \return full cached image path with the image in the cache */
QString createImageCachePath(const QString imageFileName) {
    QString cachedPath = QString("%1%2").arg(GetShareDir()).arg("themes/default//" + imageFileName);
    if (QFile::exists(cachedPath)) {
        QFile::copy(cachedPath, getGlobalPathPrefix() + "/" + imageFileName);
    }
    return QString("file://") + getGlobalPathPrefix() + "/" + imageFileName;
}

QString getGlobalPathPrefix() {
    static QString globalPathprefix;

    if (globalPathprefix.isEmpty()) {
        globalPathprefix = GetConfDir() + "/MythApps/";
        createDirectoryIfDoesNotExist(globalPathprefix);
    }

    return globalPathprefix;
}

/** \brief Checks whether a specific program is installed on a Linux system.
 * @param programName The name of the program to check. */
bool checkIfProgramInstalled(const QString &programName) {
#ifdef __linux__
    QString command = "command -v " + programName + " >/dev/null 2>&1";
    return system(command.toLatin1().constData()) == 0;
#endif
    return false;
}

QString GetThemeXmlFile(const QString &theme) {
    if (theme == "Mythbuntu" || theme == "Willi" || theme == "Functionality" || theme == "Arclight" || theme == "Monochrome" || theme == "MythAeon") {
        return "mythapps-ui.xml";
    } else if (theme == "Steppes" || theme == "Steppes-large") {
        return "mythapps-ui.Steppes.xml";
    } else if (theme == "MythCenter-wide") {
        return "mythapps-ui.720.MCW.xml";
    } else if (theme.contains("XMAS")) {
        return "mythapps-ui.720.NoAlpha.xml";
    }
    return "mythapps-ui.720.xml";
}

QString formatTimeComponent(const QString &value) { return value.isNull() || value.isEmpty() ? "00" : value.rightJustified(2, '0'); }

/** \brief get the number of threads running. Also remove finished threads from the thread pool
 \return returns the number of threads running */
int MgetThreadCount() { return QThreadPool::globalInstance()->activeThreadCount(); }

/** \brief wait for threads to complete
 *  \param  maxThreadsRunning what is the max threads that should be running?*/
void waitForThreads(int maxThreadsRunning) {
    QThreadPool *pool = QThreadPool::globalInstance();

    while (pool->activeThreadCount() > maxThreadsRunning)
        QThread::msleep(50);
}

/** \brief clear running threads */
void clearThreads() { QThreadPool::globalInstance()->clear(); }

QString getKodiLogPath() {
    QStringList possiblePaths;
    QString newestLogPath;
    QDateTime newestTime;

#ifdef _WIN32
    QString userProfile = qEnvironmentVariable("USERPROFILE");
    possiblePaths << userProfile + "\\AppData\\Roaming\\Kodi\\kodi.log";
#else
    QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    possiblePaths << home + "/.kodi/temp/kodi.log";
    possiblePaths << home + "/.var/app/tv.kodi.Kodi/data/temp/kodi.log";
#endif

    for (const QString &path : possiblePaths) {
        QFileInfo fileInfo(path);
        if (fileInfo.exists() && fileInfo.isFile()) {
            if (newestLogPath.isEmpty() || fileInfo.lastModified() > newestTime) {
                newestLogPath = path;
                newestTime = fileInfo.lastModified();
            }
        }
    }

    return newestLogPath;
}
