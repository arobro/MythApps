#ifndef FileBrowserHistory_h
#define FileBrowserHistory_h

// QT headers
#include <QList>
#include <QString>

/** \class FileBrowserHistory
 *  \brief  Stores the urls for the back button*/

class FileBrowserHistory {
  public:
    FileBrowserHistory();

    void goBack();
    void append(QString label, QString data);
    bool isEmpty();

    QString getCurrentLabel();
    QString getCurrentData();

  private:
    void removeCurrentUrlFromList();

    QList<QStringList> previousListItem; /*!< A list of directories the user has clicked on */
};
#endif
