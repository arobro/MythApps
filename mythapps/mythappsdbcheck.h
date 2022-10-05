#ifndef MYTHAPPSDBCHECK_H_
#define MYTHAPPSDBCHECK_H_

template <typename T> void createSetting(QString settingName, T settingValue, bool remote);
bool UpgradeMythAppsDatabaseSchema(void);

#endif
