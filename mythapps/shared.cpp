#include "shared.h"
#include <QDir>
#include <libmyth/mythcontext.h>
#include <sstream>

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
QString urlencode(QString url) {
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

bool isKodiPingable(QString ip, QString port) {
#ifdef __ANDROID__
    return true;
#endif

#ifdef _WIN32
    int ping = system("powershell -command \"(New-Object System.Net.Sockets.TcpClient(" + ip.toLocal8Bit() + ", " + port.toLocal8Bit() + ") | Select -ExpandProperty Connected)\"");
#else
    int ping = system("nc -vz " + ip.toLocal8Bit() + " " + port.toLocal8Bit() + " > /dev/null 2>&1");
#endif
    if (!ping == 0) {
        return false;
    } else {
        return true;
    }
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
    QStringList urls = gCoreContext->GetSettingOnHost("MythAppsPreviouslyPlayedExclude", gCoreContext->GetMasterHostName()).split("~");

    foreach (QString url, urls) {
        if (lastMediaLocation.contains(url) and !url.isEmpty()) {
            return true;
        }
    }
    return false;
}

/** \brief get the parameter from a deliminated string by position.
 *  \param string string deliminated with tilder
 *  \param position position of the requested parameter
 *  \return corresponding parameter */
QString getParm(QString string, int position) {
    QStringList stringList = string.split("~");
    if (stringList.size() >= position) {
        return stringList.at(position);
    }
    return "";
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


