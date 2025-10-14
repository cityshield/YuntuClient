#include "LogUploader.h"
#include "../network/HttpClient.h"
#include "../core/Logger.h"
#include "../core/Application.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonDocument>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QNetworkReply>
#include <QNetworkAccessManager>

LogUploader::LogUploader(QObject *parent)
    : QObject(parent)
    , m_uploadCount(0)
    , m_totalCount(0)
{
}

LogUploader::~LogUploader()
{
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

    if (!logFile.open(QIODevice::ReadOnly)) {
        Application::instance().logger()->warning("LogUploader",
            QString::fromUtf8("无法打开日志文件: %1").arg(logFilePath));
        emit logUploadFailed(logFilePath, QString::fromUtf8("无法打开文件"));
        return;
    }

    QByteArray logData = logFile.readAll();
    logFile.close();

    QFileInfo fileInfo(logFilePath);
    QString fileName = fileInfo.fileName();

    Application::instance().logger()->info("LogUploader",
        QString::fromUtf8("开始上传日志: %1 (%2 bytes)").arg(fileName).arg(logData.size()));

    // 构造上传请求
    QJsonObject data;
    data["fileName"] = fileName;
    data["fileSize"] = logData.size();
    data["logContent"] = QString::fromUtf8(logData.toBase64());  // Base64 编码
    data["uploadTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    HttpClient::instance().post(
        "/api/v1/logs/upload",
        data,
        [this, logFilePath, fileName](const QJsonObject& response) {
            Application::instance().logger()->info("LogUploader",
                QString::fromUtf8("日志上传成功: %1").arg(fileName));
            emit logUploaded(logFilePath);

            m_uploadCount++;
            if (m_uploadCount >= m_totalCount) {
                emit allLogsUploaded();
            }
        },
        [this, logFilePath, fileName](int statusCode, const QString& error) {
            Application::instance().logger()->error("LogUploader",
                QString::fromUtf8("日志上传失败: %1, 错误: %2").arg(fileName).arg(error));
            emit logUploadFailed(logFilePath, error);

            m_uploadCount++;
            if (m_uploadCount >= m_totalCount) {
                emit allLogsUploaded();
            }
        }
    );
}

void LogUploader::uploadAllLogs(const QStringList& logFilePaths)
{
    if (logFilePaths.isEmpty()) {
        Application::instance().logger()->info("LogUploader", QString::fromUtf8("没有日志文件需要上传"));
        emit allLogsUploaded();
        return;
    }

    m_uploadCount = 0;
    m_totalCount = logFilePaths.size();

    Application::instance().logger()->info("LogUploader",
        QString::fromUtf8("开始上传 %1 个日志文件").arg(m_totalCount));

    // 依次上传每个日志文件
    for (const QString& logPath : logFilePaths) {
        uploadLog(logPath);
    }
}
