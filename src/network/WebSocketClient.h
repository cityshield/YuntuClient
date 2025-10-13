#pragma once

#include <QObject>
#include <QWebSocket>
#include <QJsonObject>
#include <QTimer>

/**
 * @brief WebSocket 客户端
 *
 * 功能：
 * - 连接到服务器
 * - 自动重连
 * - 心跳保持
 * - 消息发送和接收
 * - 事件订阅
 */
class WebSocketClient : public QObject
{
    Q_OBJECT

public:
    enum ConnectionState {
        Disconnected,
        Connecting,
        Connected,
        Reconnecting
    };

    explicit WebSocketClient(QObject *parent = nullptr);
    ~WebSocketClient();

    /**
     * @brief 连接到服务器
     */
    void connectToServer(const QString& url, const QString& userId);

    /**
     * @brief 断开连接
     */
    void disconnect();

    /**
     * @brief 发送消息
     */
    void sendMessage(const QString& event, const QJsonObject& data);

    /**
     * @brief 获取连接状态
     */
    ConnectionState state() const { return m_state; }

    /**
     * @brief 是否已连接
     */
    bool isConnected() const { return m_state == Connected; }

signals:
    /**
     * @brief 连接成功
     */
    void connected();

    /**
     * @brief 连接断开
     */
    void disconnected();

    /**
     * @brief 连接错误
     */
    void error(const QString& errorString);

    /**
     * @brief 接收到消息
     */
    void messageReceived(const QString& event, const QJsonObject& data);

    /**
     * @brief 任务进度更新
     */
    void taskProgressUpdated(const QString& taskId, int progress);

    /**
     * @brief 任务日志
     */
    void taskLogReceived(const QString& taskId, const QString& log);

    /**
     * @brief 任务状态变化
     */
    void taskStatusChanged(const QString& taskId, const QString& status);

    /**
     * @brief 通知消息
     */
    void notificationReceived(const QString& title, const QString& message);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString& message);
    void onError(QAbstractSocket::SocketError error);
    void onHeartbeatTimeout();
    void attemptReconnect();

private:
    void setupHeartbeat();
    void stopHeartbeat();
    void handleMessage(const QJsonObject& message);

    QWebSocket* m_webSocket;
    QTimer* m_heartbeatTimer;
    QTimer* m_reconnectTimer;

    QString m_url;
    QString m_userId;
    ConnectionState m_state;

    int m_reconnectAttempts;
    int m_maxReconnectAttempts;
    int m_reconnectInterval;  // 毫秒
};
