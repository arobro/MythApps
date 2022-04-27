// MythTV headers
#include <mythscreentype.h>
#include <mythuibutton.h>
#include <mythuicheckbox.h>
#include <mythuitextedit.h>

/** \class MythOSD
 *  \brief Creates a pause on screen display. */
class MythOSD : public MythScreenType {
    Q_OBJECT

  public:
    MythOSD(MythScreenStack *parent, const QString &name);
    ~MythOSD() override;

    bool Create() override;
    bool keyPressEvent(QKeyEvent *event) override;

    void setPlayBackTimeOSD(QString playbackTime);

    bool waitforKey();

    void closeWindow();

  private:
    MythUIText *m_title;
    MythUIText *m_description;
    bool keyPlay = false;
    bool keyEscape = false;

  public slots:
};
