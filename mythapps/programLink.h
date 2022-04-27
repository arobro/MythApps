#ifndef ProgramLink_h
#define ProgramLink_h

#include <QString>
#include <QStringList>

class ProgramLink {
  public:
    ProgramLink(QString linkName, bool _remote, bool _mostRecentOnly);

    QString getListSize();
    int getListSizeEnabled();
    QStringList getList();
    QStringList getListEnabled();

    bool contains(QString currentselectionDetails);
    void removeOne(QString currentselectionDetails);
    void append(QString currentselectionDetails);

    void listRemove(QString selectionDetails);
    void load();

    void sort();

    bool isPinnedToHome(QString currentselectionDetails);
    void removeFromHomeScreen(QString currentselectionDetails);
    void addToHomeScreen(QString currentselectionDetails);
    QString getUnWatchedSize();

  private:
    QString listRemoveIfContains(QString line);

    QStringList LinkDataList;
    QString linkName;
    bool remote;
    bool mostRecentOnly;
    int listSizeEnabled = 0;

    void saveList();
};
#endif
