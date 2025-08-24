#include "netSocketRequest.h"

// QT headers
#include <QAuthenticator>
#include <QByteArray>
#include <QJsonDocument>
#include <QTimer>

// MythTV headers
#include <libmyth/mythcontext.h>

// MythApps headers
#include "shared.h"

QStringList allowedMethods = {"Player.OnPlay", "Player.OnStop", "Input.OnInputRequested", "Player.OnAVStart", "Player.OnResume", "Player.OnSeek", "Player.OnPause"};

NetSocketRequest::NetSocketRequest(const QString &url, QObject *parent) : QObject(parent) {
    LOGS(1, "", "url", url);

    m_request.setUrl(QUrl(url));

    connect(&m_ws, &QWebSocket::connected, this, &NetSocketRequest::onConnected);
    connect(&m_ws, &QWebSocket::disconnected, this, &NetSocketRequest::onDisconnected);
    connect(&m_ws, &QWebSocket::textMessageReceived, this, &NetSocketRequest::onTextMessageReceived);

    if (m_ws.isValid()) {
        m_ready = true;
    }

    m_ws.open(m_request);
}

NetSocketRequest::~NetSocketRequest() { m_ws.abort(); }

void NetSocketRequest::onConnected() {
    LOGS(1, "WebSocket connected");
    m_ready = true;
}

void NetSocketRequest::onDisconnected() {
    LOGS(1, "WebSocket disconnected");
    m_ready = false;
}

void NetSocketRequest::onTextMessageReceived(const QString &message) {
    LOGS(0, "", "message", message);
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    QJsonObject obj = doc.object();
    int msgId = obj["id"].toInt();

    m_responses[msgId] = obj["result"];
    if (m_waitLoops.contains(msgId)) {
        m_waitLoops[msgId]->quit();
    }

    if (containsAny(message, allowedMethods)) {
        QString methodName = obj["method"].toString();
        emit receivedFromSocket(methodName, message);
    }
}

void NetSocketRequest::ensureConnected() {
    LOGS(0, "");
    static int totalFailures = 0;   // Tracks cumulative failures
    const int maxRetries = 2;       // Per-call retry limit
    const int maxTotalFailures = 3; // Global failure threshold

    if (totalFailures >= maxTotalFailures) {
        LOG(VB_GENERAL, LOG_ERR, "ensureConnected(): aborting");
        return;
    }

    int attemptCount = 0;

    while (!m_ready && attemptCount < maxRetries) {
        m_ws.open(m_request);
        attemptCount++;

        QEventLoop loop;
        connect(&m_ws, &QWebSocket::connected, &loop, &QEventLoop::quit);
        QTimer::singleShot(500, &loop, &QEventLoop::quit);
        loop.exec();

        if (m_ws.state() == QAbstractSocket::ConnectedState) {
            m_ready = true;
            totalFailures = 0;
            break;
        } else {
            totalFailures++;
        }
    }

    if (!m_ready)
        LOG(VB_GENERAL, LOG_DEBUG, QString("ensureConnected(): failed after %1 attempts (total failures: %2)").arg(attemptCount).arg(totalFailures));
}

QJsonValue NetSocketRequest::call(const QString &method, const QJsonObject &params, const QJsonArray &properties) {
    LOGS(0, "", "method", method);
    ensureConnected();
    if (!m_ready)
        return {};

    int msgId = ++m_nextId;
    QJsonObject req{{"jsonrpc", "2.0"}, {"id", msgId}, {"method", method}};

    QJsonObject fp = params;
    if (!properties.isEmpty())
        fp["properties"] = properties;
    if (!fp.isEmpty())
        req["params"] = fp;

    auto text = QJsonDocument(req).toJson(QJsonDocument::Compact);
    m_ws.sendTextMessage(text);

    QEventLoop loop;
    m_waitLoops.insert(msgId, &loop);
    loop.exec();
    m_waitLoops.remove(msgId);

    QJsonValue result = m_responses.take(msgId);

    return result;
}

/** \brief switch between Kodi and MythTV on Android
 * 	\param app name of app to switch to
 *  \return is the helper switching app (mythapp services) running? */
bool NetSocketRequest::androidAppSwitch(QString app) {
    LOGS(1, "", "app", app);

    if (!m_webSocketAndroid.isValid()) {
        m_webSocketAndroid.open(QUrl("ws://127.0.0.1:8088"));

        QObject::connect(&m_webSocketAndroid, &QWebSocket::connected, [=]() {
            QObject::connect(&m_webSocketAndroid, &QWebSocket::textMessageReceived, [=]() {
                LOG(VB_GENERAL, LOG_INFO, "NSR->androidAppSwitch() mythAppsServiceconnected ");
                mythAppsServiceconnected = true;
            });
        });
    }

    for (int i = 0; i < 5; i++) {
        delayMilli(500);
        if (mythAppsServiceconnected) {
            break;
        }
    }

    if (!mythAppsServiceconnected) {
        LOG(VB_GENERAL, LOG_INFO, "NSR->androidAppSwitch() MythApp Services apk not installed or running.");
        return false;
    }
    m_webSocketAndroid.sendTextMessage(app);
    return true;
}

bool NetSocketRequest::containsAny(const QString &message, const QStringList &keywords) {
    for (const QString &keyword : keywords) {
        if (message.contains(keyword)) {
            return true;
        }
    }
    return false;
}
