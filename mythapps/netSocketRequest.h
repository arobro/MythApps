#pragma once

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QObject>
#include <QWebSocket>

class NetSocketRequest : public QObject {
    Q_OBJECT

  public:
    NetSocketRequest(const QString &url, QObject *parent = nullptr);

    ~NetSocketRequest();

    QJsonValue call(const QString &method, const QJsonObject &params = {}, const QJsonArray &properties = {});
    bool androidAppSwitch(QString app);

  private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString &message);

  private:
    void ensureConnected();
    bool containsAny(const QString &message, const QStringList &keywords);

    QWebSocket m_ws;
    QNetworkRequest m_request;
    bool m_ready = false;
    int m_nextId = 0;
    QHash<int, QJsonValue> m_responses;
    QHash<int, QEventLoop *> m_waitLoops;

    QWebSocket m_webSocketAndroid;
    bool mythAppsServiceconnected = false;

  signals:
    void receivedFromSocket(const QString &method, const QString &message);
};
