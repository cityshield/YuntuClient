#include "HttpClient.h"
#include <QUrlQuery>
#include <QHttpMultiPart>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QDebug>
#include <QBuffer>

// 自定义 QIODevice 用于读取文件的特定区块
class FileChunkDevice : public QIODevice
{
public:
    FileChunkDevice(const QString& filePath, qint64 offset, qint64 size, QObject* parent = nullptr)
        : QIODevice(parent)
        , m_file(filePath)
        , m_offset(offset)
        , m_size(size)
        , m_pos(0)
    {
    }

    bool open(OpenMode mode) override
    {
        if (!m_file.open(QIODevice::ReadOnly)) {
            return false;
        }
        if (!m_file.seek(m_offset)) {
            m_file.close();
            return false;
        }
        m_pos = 0;
        return QIODevice::open(mode);
    }

    void close() override
    {
        m_file.close();
        QIODevice::close();
    }

    qint64 size() const override
    {
        return m_size;
    }

    qint64 bytesAvailable() const override
    {
        return m_size - m_pos + QIODevice::bytesAvailable();
    }

    bool isSequential() const override
    {
        return false;
    }

protected:
    qint64 readData(char* data, qint64 maxSize) override
    {
        qint64 remainingBytes = m_size - m_pos;
        if (remainingBytes <= 0) {
            return 0;  // EOF
        }

        qint64 bytesToRead = qMin(maxSize, remainingBytes);
        qint64 bytesRead = m_file.read(data, bytesToRead);

        if (bytesRead > 0) {
            m_pos += bytesRead;
        }

        return bytesRead;
    }

    qint64 writeData(const char*, qint64) override
    {
        return -1;  // 不支持写入
    }

private:
    QFile m_file;
    qint64 m_offset;
    qint64 m_size;
    qint64 m_pos;
};

HttpClient& HttpClient::instance()
{
    static HttpClient instance;
    return instance;
}

HttpClient::HttpClient()
    : m_manager(new QNetworkAccessManager(this))
    , m_timeout(30000)  // 默认30秒超时
{
}

HttpClient::~HttpClient()
{
}

void HttpClient::setBaseUrl(const QString& baseUrl)
{
    m_baseUrl = baseUrl;
    if (m_baseUrl.endsWith("/")) {
        m_baseUrl.chop(1);
    }
}

void HttpClient::setAccessToken(const QString& token)
{
    m_accessToken = token;
}

void HttpClient::clearAccessToken()
{
    m_accessToken.clear();
}

void HttpClient::get(const QString& path,
                    const QMap<QString, QString>& params,
                    SuccessCallback onSuccess,
                    ErrorCallback onError)
{
    QString url = buildUrl(path, params);
    QNetworkRequest request = buildRequest(path);
    request.setUrl(QUrl(url));

    emit requestStarted(url);

    QNetworkReply* reply = m_manager->get(request);
    handleReply(reply, onSuccess, onError);
}

void HttpClient::post(const QString& path,
                     const QJsonObject& data,
                     SuccessCallback onSuccess,
                     ErrorCallback onError)
{
    QString url = buildUrl(path);
    QNetworkRequest request = buildRequest(path);
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    emit requestStarted(url);

    QByteArray jsonData = QJsonDocument(data).toJson();
    QNetworkReply* reply = m_manager->post(request, jsonData);
    handleReply(reply, onSuccess, onError);
}

void HttpClient::put(const QString& path,
                    const QJsonObject& data,
                    SuccessCallback onSuccess,
                    ErrorCallback onError)
{
    QString url = buildUrl(path);
    QNetworkRequest request = buildRequest(path);
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    emit requestStarted(url);

    QByteArray jsonData = QJsonDocument(data).toJson();
    QNetworkReply* reply = m_manager->put(request, jsonData);
    handleReply(reply, onSuccess, onError);
}

void HttpClient::deleteRequest(const QString& path,
                              SuccessCallback onSuccess,
                              ErrorCallback onError)
{
    QString url = buildUrl(path);
    QNetworkRequest request = buildRequest(path);
    request.setUrl(QUrl(url));

    emit requestStarted(url);

    QNetworkReply* reply = m_manager->deleteResource(request);
    handleReply(reply, onSuccess, onError);
}

void HttpClient::uploadFile(const QString& path,
                           const QString& filePath,
                           const QMap<QString, QString>& fields,
                           SuccessCallback onSuccess,
                           ErrorCallback onError,
                           std::function<void(qint64, qint64)> onProgress)
{
    QString url = buildUrl(path);
    QNetworkRequest request = buildRequest(path);
    request.setUrl(QUrl(url));

    // 创建 multipart 表单
    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // 添加表单字段
    for (auto it = fields.begin(); it != fields.end(); ++it) {
        QHttpPart textPart;
        textPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                          QVariant(QString("form-data; name=\"%1\"").arg(it.key())));
        textPart.setBody(it.value().toUtf8());
        multiPart->append(textPart);
    }

    // 添加文件
    QFile* file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        if (onError) {
            onError(-1, "无法打开文件");
        }
        delete file;
        delete multiPart;
        return;
    }

    QFileInfo fileInfo(filePath);
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                      QVariant(QString("form-data; name=\"file\"; filename=\"%1\"")
                              .arg(fileInfo.fileName())));
    filePart.setBodyDevice(file);
    file->setParent(multiPart);
    multiPart->append(filePart);

    emit requestStarted(url);

    QNetworkReply* reply = m_manager->post(request, multiPart);
    multiPart->setParent(reply);

    // 进度回调
    if (onProgress) {
        connect(reply, &QNetworkReply::uploadProgress, onProgress);
    }

    handleReply(reply, onSuccess, onError);
}

