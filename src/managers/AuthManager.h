/**
 * @file AuthManager.h
 * @brief 认证管理器
 */

#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include "../models/User.h"
#include "../network/ApiService.h"

/**
 * @brief 认证管理器
 *
 * 管理用户认证、登录状态、Token 管理等
 * 使用单例模式
 */
class AuthManager : public QObject
{
    Q_OBJECT

public:
    static AuthManager& instance();

    // 禁用拷贝构造和赋值
    AuthManager(const AuthManager&) = delete;
    AuthManager& operator=(const AuthManager&) = delete;

    /**
     * @brief 初始化认证管理器
     *
     * 检查本地存储的登录状态，尝试自动登录
     */
    void initialize();

    /**
     * @brief 用户登录
     * @param username 用户名或邮箱
     * @param password 密码
     * @param remember 是否记住登录状态
     */
    void login(const QString& username, const QString& password, bool remember = false);

    /**
     * @brief 用户注册
     * @param username 用户名
     * @param email 邮箱
     * @param password 密码
     * @param phone 手机号（可选）
     */
    void registerUser(const QString& username, const QString& email,
                     const QString& password, const QString& phone = QString());

    /**
     * @brief 用户登出
     */
    void logout();

    /**
     * @brief 刷新访问令牌
     */
    void refreshToken();

    /**
     * @brief 检查是否已登录
     */
    bool isLoggedIn() const { return m_isLoggedIn; }

    /**
     * @brief 获取当前用户
     */
    User* currentUser() const { return m_currentUser; }

    /**
     * @brief 获取访问令牌
     */
    QString accessToken() const { return m_accessToken; }

    /**
     * @brief 获取刷新令牌
     */
    QString refreshTokenValue() const { return m_refreshToken; }

    /**
     * @brief 检查 Token 是否过期
     */
    bool isTokenExpired() const;

    /**
     * @brief 检查 Token 是否即将过期（5分钟内）
     */
    bool isTokenExpiringSoon() const;

    /**
     * @brief 清除所有认证数据
     */
    void clearAuthData();

    /**
     * @brief 保存认证数据到本地
     */
    void saveAuthData();

    /**
     * @brief 从本地加载认证数据
     */
    void loadAuthData();

signals:
    /**
     * @brief 登录成功信号
     * @param user 用户信息
     */
    void loginSuccess(User* user);

    /**
     * @brief 登录失败信号
     * @param error 错误信息
     */
    void loginFailed(const QString& error);

    /**
     * @brief 注册成功信号
     */
    void registerSuccess();

    /**
     * @brief 注册失败信号
     * @param error 错误信息
     */
    void registerFailed(const QString& error);

    /**
     * @brief 登出成功信号
     */
    void logoutSuccess();

    /**
     * @brief Token 刷新成功信号
     */
    void tokenRefreshed();

    /**
     * @brief Token 刷新失败信号
     */
    void tokenRefreshFailed();

    /**
     * @brief 认证状态改变信号
     * @param isLoggedIn 是否已登录
     */
    void authStateChanged(bool isLoggedIn);

    /**
     * @brief 用户信息更新信号
     */
    void userInfoUpdated();

private:
    explicit AuthManager(QObject *parent = nullptr);
    ~AuthManager();

    /**
     * @brief 设置认证令牌
     */
    void setTokens(const QString& accessToken, const QString& refreshToken);

    /**
     * @brief 设置当前用户
     */
    void setCurrentUser(User* user);

    /**
     * @brief 更新登录状态
     */
    void setLoggedIn(bool loggedIn);

    /**
     * @brief 启动 Token 刷新定时器
     */
    void startTokenRefreshTimer();

    /**
     * @brief 停止 Token 刷新定时器
     */
    void stopTokenRefreshTimer();

    /**
     * @brief 自动刷新 Token
     */
    void autoRefreshToken();

private:
    ApiService* m_apiService;
    User* m_currentUser;

    QString m_accessToken;
    QString m_refreshToken;
    QDateTime m_tokenExpireTime;

    bool m_isLoggedIn;
    bool m_rememberMe;

    QTimer* m_tokenRefreshTimer;
};

#endif // AUTHMANAGER_H
