#include "LogUploader.h"
#include "../core/Application.h"
#include "../core/Config.h"
#include "../core/Logger.h"
#include "../network/ApiService.h"
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QCryptographicHash>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMessageAuthenticationCode>
#include <QUrl>
#include <QDebug>

LogUploader::LogUploader(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_uploadCount(0)
    , m_totalCount(0)
    , m_ossConfigValid(false)
{
}

LogUploader::~LogUploader()
{
}

QString LogUploader::generateOssSignature(
    const QString& verb,
    const QString& contentMd5,
    const QString& contentType,
    const QString& date,
    const QString& ossHeaders,
    const QString& resource)
{
    // 构造签名字符串
    // Signature = base64(hmac-sha1(AccessKeySecret,
    //             VERB + "\n"
    //             + Content-MD5 + "\n"
    //             + Content-Type + "\n"
    //             + Date + "\n"
    //             + CanonicalizedOSSHeaders
    //             + CanonicalizedResource))

    QString stringToSign = verb + "\n"
                         + contentMd5 + "\n"
                         + contentType + "\n"
                         + date + "\n"
                         + ossHeaders
                         + resource;

    // 使用缓存的密钥
    QString secretKey = m_ossAccessKeySecret;

    // 使用 HMAC-SHA1 签名
    QByteArray key = secretKey.toUtf8();
    QByteArray message = stringToSign.toUtf8();

    QByteArray hash = QMessageAuthenticationCode::hash(
        message,
        key,
        QCryptographicHash::Sha1
    );

    return hash.toBase64();
}

void LogUploader::uploadToOss(const QString& filePath, const QString& objectName)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件:" << filePath;
        emit logUploadFailed(filePath, QString::fromUtf8("无法打开文件"));

        m_uploadCount++;
        if (m_uploadCount >= m_totalCount) {
            emit allLogsUploaded();
        }
        return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    // 检查 OSS 配置是否有效
    if (!m_ossConfigValid) {
        qWarning() << "OSS 配置无效，请先获取配置";
        Application::instance().logger()->error("LogUploader",
            QString::fromUtf8("OSS 配置无效，无法上传日志: %1").arg(filePath));
        emit logUploadFailed(filePath, QString::fromUtf8("OSS 配置无效"));

        m_uploadCount++;
        if (m_uploadCount >= m_totalCount) {
            emit allLogsUploaded();
        }
        return;
    }

    // 使用缓存的 OSS 配置
    QString accessKey = m_ossAccessKeyId;
    QString bucket = m_ossBucketName;
    QString endpoint = m_ossEndpoint;

    // 构造 OSS URL
    // 格式: https://{bucket}.{endpoint}/{objectName}
    QString ossUrl = QString("https://%1.%2/%3")
                        .arg(bucket)
                        .arg(endpoint)
                        .arg(objectName);

    qDebug() << "准备上传到 OSS:" << ossUrl;

    // 生成日期（GMT 格式）
    QString date = QDateTime::currentDateTimeUtc().toString("ddd, dd MMM yyyy HH:mm:ss 'GMT'");

    // 内容类型
    QString contentType = "text/plain";

    // 资源路径
    QString resource = QString("/%1/%2").arg(bucket).arg(objectName);

    // 生成签名
    QString signature = generateOssSignature("PUT", "", contentType, date, "", resource);

    // 构造 Authorization 头
    QString authorization = QString("OSS %1:%2").arg(accessKey).arg(signature);

    qDebug() << "OSS Authorization:" << authorization;

    // 创建请求
    QUrl url(ossUrl);
    QNetworkRequest networkRequest(url);
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    networkRequest.setRawHeader("Date", date.toUtf8());
    networkRequest.setRawHeader("Authorization", authorization.toUtf8());

    // 发送 PUT 请求
    QNetworkReply* reply = m_networkManager->put(networkRequest, fileData);

    // 连接完成信号
    connect(reply, &QNetworkReply::finished, this, [this, reply, filePath, ossUrl]() {
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "日志上传成功:" << filePath << "到" << ossUrl;
            Application::instance().logger()->info("LogUploader",
                QString::fromUtf8("日志上传成功: %1").arg(filePath));
            emit logUploaded(filePath);
        } else {
            QString errorMsg = reply->errorString();
            QByteArray responseData = reply->readAll();
            qWarning() << "日志上传失败:" << filePath;
            qWarning() << "错误:" << errorMsg;
            qWarning() << "响应:" << responseData;

            Application::instance().logger()->error("LogUploader",
                QString::fromUtf8("日志上传失败: %1, 错误: %2, 响应: %3")
                    .arg(filePath).arg(errorMsg).arg(QString::fromUtf8(responseData)));

            emit logUploadFailed(filePath, errorMsg);
        }

        reply->deleteLater();

        m_uploadCount++;
        if (m_uploadCount >= m_totalCount) {
            emit allLogsUploaded();
        }
    });
}

