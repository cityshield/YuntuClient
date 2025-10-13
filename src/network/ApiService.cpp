#include "ApiService.h"
#include "HttpClient.h"
#include <QDebug>

ApiService& ApiService::instance()
{
    static ApiService instance;
    return instance;
}

ApiService::ApiService()
{
}

ApiService::~ApiService()
{
}

// =============== 认证相关 ===============

void ApiService::sendSmsCode(const QString& phone,
                            SuccessCallback onSuccess,
                            ErrorCallback onError)
{
    QJsonObject data;
    data["phone"] = phone;

    HttpClient::instance().post("/api/v1/auth/send-code", data, onSuccess, onError);
}

void ApiService::login(const QString& username,
                      const QString& password,
                      SuccessCallback onSuccess,
                      ErrorCallback onError)
{
    QJsonObject data;
    data["username"] = username;
    data["password"] = password;

    HttpClient::instance().post("/api/v1/auth/login", data, onSuccess, onError);
}

void ApiService::registerUser(const QString& username,
                             const QString& email,
                             const QString& password,
                             const QString& phone,
                             SuccessCallback onSuccess,
                             ErrorCallback onError)
{
    QJsonObject data;
    data["username"] = username;
    data["email"] = email;
    data["password"] = password;
    if (!phone.isEmpty()) {
        data["phone"] = phone;
    }

    HttpClient::instance().post("/api/v1/auth/register", data, onSuccess, onError);
}

void ApiService::loginWithPhone(const QString& phone,
                               const QString& code,
                               SuccessCallback onSuccess,
                               ErrorCallback onError)
{
    QJsonObject data;
    data["phone"] = phone;
    data["code"] = code;

    HttpClient::instance().post("/api/v1/auth/login-phone", data, onSuccess, onError);
}

void ApiService::loginWithWechat(const QString& code,
                                SuccessCallback onSuccess,
                                ErrorCallback onError)
{
    QJsonObject data;
    data["code"] = code;

    HttpClient::instance().post("/api/v1/auth/wechat-login", data, onSuccess, onError);
}

void ApiService::getCurrentUser(SuccessCallback onSuccess, ErrorCallback onError)
{
    HttpClient::instance().get("/api/v1/auth/me", {}, onSuccess, onError);
}

void ApiService::refreshToken(const QString& refreshToken,
                             SuccessCallback onSuccess,
                             ErrorCallback onError)
{
    QJsonObject data;
    data["refresh_token"] = refreshToken;

    HttpClient::instance().post("/api/v1/auth/refresh", data, onSuccess, onError);
}

void ApiService::logout(SuccessCallback onSuccess, ErrorCallback onError)
{
    HttpClient::instance().post("/api/v1/auth/logout", QJsonObject(), onSuccess, onError);
    HttpClient::instance().clearAccessToken();
    qDebug() << "ApiService: 用户已登出";
}

// =============== 用户相关 ===============

void ApiService::updateUserProfile(const QJsonObject& data,
                                  SuccessCallback onSuccess,
                                  ErrorCallback onError)
{
    HttpClient::instance().put("/api/v1/users/profile", data, onSuccess, onError);
}

void ApiService::getBalance(SuccessCallback onSuccess, ErrorCallback onError)
{
    HttpClient::instance().get("/api/v1/billing/balance", {}, onSuccess, onError);
}

void ApiService::getBillingRecords(const QString& startDate,
                                  const QString& endDate,
                                  SuccessCallback onSuccess,
                                  ErrorCallback onError)
{
    QMap<QString, QString> params;
    params["start_date"] = startDate;
    params["end_date"] = endDate;

    HttpClient::instance().get("/api/v1/billing/records", params, onSuccess, onError);
}

// =============== 任务相关 ===============

void ApiService::createTask(const QJsonObject& taskData,
                           SuccessCallback onSuccess,
                           ErrorCallback onError)
{
    HttpClient::instance().post("/api/v1/tasks", taskData, onSuccess, onError);
}

void ApiService::getTasks(const QString& status,
                         int skip,
                         int limit,
                         SuccessCallback onSuccess,
                         ErrorCallback onError)
{
    QMap<QString, QString> params;
    if (!status.isEmpty()) {
        params["status"] = status;
    }
    params["skip"] = QString::number(skip);
    params["limit"] = QString::number(limit);

    HttpClient::instance().get("/api/v1/tasks", params, onSuccess, onError);
}

void ApiService::getTask(const QString& taskId,
                        SuccessCallback onSuccess,
                        ErrorCallback onError)
{
    QString path = QString("/api/v1/tasks/%1").arg(taskId);
    HttpClient::instance().get(path, {}, onSuccess, onError);
}

void ApiService::pauseTask(const QString& taskId,
                          SuccessCallback onSuccess,
                          ErrorCallback onError)
{
    QString path = QString("/api/v1/tasks/%1/pause").arg(taskId);
    HttpClient::instance().put(path, QJsonObject(), onSuccess, onError);
}

void ApiService::resumeTask(const QString& taskId,
                           SuccessCallback onSuccess,
                           ErrorCallback onError)
{
    QString path = QString("/api/v1/tasks/%1/resume").arg(taskId);
    HttpClient::instance().put(path, QJsonObject(), onSuccess, onError);
}

void ApiService::cancelTask(const QString& taskId,
                           SuccessCallback onSuccess,
                           ErrorCallback onError)
{
    QString path = QString("/api/v1/tasks/%1/cancel").arg(taskId);
    HttpClient::instance().put(path, QJsonObject(), onSuccess, onError);
}

void ApiService::deleteTask(const QString& taskId,
                           bool deleteCloudData,
                           SuccessCallback onSuccess,
                           ErrorCallback onError)
{
    QMap<QString, QString> params;
    params["delete_cloud_data"] = deleteCloudData ? "true" : "false";

    QString path = QString("/api/v1/tasks/%1").arg(taskId);
    HttpClient::instance().deleteRequest(path, onSuccess, onError);
}

void ApiService::getTaskLogs(const QString& taskId,
                            int skip,
                            int limit,
                            SuccessCallback onSuccess,
                            ErrorCallback onError)
{
    QMap<QString, QString> params;
    params["skip"] = QString::number(skip);
    params["limit"] = QString::number(limit);

    QString path = QString("/api/v1/tasks/%1/logs").arg(taskId);
    HttpClient::instance().get(path, params, onSuccess, onError);
}

// =============== 文件相关 ===============

void ApiService::generateDownloadUrl(const QString& taskId,
                                    const QString& fileName,
                                    SuccessCallback onSuccess,
                                    ErrorCallback onError)
{
    QString path = QString("/api/v1/files/download/%1/%2").arg(taskId).arg(fileName);
    HttpClient::instance().get(path, {}, onSuccess, onError);
}
