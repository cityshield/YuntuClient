#include "FileUploader.h"
#include "HttpClient.h"
#include <QFileInfo>
#include <QCryptographicHash>
#include <QTimer>
#include <QDebug>

FileUploader::FileUploader(QObject *parent)
    : QObject(parent)
    , m_file(nullptr)
    , m_fileSize(0)
    , m_chunkSize(5 * 1024 * 1024)  // 默认5MB
    , m_maxConcurrency(3)  // 默认3个并发
    , m_maxRetries(3)  // 默认重试3次
    , m_uploadingCount(0)
    , m_completedCount(0)
    , m_isUploading(false)
    , m_isPaused(false)
    , m_uploadedBytes(0)
    , m_lastUploadedBytes(0)
    , m_currentSpeed(0)
{
    m_speedTimer = new QTimer(this);
    m_speedTimer->setInterval(1000);  // 每秒更新一次速度
    connect(m_speedTimer, &QTimer::timeout, this, &FileUploader::onSpeedTimerTimeout);
}

FileUploader::~FileUploader()
{
    if (m_file) {
        m_file->close();
        delete m_file;
    }
}

void FileUploader::startUpload(const QString& filePath, const QString& taskId)
{
    if (m_isUploading) {
        qWarning() << "FileUploader: 已经在上传中";
        return;
    }

    m_filePath = filePath;
    m_taskId = taskId;

    // 打开文件
    m_file = new QFile(filePath);
    if (!m_file->open(QIODevice::ReadOnly)) {
        emit uploadError("无法打开文件: " + filePath);
        delete m_file;
        m_file = nullptr;
        return;
    }

    m_fileSize = m_file->size();
    m_uploadedBytes = 0;
    m_lastUploadedBytes = 0;

    qDebug() << "FileUploader: 开始上传文件" << filePath;
    qDebug() << "文件大小:" << m_fileSize << "字节";

    // 准备分片
    prepareChunks();

    m_isUploading = true;
    m_isPaused = false;
    m_speedTimer->start();

    // 开始上传
    uploadNextChunk();
}

void FileUploader::pause()
{
    m_isPaused = true;
    m_speedTimer->stop();
    qDebug() << "FileUploader: 上传已暂停";
}

void FileUploader::resume()
{
    if (!m_isPaused) {
        return;
    }

    m_isPaused = false;
    m_speedTimer->start();
    qDebug() << "FileUploader: 上传已继续";

    uploadNextChunk();
}

void FileUploader::cancel()
{
    m_isUploading = false;
    m_isPaused = false;
    m_speedTimer->stop();

    if (m_file) {
        m_file->close();
        delete m_file;
        m_file = nullptr;
    }

    qDebug() << "FileUploader: 上传已取消";
    emit uploadFinished(false);
}

void FileUploader::prepareChunks()
{
    m_chunks.clear();
    m_completedCount = 0;
    m_uploadingCount = 0;

    int totalChunks = (m_fileSize + m_chunkSize - 1) / m_chunkSize;

    for (int i = 0; i < totalChunks; ++i) {
        ChunkInfo chunk;
        chunk.index = i;
        chunk.offset = i * m_chunkSize;
        chunk.size = qMin(m_chunkSize, m_fileSize - chunk.offset);
        chunk.uploaded = false;
        chunk.retryCount = 0;

        m_chunks.append(chunk);
    }

    qDebug() << "FileUploader: 分片数量:" << totalChunks;
}

void FileUploader::uploadNextChunk()
{
    if (m_isPaused || !m_isUploading) {
        return;
    }

    // 检查是否所有分片都已完成
    if (m_completedCount >= m_chunks.size()) {
        // 所有分片上传完成，合并文件
        mergeChunks();
        return;
    }

    // 上传下一个分片（限制并发数）
    while (m_uploadingCount < m_maxConcurrency && m_completedCount + m_uploadingCount < m_chunks.size()) {
        // 查找下一个未上传的分片
        for (int i = 0; i < m_chunks.size(); ++i) {
            if (!m_chunks[i].uploaded && m_chunkData.contains(i) == false) {
                uploadChunk(i);
                break;
            }
        }
    }
}

