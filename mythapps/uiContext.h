#ifndef UICONTEXT_H
#define UICONTEXT_H

// Forward declarations of MythTV UI classes
class MythUIText;
class MythUIShape;
class MythUIButtonList;
class MythUIImage;
class MythUITextEdit;
class MythUIType;

struct UIContext {
    MythUIText *plot = nullptr;
    MythUIText *streamDetails = nullptr;
    MythUIShape *streamDetailsbackground = nullptr;
    MythUIText *filepath = nullptr;
    MythUIText *title = nullptr;
    MythUIButtonList *fileListGrid = nullptr;
    MythUIImage *screenshotMainMythImage = nullptr;
    MythUITextEdit *SearchTextEdit = nullptr;
    MythUIText *SearchTextEditBackgroundText = nullptr;
    MythUIButtonList *searchButtonList = nullptr;
    MythUIButtonList *searchSettingsButtonList = nullptr;
    MythUIImage *loaderImage = nullptr;
    MythUIType *searchButtonListGroup = nullptr;
    MythUIType *searchSettingsGroup = nullptr;
    MythUIButton *androidMenuBtn = nullptr;
    MythUIType *help = nullptr;
};

#endif // UICONTEXT_H
