#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QNetworkAccessManager>

/**
 * @brief 日志上传服务
 *
 * 负责将本地日志文件直接上传到阿里云 OSS
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
    /**
     * @brief 生成 OSS 签名
     * @param verb HTTP 方法 (PUT, GET等)
     * @param contentMd5 内容 MD5
     * @param contentType 内容类型
     * @param date 日期
     * @param ossHeaders OSS 自定义头
     * @param resource 资源路径
     * @return 签名字符串
     */
    QString generateOssSignature(
        const QString& verb,
        const QString& contentMd5,
        const QString& contentType,
        const QString& date,
        const QString& ossHeaders,
        const QString& resource
    );

    /**
     * @brief 上传文件到 OSS
     * @param filePath 本地文件路径
     * @param objectName OSS 对象名称
     */
    void uploadToOss(const QString& filePath, const QString& objectName);

private:
    QNetworkAccessManager* m_networkManager;
    int m_uploadCount;
    int m_totalCount;
};
