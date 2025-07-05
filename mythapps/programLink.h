#ifndef ProgramLink_h
#define ProgramLink_h

// QT headers
#include <QString>
#include <QStringList>

// MythTV headers
#include <libmyth/mythcontext.h>

// MythApps headers
#include "container.h"

class ProgramLink {
  public:
    ProgramLink(QString linkName);

    QString getListSize();
    int getListSizeEnabled();

    QList<FileFolderContainer> getList(bool descending = false, int limit = 0);
    QStringList getListEnabled();

    bool contains(QString url);
    bool containsLike(QString url);
    QString findSearchUrl(QString url);

    void append(FileFolderContainer fileFolderContainerTemp);
    void appendSearchUrl(QString currentSearchUrl);

    void listRemove(FileFolderContainer fileFolderContainerTemp);

    bool isPinnedToHome(FileFolderContainer fileFolderContainerTemp);
    void removeFromHomeScreen(FileFolderContainer fileFolderContainerTemp);
    void addToHomeScreen(FileFolderContainer fileFolderContainerTemp);

    QString getUnWatchedSize();

  private:
    void execQuery(MSqlQuery &query);

    QList<FileFolderContainer> LinkDataList;
    QString linkName;
    FileFolderContainer fileFolderContainer;
};
#endif // ProgramLink_h
