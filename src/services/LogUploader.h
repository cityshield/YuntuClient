#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

/**
 * @brief 日志上传服务
 *
 * 负责将本地日志文件上传到 OSS
 */
class LogUploader : public QObject
{
    Q_OBJECT

public:
    explicit LogUploader(QObject *parent = nullptr);
    ~LogUploader();

    /**
     * @brief 上传指定的日志文件到 OSS
     * @param logFilePath 日志文件路径
     */
    void uploadLog(const QString& logFilePath);

    /**
     * @brief 上传所有日志文件到 OSS
     * @param logFilePaths 日志文件路径列表
     */
    void uploadAllLogs(const QStringList& logFilePaths);

signals:
    /**
     * @brief 单个日志上传成功
     */
    void logUploaded(const QString& filePath);

    /**
     * @brief 单个日志上传失败
     */
    void logUploadFailed(const QString& filePath, const QString& error);

    /**
     * @brief 所有日志上传完成
     */
    void allLogsUploaded();

private:
    int m_uploadCount;
    int m_totalCount;
};
