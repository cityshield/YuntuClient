#include "HttpClient.h"
#include <QUrlQuery>
#include <QHttpMultiPart>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QDebug>

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

void HttpClient::downloadFile(const QString& url,
                             const QString& savePath,
                             std::function<void(qint64, qint64)> onProgress,
                             std::function<void()> onSuccess,
                             ErrorCallback onError)
{
    QNetworkRequest request(QUrl(url));

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
