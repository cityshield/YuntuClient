#pragma once

#include <QObject>
#include <QFile>
#include <QString>
#include <QVector>
#include <QMap>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>

/**
 * @brief 文件分片上传器
 *
 * 功能：
 * - 分片上传大文件
 * - 断点续传
 * - 上传进度追踪
 * - 并发上传多个分片
 * - 上传失败自动重试
 */
class FileUploader : public QObject
{
    Q_OBJECT

public:
    struct ChunkInfo {
        int index;
        qint64 offset;
        qint64 size;
        bool uploaded;
        int retryCount;
    };

    explicit FileUploader(QObject *parent = nullptr);
    ~FileUploader();

    /**
     * @brief 开始上传文件
     * @param filePath 本地文件路径
     * @param taskId 任务ID
     */
    void startUpload(const QString& filePath, const QString& taskId);

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
     * @brief 设置分片大小（字节）
     */
    void setChunkSize(qint64 size) { m_chunkSize = size; }

    /**
     * @brief 设置并发上传数
     */
    void setConcurrency(int count) { m_maxConcurrency = count; }

    /**
     * @brief 设置最大重试次数
     */
    void setMaxRetries(int count) { m_maxRetries = count; }

    /**
     * @brief 是否正在上传
     */
    bool isUploading() const { return m_isUploading; }

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
    void onChunkUploaded(int chunkIndex, bool success);
    void onSpeedTimerTimeout();

private:
    void prepareChunks();
    void uploadNextChunk();
    void uploadChunk(int chunkIndex);
    void mergeChunks();
    void updateProgress();
    void calculateSpeed();

    QString m_filePath;
    QString m_taskId;
    QFile* m_file;
    qint64 m_fileSize;

    qint64 m_chunkSize;
    int m_maxConcurrency;
    int m_maxRetries;

    QVector<ChunkInfo> m_chunks;
    int m_uploadingCount;
    int m_completedCount;

    bool m_isUploading;
    bool m_isPaused;

    qint64 m_uploadedBytes;
    qint64 m_lastUploadedBytes;
    QTimer* m_speedTimer;
    qint64 m_currentSpeed;
};
