#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QJsonObject>
#include <functional>

#ifdef ENABLE_OSS_SDK
#include <alibabacloud/oss/OssClient.h>
#endif

/**
 * @brief 阿里云 OSS 直传上传器（使用 STS 临时凭证）
 *
 * 功能：
 * - 使用阿里云 OSS C++ SDK 直传文件
 * - 断点续传（Resumable Upload）
 * - 进度追踪和速度监控
 * - 智能重试机制
 * - 动态并发调整
 */
class OSSUploader : public QObject
{
    Q_OBJECT

public:
    struct STSCredentials {
        QString accessKeyId;
        QString accessKeySecret;
        QString securityToken;
        QString endpoint;
        QString bucketName;
        QString objectKey;
        QString expiration;
    };

    struct UploadConfig {
        qint64 partSize = 10 * 1024 * 1024;  // 默认 10MB 分片
        int threadNum = 3;                    // 默认 3 并发
        int maxRetries = 5;                   // 最大重试次数
        bool enableCheckpoint = true;         // 启用断点续传
        QString checkpointDir = "./upload_checkpoints";  // checkpoint 目录
    };

    explicit OSSUploader(QObject *parent = nullptr);
    ~OSSUploader();

    /**
     * @brief 开始上传文件
     * @param filePath 本地文件路径
     * @param taskId 任务ID
     * @param credentials STS 临时凭证
     * @param config 上传配置
     */
    void startUpload(const QString& filePath,
                    const QString& taskId,
                    const STSCredentials& credentials,
                    const UploadConfig& config = UploadConfig());

    /**
     * @brief 暂停上传
     */
    void pause();

    /**
     * @brief 继续上传
     */
    void resume();

    /**
     * @brief 取消上传
     */
    void cancel();

    /**
     * @brief 是否正在上传
     */
    bool isUploading() const { return m_isUploading; }

    /**
     * @brief 是否支持 OSS SDK
     */
    static bool isOSSSDKAvailable();

signals:
    /**
     * @brief 上传进度
     * @param progress 进度 (0-100)
     * @param uploadedBytes 已上传字节数
     * @param totalBytes 总字节数
     */
    void progressChanged(int progress, qint64 uploadedBytes, qint64 totalBytes);

    /**
     * @brief 上传速度
     * @param bytesPerSecond 每秒字节数
     */
    void speedChanged(qint64 bytesPerSecond);

    /**
     * @brief 上传完成
     */
    void uploadFinished(bool success);

    /**
     * @brief 上传错误
     */
    void uploadError(const QString& error);

private slots:
    void onSpeedTimerTimeout();

private:
#ifdef ENABLE_OSS_SDK
    void initializeOSSClient(const STSCredentials& credentials);
    void performUpload();
    void saveCheckpoint();
    void loadCheckpoint();
    int calculateOptimalConcurrency();

    static void progressCallback(size_t increment, int64_t transfered,
                                  int64_t total, void* userData);
#endif

    QString m_filePath;
    QString m_taskId;
    STSCredentials m_credentials;
    UploadConfig m_config;

    bool m_isUploading;
    bool m_isPaused;

    qint64 m_fileSize;
    qint64 m_uploadedBytes;
    qint64 m_lastUploadedBytes;
    qint64 m_currentSpeed;

    QTimer* m_speedTimer;

#ifdef ENABLE_OSS_SDK
    AlibabaCloud::OSS::OssClient* m_ossClient;
#endif
};