void HttpClient::uploadChunk(const QString& path,
                             const QString& filePath,
                             qint64 offset,
                             qint64 size,
                             const QMap<QString, QString>& fields,
                             SuccessCallback onSuccess,
                             ErrorCallback onError,
                             std::function<void(qint64, qint64)> onProgress)
{
    QString url = buildUrl(path);
    QNetworkRequest request = buildRequest(path);
    request.setUrl(QUrl(url));

    // 创建 multipart 表单
    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // 添加表单字段
    for (auto it = fields.begin(); it != fields.end(); ++it) {
        QHttpPart textPart;
        textPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                          QVariant(QString("form-data; name=\"%1\"").arg(it.key())));
        textPart.setBody(it.value().toUtf8());
        multiPart->append(textPart);
    }

    // 创建文件分片设备（流式读取，不加载全部内容到内存）
    FileChunkDevice* chunkDevice = new FileChunkDevice(filePath, offset, size);
    if (!chunkDevice->open(QIODevice::ReadOnly)) {
        if (onError) {
            onError(-1, "无法打开文件分片");
        }
        delete chunkDevice;
        delete multiPart;
        return;
    }

    QFileInfo fileInfo(filePath);
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                      QVariant(QString("form-data; name=\"chunkData\"; filename=\"%1\"")
                              .arg(fileInfo.fileName())));
    filePart.setBodyDevice(chunkDevice);
    chunkDevice->setParent(multiPart);  // 内存管理：multiPart 被删除时自动删除 chunkDevice
    multiPart->append(filePart);

    emit requestStarted(url);

    QNetworkReply* reply = m_manager->post(request, multiPart);
    multiPart->setParent(reply);  // 内存管理：reply 被删除时自动删除 multiPart

    // 进度回调
    if (onProgress) {
        connect(reply, &QNetworkReply::uploadProgress, onProgress);
    }

    handleReply(reply, onSuccess, onError);
}

void HttpClient::downloadFile(const QString& url,
                             const QString& savePath,
                             std::function<void(qint64, qint64)> onProgress,
                             std::function<void()> onSuccess,
                             ErrorCallback onError)
{
    QUrl requestUrl(url);
    QNetworkRequest request;
    request.setUrl(requestUrl);

    emit requestStarted(url);

    QNetworkReply* reply = m_manager->get(request);

    // 进度回调
    if (onProgress) {
        connect(reply, &QNetworkReply::downloadProgress, onProgress);
    }

    // 下载完成
    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QFile file(savePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(reply->readAll());
                file.close();

                if (onSuccess) {
                    onSuccess();
                }
                emit requestFinished(url, true);
            } else {
                if (onError) {
                    onError(-1, "无法写入文件");
                }
                emit requestFinished(url, false);
            }
        } else {
            if (onError) {
                onError(reply->error(), reply->errorString());
            }
            emit requestFinished(url, false);
        }

        reply->deleteLater();
    });
}

QNetworkRequest HttpClient::buildRequest(const QString& path)
{
    QNetworkRequest request;

    // 添加 User-Agent
    request.setRawHeader("User-Agent", "YuntuClient/1.0.0");

    // 添加 Authorization Token
    if (!m_accessToken.isEmpty()) {
        request.setRawHeader("Authorization",
                            QString("Bearer %1").arg(m_accessToken).toUtf8());
    }

    return request;
}

void HttpClient::handleReply(QNetworkReply* reply,
                            SuccessCallback onSuccess,
                            ErrorCallback onError)
{
    // 超时处理
    QTimer* timer = new QTimer(reply);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, [reply]() {
        reply->abort();
    });
    timer->start(m_timeout);

    // 请求完成
    connect(reply, &QNetworkReply::finished, [=]() {
        timer->stop();

        QString url = reply->url().toString();

        if (reply->error() == QNetworkReply::NoError) {
            // 成功
            QByteArray responseData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(responseData);

            if (onSuccess) {
                onSuccess(doc.object());
            }

            emit requestFinished(url, true);
        } else {
            // 错误
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QString errorString = reply->errorString();

            qDebug() << "HTTP Error:" << statusCode << errorString;

            if (onError) {
                onError(statusCode, errorString);
            }

            emit requestFinished(url, false);
        }

        reply->deleteLater();
    });
}

QString HttpClient::buildUrl(const QString& path, const QMap<QString, QString>& params)
{
    QString url = m_baseUrl + path;

    if (!params.isEmpty()) {
        QUrlQuery query;
        for (auto it = params.begin(); it != params.end(); ++it) {
            query.addQueryItem(it.key(), it.value());
        }
        url += "?" + query.toString();
    }

    return url;
}
