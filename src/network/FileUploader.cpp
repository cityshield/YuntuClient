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
    , m_maxConcurrency(1)  // 降低并发数到1，避免内存峰值
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
    qDebug() << "文件大小:" << m_fileSize << "字节" << "(" << (m_fileSize / 1024.0 / 1024.0) << "MB)";

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
    qDebug() << "FileUploader: 每片大小:" << (m_chunkSize / 1024.0 / 1024.0) << "MB";
    qDebug() << "FileUploader: 并发数:" << m_maxConcurrency;
    qDebug() << "FileUploader: 预计峰值内存:" << (m_maxConcurrency * 0.5) << "MB (流式上传，已优化)";
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
            if (!m_chunks[i].uploaded) {
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

    qDebug() << "FileUploader: 开始上传分片" << chunkIndex << "/" << m_chunks.size()
             << "偏移:" << chunk.offset << "大小:" << chunk.size;

    m_uploadingCount++;

    // 使用 QtConcurrent 在后台线程计算 MD5（只需读取一次，不需要全部加载到内存）
    QFuture<QByteArray> future = QtConcurrent::run([this, chunk]() -> QByteArray {
        QFile file(m_filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            return QByteArray();
        }

        if (!file.seek(chunk.offset)) {
            file.close();
            return QByteArray();
        }

        // 流式计算 MD5，每次读取 64KB
        QCryptographicHash hash(QCryptographicHash::Md5);
        qint64 remaining = chunk.size;
        const qint64 bufferSize = 64 * 1024;  // 64KB 缓冲区
        char buffer[bufferSize];

        while (remaining > 0) {
            qint64 toRead = qMin(bufferSize, remaining);
            qint64 bytesRead = file.read(buffer, toRead);
            if (bytesRead <= 0) {
                break;
            }
            hash.addData(buffer, bytesRead);
            remaining -= bytesRead;
        }

        file.close();
        return hash.result().toHex();
    });

    // 使用 QFutureWatcher 监听后台任务完成
    QFutureWatcher<QByteArray>* watcher = new QFutureWatcher<QByteArray>(this);

    // 按值捕获关键数据，避免引用失效
    qint64 chunkOffset = chunk.offset;
    qint64 chunkSize = chunk.size;

    connect(watcher, &QFutureWatcher<QByteArray>::finished, this, [this, chunkIndex, chunkOffset, chunkSize, watcher]() {
        QByteArray hash = watcher->result();
        watcher->deleteLater();

        if (hash.isEmpty()) {
            qWarning() << "FileUploader: 计算分片" << chunkIndex << "MD5失败";
            onChunkUploaded(chunkIndex, false);
            return;
        }

        qDebug() << "FileUploader: 分片" << chunkIndex << "MD5:" << QString::fromLatin1(hash);

        // 构造上传参数（使用 multipart/form-data 格式，流式上传）
        QMap<QString, QString> fields;
        fields["taskId"] = m_taskId;
        fields["chunkIndex"] = QString::number(chunkIndex);
        fields["totalChunks"] = QString::number(m_chunks.size());
        fields["chunkHash"] = QString::fromLatin1(hash);

        // 使用流式上传（不会将整个分片加载到内存）
        HttpClient::instance().uploadChunk(
            "/api/v1/files/upload/chunk",
            m_filePath,
            chunkOffset,
            chunkSize,
            fields,
            [this, chunkIndex](const QJsonObject& response) {
                // 上传成功
                qDebug() << "FileUploader: 分片" << chunkIndex << "上传成功";
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
