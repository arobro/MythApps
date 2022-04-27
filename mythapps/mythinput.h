// MythTV headers
#include <mythscreentype.h>
#include <mythuibutton.h>
#include <mythuitextedit.h>

/** \class MythInput
 *  \brief Inputbox. Can also take a password */
class MythInput : public MythScreenType {
    Q_OBJECT

  public:
    MythInput(MythScreenStack *parent, const QString &name);
    ~MythInput() override;

    bool Create() override;
    bool keyPressEvent(QKeyEvent *event) override;
    bool passwordActive = false;

    void setTitle(QString title);
    bool setType(QString type);
    void setTextValue(QString value);
    QString getTextValue();

    bool waitforKey();
    bool isCancelled();
    void closeWindow();

  private:
    MythUIText *m_title;
    MythUITextEdit *m_TextEditValue;

    bool keyOk = false;
    bool cancel = false;

    MythUIButton *m_okBtn;

  public slots:

    void button_ok();
};
