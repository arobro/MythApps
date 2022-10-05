#ifndef ProgramLink_h
#define ProgramLink_h

#include <QString>
#include <QStringList>

#include "container.h"

class ProgramLink {
  public:
    ProgramLink(QString linkName, bool _remote, bool _mostRecentOnly);

    QString getListSize();
    int getListSizeEnabled();

    QList<FileFolderContainer> getList();
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
    QList<FileFolderContainer> LinkDataList;

    QString linkName;
    bool remote;
    bool mostRecentOnly;

    FileFolderContainer fileFolderContainer;
};
#endif