void LogUploader::uploadLog(const QString& logFilePath)
{
    QFile logFile(logFilePath);
    if (!logFile.exists()) {
        Application::instance().logger()->warning("LogUploader",
            QString::fromUtf8("日志文件不存在: %1").arg(logFilePath));
        emit logUploadFailed(logFilePath, QString::fromUtf8("文件不存在"));
        return;
    }

    QFileInfo fileInfo(logFilePath);
    QString fileName = fileInfo.fileName();

    // 生成 OSS 对象名称，格式: logs/YYYY-MM-DD/filename.log
    QString dateFolder = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QString objectName = QString("logs/%1/%2").arg(dateFolder).arg(fileName);

    Application::instance().logger()->info("LogUploader",
        QString::fromUtf8("开始上传日志: %1 -> %2").arg(fileName).arg(objectName));

    // 上传到 OSS
    uploadToOss(logFilePath, objectName);
}

void LogUploader::uploadAllLogs(const QStringList& logFilePaths)
{
    if (logFilePaths.isEmpty()) {
        Application::instance().logger()->info("LogUploader", QString::fromUtf8("没有日志文件需要上传"));
        emit allLogsUploaded();
        return;
    }

    // 保存待上传的日志路径
    m_pendingLogPaths = logFilePaths;

    // 如果没有 OSS 配置，先获取
    if (!m_ossConfigValid) {
        Application::instance().logger()->info("LogUploader",
            QString::fromUtf8("正在从服务器获取 OSS 配置..."));
        fetchOssConfig();
        return;
    }

    // 已有配置，直接上传
    m_uploadCount = 0;
    m_totalCount = logFilePaths.size();

    Application::instance().logger()->info("LogUploader",
        QString::fromUtf8("开始上传 %1 个日志文件到 OSS").arg(m_totalCount));

    qDebug() << "========== 开始上传日志到 OSS ==========";
    qDebug() << "总共" << m_totalCount << "个文件";

    // 依次上传每个日志文件
    for (const QString& logPath : logFilePaths) {
        uploadLog(logPath);
    }
}

void LogUploader::fetchOssConfig()
{
    Application::instance().logger()->info("LogUploader",
        QString::fromUtf8("正在向服务器请求 OSS 配置..."));

    ApiService::instance().getOssConfig(
        [this](const QJsonObject& response) {
            onOssConfigReceived(response);
        },
        [this](int statusCode, const QString& error) {
            onOssConfigError(statusCode, error);
        }
    );
}

void LogUploader::onOssConfigReceived(const QJsonObject& response)
{
    // 解析 OSS 配置
    m_ossAccessKeyId = response["access_key_id"].toString();
    m_ossAccessKeySecret = response["access_key_secret"].toString();
    m_ossBucketName = response["bucket_name"].toString();
    m_ossEndpoint = response["endpoint"].toString();
    m_ossBaseUrl = response["base_url"].toString();

    // 验证配置完整性
    if (m_ossAccessKeyId.isEmpty() || m_ossAccessKeySecret.isEmpty() ||
        m_ossBucketName.isEmpty() || m_ossEndpoint.isEmpty()) {
        Application::instance().logger()->error("LogUploader",
            QString::fromUtf8("OSS 配置不完整，无法上传日志"));
        m_ossConfigValid = false;
        emit allLogsUploaded();
        return;
    }

    m_ossConfigValid = true;

    Application::instance().logger()->info("LogUploader",
        QString::fromUtf8("OSS 配置获取成功，Bucket: %1, Endpoint: %2")
            .arg(m_ossBucketName).arg(m_ossEndpoint));

    // 开始上传待处理的日志
    if (!m_pendingLogPaths.isEmpty()) {
        uploadAllLogs(m_pendingLogPaths);
    }
}

void LogUploader::onOssConfigError(int statusCode, const QString& error)
{
    Application::instance().logger()->error("LogUploader",
        QString::fromUtf8("获取 OSS 配置失败 (状态码: %1): %2")
            .arg(statusCode).arg(error));

    m_ossConfigValid = false;

    // 通知上传失败
    emit allLogsUploaded();
}
