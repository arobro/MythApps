#ifndef DIALOG_H
#define DIALOG_H

// QT headers
#include <QObject>
#include <QString>

// MythTV headers
#include <libmythui/mythdialogbox.h>
#include <libmythui/mythuiimage.h>

class MythUIBusyDialog;

class Dialog : public QObject {
    Q_OBJECT

  public:
    explicit Dialog(QObject *parent, MythUIImage *loaderImage);
    ~Dialog();

    void createAutoClosingBusyDialog(QString dialogText, int delaySeconds);
    void createBusyDialog(const QString &title);
    void closeBusyDialog();

    MythUIImage *getLoader();

    void confirmDialog(QString description, QString type);

  private:
    MythUIBusyDialog *m_busyPopup;
    MythUIImage *m_loaderImage{nullptr};
    MythDialogBox *m_menuPopup{nullptr};
};

#endif // DIALOG_H
