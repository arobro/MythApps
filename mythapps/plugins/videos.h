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

    void load(const QString data) override;
    void displayHomeScreenItems() override;

    void setControls(Controls *c);

  private:
    void loadVideos();
    void loadDirectory(const QString &folderPath, bool recursive);

    void generateMythTVThumbnailAsync(const QString &videoFilePath, bool isPlay);

    void handleThumbnailReady(const QString &videoFilePath, const QString &thumbnailPath, bool isPlay);

    QString pluginName;
    QString pluginIcon;

    void internalPlay(const QString url);

    Controls *controls = nullptr;
    Dialog *dialog{nullptr};

    void updateMediaListCallback(const QString &data);
};

#endif // VIDEOS_H
