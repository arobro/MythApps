#ifndef MYTHAPPSDBCHECK_H_
#define MYTHAPPSDBCHECK_H_

template <typename T> void createSetting(const QString &settingName, const T &settingValue, bool remote);
bool UpgradeMythAppsDatabaseSchema(void);
QMap<QString, bool> &getSettingLocationMap();
void saveSetting(QString settingName, QString settingValue);
void saveSetting(QString settingName, bool settingValue);
#endif
