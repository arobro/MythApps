#ifndef VIDEOS_H
#define VIDEOS_H

#include "plugin_api.h"

// QT headers
#include <QList>
#include <QStringList>
#include <QVariantMap>

// MythApps headers
#include "controls.h"

class Videos : public PluginAPI {
  public:
    Videos();
    ~Videos() override;

    QString getPluginName() const override;
    QString getPluginDisplayName() const override;
    QString getPluginIcon() const override;
    void setDialog(Dialog *d) override;
    void setFileBrowserHistory(FileBrowserHistory *f) override;

    void load(const QString filePath) override;
    void displayHomeScreenItems() override;

    void setControls(Controls *c);

  private:
    void loadVideos();
    void loadDirectory(const QString &folderPath, bool recursive);

    void generateMythTVThumbnailAsync(const QString &videoFilePath, bool isDir);

    void handleThumbnailReady(const QString &videoFilePath, const QString &thumbnailPath, bool isDir);

    QString pluginName;
    QString pluginIcon;

    void internalPlay(const QString url);

    Controls *controls = nullptr;
    Dialog *dialog{nullptr};
    FileBrowserHistory *fileBrowserHistory{nullptr};

    void updateMediaListCallback(const QString filePath);

    QString makeAppPayload(const QString &filePath, bool play);
};

#endif // VIDEOS_H
