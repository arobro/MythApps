#ifndef container_h
#define container_h

/** \struct FileFolderContainer
 *  \brief Stores program data*/
struct FileFolderContainer {
    int dbID;
    QString title;
    QString url;
    QString plot;
    QString image;
    bool autoPlay;
    QString seek;
    bool pinnedToHome;
};

struct PlaybackTime {
    qint64 currentMs = 0;
    QString duration;
};

struct PlaybackInfo {
    QString currentTime;
    qint64 elapsedMs = 0;
    QString duration;
};

#endif
