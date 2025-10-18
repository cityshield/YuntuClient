#include "OSSUploader.h"
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QDebug>
#include <QJsonDocument>
#include <QFile>

#ifdef ENABLE_OSS_SDK
#include <alibabacloud/oss/OssClient.h>
using namespace AlibabaCloud::OSS;
#endif

OSSUploader::OSSUploader(QObject *parent)
    : QObject(parent)
    , m_isUploading(false)
    , m_isPaused(false)
    , m_fileSize(0)
    , m_uploadedBytes(0)
    , m_lastUploadedBytes(0)
    , m_currentSpeed(0)
#ifdef ENABLE_OSS_SDK
    , m_ossClient(nullptr)
#endif
{
    m_speedTimer = new QTimer(this);
    m_speedTimer->setInterval(1000);  // 每秒更新一次速度
    connect(m_speedTimer, &QTimer::timeout, this, &OSSUploader::onSpeedTimerTimeout);
}

OSSUploader::~OSSUploader()
{
#ifdef ENABLE_OSS_SDK
    if (m_ossClient) {
        delete m_ossClient;
    }
#endif
}

bool OSSUploader::isOSSSDKAvailable()
{
#ifdef ENABLE_OSS_SDK
    return true;
#else
    return false;
#endif
}

void OSSUploader::startUpload(const QString& filePath,
                              const QString& taskId,
                              const STSCredentials& credentials,
                              const UploadConfig& config)
{
#ifdef ENABLE_OSS_SDK
    if (m_isUploading) {
        qWarning() << "OSSUploader: 已经在上传中";
        return;
    }

    m_filePath = filePath;
    m_taskId = taskId;
    m_credentials = credentials;
    m_config = config;

    // 检查文件是否存在
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        emit uploadError("文件不存在: " + filePath);
        return;
    }

    m_fileSize = fileInfo.size();
    m_uploadedBytes = 0;
    m_lastUploadedBytes = 0;

    qDebug() << "OSSUploader: 开始上传文件" << filePath;
    qDebug() << "文件大小:" << m_fileSize << "字节" << "(" << (m_fileSize / 1024.0 / 1024.0) << "MB)";
    qDebug() << "OSS Endpoint:" << credentials.endpoint;
    qDebug() << "OSS Bucket:" << credentials.bucketName;
    qDebug() << "OSS Object Key:" << credentials.objectKey;

    // 创建 checkpoint 目录
    QDir checkpointDir(config.checkpointDir);
    if (!checkpointDir.exists()) {
        checkpointDir.mkpath(".");
    }

    // 初始化 OSS 客户端
    try {
        initializeOSSClient(credentials);
    } catch (const std::exception& e) {
        emit uploadError(QString("初始化 OSS 客户端失败: %1").arg(e.what()));
        return;
    }

    m_isUploading = true;
    m_isPaused = false;
    m_speedTimer->start();

    // 执行上传
    performUpload();
#else
    emit uploadError("OSS SDK 未启用，请使用 vcpkg 安装: vcpkg install aliyun-oss-cpp-sdk");
#endif
}

void OSSUploader::pause()
{
    m_isPaused = true;
    m_speedTimer->stop();
    qDebug() << "OSSUploader: 上传已暂停";
}

void OSSUploader::resume()
{
    if (!m_isPaused) {
        return;
    }

    m_isPaused = false;
    m_speedTimer->start();
    qDebug() << "OSSUploader: 上传已继续";

#ifdef ENABLE_OSS_SDK
    performUpload();
#endif
}

void OSSUploader::cancel()
{
    m_isUploading = false;
    m_isPaused = false;
    m_speedTimer->stop();

    qDebug() << "OSSUploader: 上传已取消";
    emit uploadFinished(false);
}

#ifdef ENABLE_OSS_SDK
void OSSUploader::initializeOSSClient(const STSCredentials& credentials)
{
    // 初始化 OSS SDK
    InitializeSdk();

    // 配置客户端
    ClientConfiguration conf;
    conf.connectTimeoutMs = 10000;  // 连接超时 10s
    conf.requestTimeoutMs = 120000; // 请求超时 120s (大文件)

    // 使用 STS 临时凭证创建客户端
    m_ossClient = new OssClient(
        credentials.endpoint.toStdString(),
        credentials.accessKeyId.toStdString(),
        credentials.accessKeySecret.toStdString(),
        credentials.securityToken.toStdString(),
        conf
    );

    qDebug() << "OSSUploader: OSS 客户端初始化成功";
}

