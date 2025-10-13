#include "WebSocketClient.h"
#include <QJsonDocument>
#include <QDebug>

WebSocketClient::WebSocketClient(QObject *parent)
    : QObject(parent)
    , m_webSocket(new QWebSocket())
    , m_heartbeatTimer(new QTimer(this))
    , m_reconnectTimer(new QTimer(this))
    , m_state(Disconnected)
    , m_reconnectAttempts(0)
    , m_maxReconnectAttempts(5)
    , m_reconnectInterval(5000)  // 5秒
{
    connect(m_webSocket, &QWebSocket::connected, this, &WebSocketClient::onConnected);
    connect(m_webSocket, &QWebSocket::disconnected, this, &WebSocketClient::onDisconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &WebSocketClient::onTextMessageReceived);
    connect(m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &WebSocketClient::onError);

    // 心跳定时器（每30秒发送一次心跳）
    m_heartbeatTimer->setInterval(30000);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &WebSocketClient::onHeartbeatTimeout);

    // 重连定时器
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &WebSocketClient::attemptReconnect);
}

WebSocketClient::~WebSocketClient()
{
    disconnect();
    delete m_webSocket;
}

void WebSocketClient::connectToServer(const QString& url, const QString& userId)
{
    if (m_state == Connecting || m_state == Connected) {
        qDebug() << "WebSocket: 已经在连接中";
        return;
    }

    m_url = url;
    m_userId = userId;
    m_state = Connecting;
    m_reconnectAttempts = 0;

    qDebug() << "WebSocket: 连接到" << url;
    m_webSocket->open(QUrl(url));
}

void WebSocketClient::disconnect()
{
    m_reconnectTimer->stop();
    stopHeartbeat();

    if (m_webSocket->state() == QAbstractSocket::ConnectedState) {
        m_webSocket->close();
    }

    m_state = Disconnected;
}

void WebSocketClient::sendMessage(const QString& event, const QJsonObject& data)
{
    if (m_state != Connected) {
        qWarning() << "WebSocket: 未连接，无法发送消息";
        return;
    }

    QJsonObject message;
    message["event"] = event;
    message["data"] = data;
    message["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    QString jsonString = QJsonDocument(message).toJson(QJsonDocument::Compact);
    m_webSocket->sendTextMessage(jsonString);

    qDebug() << "WebSocket: 发送消息:" << event;
}

void WebSocketClient::onConnected()
{
    qDebug() << "WebSocket: 连接成功";
    m_state = Connected;
    m_reconnectAttempts = 0;

    setupHeartbeat();
    emit connected();
}

void WebSocketClient::onDisconnected()
{
    qDebug() << "WebSocket: 连接断开";
    stopHeartbeat();

    ConnectionState oldState = m_state;
    m_state = Disconnected;

    emit disconnected();

    // 自动重连
    if (oldState == Connected && m_reconnectAttempts < m_maxReconnectAttempts) {
        m_state = Reconnecting;
        qDebug() << "WebSocket: 将在" << m_reconnectInterval << "ms后重连";
        m_reconnectTimer->start(m_reconnectInterval);
    }
}

void WebSocketClient::onTextMessageReceived(const QString& message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "WebSocket: 收到无效消息";
        return;
    }

    QJsonObject obj = doc.object();
    handleMessage(obj);
}

void WebSocketClient::onError(QAbstractSocket::SocketError error)
{
    QString errorString = m_webSocket->errorString();
    qWarning() << "WebSocket: 错误" << error << errorString;

    emit this->error(errorString);
}

void WebSocketClient::onHeartbeatTimeout()
{
    // 发送心跳
    QJsonObject data;
    data["type"] = "ping";
    sendMessage("heartbeat", data);
}

void WebSocketClient::attemptReconnect()
{
    m_reconnectAttempts++;
    qDebug() << "WebSocket: 重连尝试" << m_reconnectAttempts << "/" << m_maxReconnectAttempts;

    m_state = Connecting;
    m_webSocket->open(QUrl(m_url));
}

void WebSocketClient::setupHeartbeat()
{
    m_heartbeatTimer->start();
}

void WebSocketClient::stopHeartbeat()
{
    m_heartbeatTimer->stop();
}

void WebSocketClient::handleMessage(const QJsonObject& message)
{
    QString event = message["event"].toString();
    QJsonObject data = message["data"].toObject();

    qDebug() << "WebSocket: 收到消息:" << event;

    emit messageReceived(event, data);

    // 处理特定事件
    if (event == "task:progress") {
        // 任务进度更新
        QString taskId = data["taskId"].toString();
        int progress = data["progress"].toInt();
        emit taskProgressUpdated(taskId, progress);

    } else if (event == "task:log") {
        // 任务日志
        QString taskId = data["taskId"].toString();
        QString log = data["log"].toString();
        emit taskLogReceived(taskId, log);

    } else if (event == "task:status") {
        // 任务状态变化
        QString taskId = data["taskId"].toString();
        QString status = data["status"].toString();
        emit taskStatusChanged(taskId, status);

    } else if (event == "notification") {
        // 通知消息
        QString title = data["title"].toString();
        QString message = data["message"].toString();
        emit notificationReceived(title, message);

    } else if (event == "pong") {
        // 心跳响应
        qDebug() << "WebSocket: 心跳响应收到";
    }
}
