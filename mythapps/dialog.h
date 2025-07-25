#ifndef DIALOG_H
#define DIALOG_H

// QT headers
#include <QObject>
#include <QString>

// MythTV headers
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

  private:
    MythUIBusyDialog *m_busyPopup;
    MythUIImage *m_loaderImage{nullptr};
};

#endif // DIALOG_H