void FileUploader::uploadChunk(int chunkIndex)
{
    if (chunkIndex < 0 || chunkIndex >= m_chunks.size()) {
        return;
    }

    const ChunkInfo& chunk = m_chunks[chunkIndex];

    qDebug() << "FileUploader: 准备上传分片" << chunkIndex << "/" << m_chunks.size();

    m_uploadingCount++;

    // 使用 QtConcurrent 在后台线程读取文件和计算 MD5
    // 这样不会阻塞 UI 线程
    QFuture<QPair<QByteArray, QByteArray>> future = QtConcurrent::run([this, chunk]() -> QPair<QByteArray, QByteArray> {
        // 在后台线程中执行耗时操作
        QFile file(m_filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            return QPair<QByteArray, QByteArray>();
        }

        file.seek(chunk.offset);
        QByteArray chunkData = file.read(chunk.size);
        file.close();

        // 计算 MD5
        QByteArray hash = QCryptographicHash::hash(chunkData, QCryptographicHash::Md5).toHex();

        return QPair<QByteArray, QByteArray>(chunkData, hash);
    });

    // 使用 QFutureWatcher 监听后台任务完成
    QFutureWatcher<QPair<QByteArray, QByteArray>>* watcher = new QFutureWatcher<QPair<QByteArray, QByteArray>>(this);

    connect(watcher, &QFutureWatcher<QPair<QByteArray, QByteArray>>::finished, this, [this, chunkIndex, watcher]() {
        QPair<QByteArray, QByteArray> result = watcher->result();
        QByteArray chunkData = result.first;
        QByteArray hash = result.second;

        watcher->deleteLater();

        if (chunkData.isEmpty()) {
            qWarning() << "FileUploader: 读取分片" << chunkIndex << "失败";
            onChunkUploaded(chunkIndex, false);
            return;
        }

        qDebug() << "FileUploader: 开始上传分片" << chunkIndex << "/" << m_chunks.size();

        m_chunkData[chunkIndex] = chunkData;

        // 构造上传参数
        QMap<QString, QString> fields;
        fields["taskId"] = m_taskId;
        fields["chunkIndex"] = QString::number(chunkIndex);
        fields["totalChunks"] = QString::number(m_chunks.size());
        fields["chunkHash"] = QString::fromLatin1(hash);

        // TODO: 实际上传应该使用临时文件而不是内存中的数据
        // 这里简化处理，实际项目中需要改进

        HttpClient::instance().post(
            "/api/v1/files/upload/chunk",
            QJsonObject(),
            [this, chunkIndex](const QJsonObject& response) {
                // 上传成功
                onChunkUploaded(chunkIndex, true);
            },
            [this, chunkIndex](int statusCode, const QString& error) {
                // 上传失败
                qWarning() << "FileUploader: 分片" << chunkIndex << "上传失败:" << error;
                onChunkUploaded(chunkIndex, false);
            }
        );
    });

    watcher->setFuture(future);
}

void FileUploader::onChunkUploaded(int chunkIndex, bool success)
{
    m_uploadingCount--;

    if (success) {
        // 分片上传成功
        m_chunks[chunkIndex].uploaded = true;
        m_completedCount++;

        qint64 chunkSize = m_chunks[chunkIndex].size;
        m_uploadedBytes += chunkSize;

        // 清理分片数据
        m_chunkData.remove(chunkIndex);

        updateProgress();

        qDebug() << "FileUploader: 分片" << chunkIndex << "上传成功，进度:"
                 << m_completedCount << "/" << m_chunks.size();

    } else {
        // 分片上传失败，检查重试
        m_chunks[chunkIndex].retryCount++;

        if (m_chunks[chunkIndex].retryCount < m_maxRetries) {
            qDebug() << "FileUploader: 分片" << chunkIndex << "重试"
                     << m_chunks[chunkIndex].retryCount << "/" << m_maxRetries;
        } else {
            // 超过最大重试次数
            qWarning() << "FileUploader: 分片" << chunkIndex << "上传失败，超过最大重试次数";
            m_isUploading = false;
            m_speedTimer->stop();
            emit uploadError("分片上传失败");
            emit uploadFinished(false);
            return;
        }
    }

    // 继续上传下一个分片
    uploadNextChunk();
}

void FileUploader::mergeChunks()
{
    qDebug() << "FileUploader: 所有分片上传完成，请求合并文件";

    QJsonObject data;
    data["taskId"] = m_taskId;
    data["fileName"] = QFileInfo(m_filePath).fileName();
    data["totalChunks"] = m_chunks.size();
    data["fileSize"] = m_fileSize;

    HttpClient::instance().post(
        "/api/v1/files/upload/merge",
        data,
        [this](const QJsonObject& response) {
            // 合并成功
            qDebug() << "FileUploader: 文件上传完成";
            m_isUploading = false;
            m_speedTimer->stop();

            if (m_file) {
                m_file->close();
                delete m_file;
                m_file = nullptr;
            }

            emit uploadFinished(true);
        },
        [this](int statusCode, const QString& error) {
            // 合并失败
            qWarning() << "FileUploader: 文件合并失败:" << error;
            m_isUploading = false;
            m_speedTimer->stop();
            emit uploadError("文件合并失败: " + error);
            emit uploadFinished(false);
        }
    );
}

void FileUploader::updateProgress()
{
    int progress = (m_uploadedBytes * 100) / m_fileSize;
    emit progressChanged(progress, m_uploadedBytes, m_fileSize);
}

void FileUploader::onSpeedTimerTimeout()
{
    calculateSpeed();
}

void FileUploader::calculateSpeed()
{
    qint64 uploadedSinceLastCheck = m_uploadedBytes - m_lastUploadedBytes;
    m_currentSpeed = uploadedSinceLastCheck;  // 每秒字节数

    m_lastUploadedBytes = m_uploadedBytes;

    emit speedChanged(m_currentSpeed);

    // 估算剩余时间
    if (m_currentSpeed > 0) {
        qint64 remainingBytes = m_fileSize - m_uploadedBytes;
        qint64 remainingSeconds = remainingBytes / m_currentSpeed;
        qDebug() << "FileUploader: 速度:" << (m_currentSpeed / 1024) << "KB/s, 预计剩余时间:"
                 << remainingSeconds << "秒";
    }
}