void OSSUploader::performUpload()
{
    if (m_isPaused || !m_isUploading) {
        return;
    }

    qDebug() << "OSSUploader: 开始断点续传上传";
    qDebug() << "分片大小:" << (m_config.partSize / 1024.0 / 1024.0) << "MB";
    qDebug() << "并发数:" << m_config.threadNum;
    qDebug() << "Checkpoint 目录:" << m_config.checkpointDir;

    // 构建 checkpoint 文件路径
    QString checkpointPath = QString("%1/%2.checkpoint")
        .arg(m_config.checkpointDir)
        .arg(m_taskId);

    try {
        // 创建上传请求
        UploadObjectRequest request(
            m_credentials.bucketName.toStdString(),
            m_credentials.objectKey.toStdString(),
            m_filePath.toStdString(),
            checkpointPath.toStdString(),
            m_config.partSize,
            m_config.threadNum
        );

        // 设置进度回调
        TransferProgress transferProgress = {
            progressCallback,
            this
        };
        request.setTransferProgress(transferProgress);

        qDebug() << "OSSUploader: 调用 ResumableUploadObject...";

        // 执行断点续传上传
        auto outcome = m_ossClient->ResumableUploadObject(request);

        if (outcome.isSuccess()) {
            qDebug() << "OSSUploader: 文件上传成功!";
            qDebug() << "ETag:" << QString::fromStdString(outcome.result().ETag());

            m_isUploading = false;
            m_speedTimer->stop();

            // 删除 checkpoint 文件
            QFile::remove(checkpointPath);

            emit uploadFinished(true);
        } else {
            // 上传失败
            QString errorMsg = QString("上传失败: %1 (错误码: %2)")
                .arg(QString::fromStdString(outcome.error().Message()))
                .arg(QString::fromStdString(outcome.error().Code()));

            qWarning() << "OSSUploader:" << errorMsg;

            m_isUploading = false;
            m_speedTimer->stop();

            emit uploadError(errorMsg);
            emit uploadFinished(false);
        }

    } catch (const std::exception& e) {
        QString errorMsg = QString("上传异常: %1").arg(e.what());
        qWarning() << "OSSUploader:" << errorMsg;

        m_isUploading = false;
        m_speedTimer->stop();

        emit uploadError(errorMsg);
        emit uploadFinished(false);
    }
}

void OSSUploader::progressCallback(size_t increment, int64_t transfered,
                                   int64_t total, void* userData)
{
    OSSUploader* uploader = static_cast<OSSUploader*>(userData);
    if (!uploader) {
        return;
    }

    uploader->m_uploadedBytes = transfered;

    // 计算进度百分比
    int progress = 0;
    if (total > 0) {
        progress = static_cast<int>((transfered * 100) / total);
    }

    // 发送进度信号
    emit uploader->progressChanged(progress, transfered, total);

    // 输出日志（降低频率，仅在进度变化时输出）
    static int lastProgress = -1;
    if (progress != lastProgress && progress % 5 == 0) {
        qDebug() << "OSSUploader: 上传进度:" << progress << "%"
                 << "(" << (transfered / 1024.0 / 1024.0) << "MB"
                 << "/" << (total / 1024.0 / 1024.0) << "MB)";
        lastProgress = progress;
    }
}

void OSSUploader::saveCheckpoint()
{
    // 保存上传状态到 JSON 文件（用于自定义恢复逻辑）
    QJsonObject checkpoint;
    checkpoint["taskId"] = m_taskId;
    checkpoint["filePath"] = m_filePath;
    checkpoint["fileSize"] = m_fileSize;
    checkpoint["uploadedBytes"] = m_uploadedBytes;
    checkpoint["objectKey"] = m_credentials.objectKey;

    QString checkpointPath = QString("%1/%2_state.json")
        .arg(m_config.checkpointDir)
        .arg(m_taskId);

    QFile file(checkpointPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(checkpoint).toJson());
        file.close();
    }
}

void OSSUploader::loadCheckpoint()
{
    QString checkpointPath = QString("%1/%2_state.json")
        .arg(m_config.checkpointDir)
        .arg(m_taskId);

    QFile file(checkpointPath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();

        QJsonObject checkpoint = doc.object();
        m_uploadedBytes = checkpoint["uploadedBytes"].toVariant().toLongLong();

        qDebug() << "OSSUploader: 从 checkpoint 恢复，已上传:" << m_uploadedBytes << "字节";
    }
}

int OSSUploader::calculateOptimalConcurrency()
{
    // 根据当前上传速度动态调整并发数
    double speedMBps = m_currentSpeed / 1024.0 / 1024.0;

    if (speedMBps > 10.0) {
        return 8;  // 高速网络
    } else if (speedMBps > 5.0) {
        return 5;  // 中速网络
    } else if (speedMBps > 1.0) {
        return 3;  // 低速网络
    } else {
        return 1;  // 极慢网络
    }
}
#endif

void OSSUploader::onSpeedTimerTimeout()
{
    qint64 uploadedSinceLastCheck = m_uploadedBytes - m_lastUploadedBytes;
    m_currentSpeed = uploadedSinceLastCheck;  // 每秒字节数

    m_lastUploadedBytes = m_uploadedBytes;

    emit speedChanged(m_currentSpeed);

    // 估算剩余时间
    if (m_currentSpeed > 0) {
        qint64 remainingBytes = m_fileSize - m_uploadedBytes;
        qint64 remainingSeconds = remainingBytes / m_currentSpeed;
        qDebug() << "OSSUploader: 速度:" << (m_currentSpeed / 1024) << "KB/s, 预计剩余时间:"
                 << remainingSeconds << "秒";
    }
}
