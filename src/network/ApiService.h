#pragma once

#include <QObject>
#include <QJsonObject>
#include <functional>

class User;
class Task;

/**
 * @brief API 服务封装类
 *
 * 封装所有后端 API 调用
 */
class ApiService : public QObject
{
    Q_OBJECT

public:
    using SuccessCallback = std::function<void(const QJsonObject&)>;
    using ErrorCallback = std::function<void(int, const QString&)>;

    static ApiService& instance();

    // =============== 认证相关 ===============

    /**
     * @brief 发送短信验证码
     */
    void sendSmsCode(const QString& phone,
                    SuccessCallback onSuccess = nullptr,
                    ErrorCallback onError = nullptr);

    /**
     * @brief 用户名密码登录
     */
    void login(const QString& username,
              const QString& password,
              SuccessCallback onSuccess = nullptr,
              ErrorCallback onError = nullptr);

    /**
     * @brief 用户注册
     */
    void registerUser(const QString& username,
                     const QString& email,
                     const QString& password,
                     const QString& phone,
                     SuccessCallback onSuccess = nullptr,
                     ErrorCallback onError = nullptr);

    /**
     * @brief 手机号验证码登录
     */
    void loginWithPhone(const QString& phone,
                       const QString& code,
                       SuccessCallback onSuccess = nullptr,
                       ErrorCallback onError = nullptr);

    /**
     * @brief 微信登录
     */
    void loginWithWechat(const QString& code,
                        SuccessCallback onSuccess = nullptr,
                        ErrorCallback onError = nullptr);

    /**
     * @brief 获取当前用户信息
     */
    void getCurrentUser(SuccessCallback onSuccess = nullptr,
                       ErrorCallback onError = nullptr);

    /**
     * @brief 刷新访问令牌
     */
    void refreshToken(const QString& refreshToken,
                     SuccessCallback onSuccess = nullptr,
                     ErrorCallback onError = nullptr);

    /**
     * @brief 登出
     */
    void logout(SuccessCallback onSuccess = nullptr,
               ErrorCallback onError = nullptr);

    // =============== 用户相关 ===============

    /**
     * @brief 更新用户资料
     */
    void updateUserProfile(const QJsonObject& data,
                          SuccessCallback onSuccess = nullptr,
                          ErrorCallback onError = nullptr);

    /**
     * @brief 获取余额
     */
    void getBalance(SuccessCallback onSuccess = nullptr,
                   ErrorCallback onError = nullptr);

    /**
     * @brief 获取费用明细
     */
    void getBillingRecords(const QString& startDate,
                          const QString& endDate,
                          SuccessCallback onSuccess = nullptr,
                          ErrorCallback onError = nullptr);

    // =============== 任务相关 ===============

    /**
     * @brief 创建任务
     */
    void createTask(const QJsonObject& taskData,
                   SuccessCallback onSuccess = nullptr,
                   ErrorCallback onError = nullptr);

    /**
     * @brief 获取任务列表
     */
    void getTasks(const QString& status = QString(),
                 int skip = 0,
                 int limit = 20,
                 SuccessCallback onSuccess = nullptr,
                 ErrorCallback onError = nullptr);

    /**
     * @brief 获取任务详情
     */
    void getTask(const QString& taskId,
                SuccessCallback onSuccess = nullptr,
                ErrorCallback onError = nullptr);

    /**
     * @brief 暂停任务
     */
    void pauseTask(const QString& taskId,
                  SuccessCallback onSuccess = nullptr,
                  ErrorCallback onError = nullptr);

    /**
     * @brief 继续任务
     */
    void resumeTask(const QString& taskId,
                   SuccessCallback onSuccess = nullptr,
                   ErrorCallback onError = nullptr);

    /**
     * @brief 取消任务
     */
    void cancelTask(const QString& taskId,
                   SuccessCallback onSuccess = nullptr,
                   ErrorCallback onError = nullptr);

    /**
     * @brief 删除任务
     */
    void deleteTask(const QString& taskId,
                   bool deleteCloudData,
                   SuccessCallback onSuccess = nullptr,
                   ErrorCallback onError = nullptr);

    /**
     * @brief 获取任务日志
     */
    void getTaskLogs(const QString& taskId,
                    int skip = 0,
                    int limit = 100,
                    SuccessCallback onSuccess = nullptr,
                    ErrorCallback onError = nullptr);

    // =============== 文件相关 ===============

    /**
     * @brief 生成文件下载URL
     */
    void generateDownloadUrl(const QString& taskId,
                            const QString& fileName,
                            SuccessCallback onSuccess = nullptr,
                            ErrorCallback onError = nullptr);

private:
    ApiService();
    ~ApiService();
    ApiService(const ApiService&) = delete;
    ApiService& operator=(const ApiService&) = delete;
};
