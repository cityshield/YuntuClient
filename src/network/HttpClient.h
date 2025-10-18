#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonObject>
#include <QJsonDocument>
#include <functional>
#include <QMap>

/**
 * @brief HTTP 客户端封装类
 *
 * 功能：
 * - 支持 GET/POST/PUT/DELETE 请求
 * - 自动添加 JWT Token
 * - JSON 数据自动序列化/反序列化
 * - 错误处理
 * - 请求超时控制
 */
class HttpClient : public QObject
{
    Q_OBJECT

public:
    // 回调函数类型
    using SuccessCallback = std::function<void(const QJsonObject&)>;
    using ErrorCallback = std::function<void(int statusCode, const QString& error)>;

    static HttpClient& instance();

    /**
     * @brief 设置基础 URL
     */
    void setBaseUrl(const QString& baseUrl);

    /**
     * @brief 设置访问令牌
     */
    void setAccessToken(const QString& token);

    /**
     * @brief 清除访问令牌
     */
    void clearAccessToken();

    /**
     * @brief GET 请求
     */
    void get(const QString& path,
             const QMap<QString, QString>& params = {},
             SuccessCallback onSuccess = nullptr,
             ErrorCallback onError = nullptr);

    /**
     * @brief POST 请求
     */
    void post(const QString& path,
              const QJsonObject& data,
              SuccessCallback onSuccess = nullptr,
              ErrorCallback onError = nullptr);

    /**
     * @brief PUT 请求
     */
    void put(const QString& path,
             const QJsonObject& data,
             SuccessCallback onSuccess = nullptr,
             ErrorCallback onError = nullptr);

    /**
     * @brief DELETE 请求
     */
    void deleteRequest(const QString& path,
                      SuccessCallback onSuccess = nullptr,
                      ErrorCallback onError = nullptr);

    /**
     * @brief 上传文件（multipart/form-data）
     */
    void uploadFile(const QString& path,
                   const QString& filePath,
                   const QMap<QString, QString>& fields = {},
                   SuccessCallback onSuccess = nullptr,
                   ErrorCallback onError = nullptr,
                   std::function<void(qint64, qint64)> onProgress = nullptr);

    /**
     * @brief 上传文件分片（multipart/form-data，流式上传）
     */
    void uploadChunk(const QString& path,
                    const QString& filePath,
                    qint64 offset,
                    qint64 size,
                    const QMap<QString, QString>& fields = {},
                    SuccessCallback onSuccess = nullptr,
                    ErrorCallback onError = nullptr,
                    std::function<void(qint64, qint64)> onProgress = nullptr);

    /**
     * @brief 下载文件
     */
    void downloadFile(const QString& url,
                     const QString& savePath,
                     std::function<void(qint64, qint64)> onProgress = nullptr,
                     std::function<void()> onSuccess = nullptr,
                     ErrorCallback onError = nullptr);

    /**
     * @brief 设置请求超时时间（毫秒）
     */
    void setTimeout(int timeout) { m_timeout = timeout; }

signals:
    /**
     * @brief 请求开始信号
     */
    void requestStarted(const QString& url);

    /**
     * @brief 请求完成信号
     */
    void requestFinished(const QString& url, bool success);

private:
    HttpClient();
    ~HttpClient();
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;

    QNetworkRequest buildRequest(const QString& path);
    void handleReply(QNetworkReply* reply,
                    SuccessCallback onSuccess,
                    ErrorCallback onError);

    QString buildUrl(const QString& path, const QMap<QString, QString>& params = {});

    QNetworkAccessManager* m_manager;
    QString m_baseUrl;
    QString m_accessToken;
    int m_timeout;  // 超时时间（毫秒）
};
