/**
 * @file AuthManager.cpp
 * @brief 认证管理器实现
 */

#include "AuthManager.h"
#include "../core/Config.h"
#include "../core/Logger.h"
#include "../core/Application.h"
#include "../network/HttpClient.h"
#include <QSettings>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>

AuthManager::AuthManager(QObject *parent)
    : QObject(parent)
    , m_currentUser(nullptr)
    , m_isLoggedIn(false)
    , m_rememberMe(false)
    , m_tokenRefreshTimer(new QTimer(this))
{
    // 配置 Token 刷新定时器
    m_tokenRefreshTimer->setInterval(4 * 60 * 1000);  // 4 分钟检查一次
    connect(m_tokenRefreshTimer, &QTimer::timeout, this, &AuthManager::autoRefreshToken);
}

AuthManager::~AuthManager()
{
    if (m_currentUser) {
        delete m_currentUser;
        m_currentUser = nullptr;
    }
}

AuthManager& AuthManager::instance()
{
    static AuthManager instance;
    return instance;
}

void AuthManager::initialize()
{
    Application::instance().logger()->info("AuthManager", QString::fromUtf8("初始化认证管理器"));

    // 加载本地保存的认证数据
    loadAuthData();

    // 如果有保存的登录状态，尝试验证 Token
    if (m_isLoggedIn && !m_accessToken.isEmpty()) {
        if (isTokenExpired()) {
            Application::instance().logger()->warning("AuthManager", QString::fromUtf8("Token 已过期，尝试刷新"));
            refreshToken();
        } else {
            Application::instance().logger()->info("AuthManager", QString::fromUtf8("自动登录成功"));
            emit authStateChanged(true);
            emit loginSuccess(m_currentUser);
            startTokenRefreshTimer();
        }
    }
}

void AuthManager::login(const QString& username, const QString& password, bool remember)
{
    Application::instance().logger()->info("AuthManager", QString::fromUtf8("尝试登录: %1").arg(username));

    m_rememberMe = remember;

    // 调用 API 登录
    ApiService::instance().login(
        username,
        password,
        [this, remember](const QJsonObject& response) {
            // 登录成功，解析响应
            QString accessToken = response["access_token"].toString();
            QString refreshToken = response["refresh_token"].toString();
            int expiresIn = response["expires_in"].toInt(3600);  // 默认 1 小时

            // 设置 Token
            setTokens(accessToken, refreshToken);
            m_tokenExpireTime = QDateTime::currentDateTime().addSecs(expiresIn);

            // 获取用户信息
            QJsonObject userJson = response["user"].toObject();
            User* user = User::fromJson(userJson, this);
            user->setIsLoggedIn(true);
            setCurrentUser(user);

            // 更新登录状态
            setLoggedIn(true);

            // 保存登录状态（如果勾选记住我）
            if (remember) {
                saveAuthData();
            }

            // 启动 Token 刷新定时器
            startTokenRefreshTimer();

            Application::instance().logger()->info("AuthManager", QString::fromUtf8("登录成功: %1").arg(user->username()));
            emit loginSuccess(m_currentUser);
        },
        [this](int statusCode, const QString& error) {
            Application::instance().logger()->error("AuthManager", QString::fromUtf8("登录失败: %1").arg(error));
            emit loginFailed(error);
        }
    );
}

void AuthManager::sendVerificationCode(const QString& phone)
{
    Application::instance().logger()->info("AuthManager", QString::fromUtf8("发送验证码到: %1").arg(phone));

    // 调用 API 发送验证码
    ApiService::instance().sendVerificationCode(
        phone,
        [this, phone](const QJsonObject& response) {
            Application::instance().logger()->info("AuthManager", QString::fromUtf8("验证码发送成功: %1").arg(phone));
            // 发送成功不需要特殊处理，RegisterDialog 会自动开始倒计时
        },
        [this](int statusCode, const QString& error) {
            Application::instance().logger()->error("AuthManager", QString::fromUtf8("验证码发送失败: %1").arg(error));
            // TODO: 可以添加一个专门的信号来通知验证码发送失败
        }
    );
}

void AuthManager::registerUser(const QString& username, const QString& phone,
                               const QString& verificationCode, const QString& password)
{
    Application::instance().logger()->info("AuthManager", QString::fromUtf8("尝试注册: %1 (手机: %2)").arg(username, phone));

    // 调用 API 注册
    ApiService::instance().registerUser(
        username,
        phone,
        verificationCode,
        password,
        [this, username](const QJsonObject& response) {
            Application::instance().logger()->info("AuthManager", QString::fromUtf8("注册成功: %1").arg(username));
            emit registerSuccess();
        },
        [this](int statusCode, const QString& error) {
            Application::instance().logger()->error("AuthManager", QString::fromUtf8("注册失败: %1").arg(error));
            emit registerFailed(error);
        }
    );
}

void AuthManager::logout()
{
    Application::instance().logger()->info("AuthManager", QString::fromUtf8("用户登出"));

    // 停止 Token 刷新定时器
    stopTokenRefreshTimer();

    // 清除认证数据
    clearAuthData();

    // 更新登录状态
    setLoggedIn(false);

    // 通知后端（可选）
    if (!m_accessToken.isEmpty()) {
        ApiService::instance().logout(
            [this](const QJsonObject& response) {
                Application::instance().logger()->info("AuthManager", QString::fromUtf8("服务器端登出成功"));
            },
            [this](int statusCode, const QString& error) {
                Application::instance().logger()->warning("AuthManager", QString::fromUtf8("服务器端登出失败: %1").arg(error));
            }
        );
    }

    emit logoutSuccess();
}

void AuthManager::refreshToken()
{
    if (m_refreshToken.isEmpty()) {
        Application::instance().logger()->error("AuthManager", QString::fromUtf8("刷新 Token 失败: 没有 refresh token"));
        emit tokenRefreshFailed();
        return;
    }

    Application::instance().logger()->info("AuthManager", QString::fromUtf8("刷新访问令牌"));

    ApiService::instance().refreshToken(
        m_refreshToken,
        [this](const QJsonObject& response) {
            // 刷新成功，更新 Token
            QString accessToken = response["access_token"].toString();
            QString refreshToken = response["refresh_token"].toString();
            int expiresIn = response["expires_in"].toInt(3600);

            setTokens(accessToken, refreshToken);
            m_tokenExpireTime = QDateTime::currentDateTime().addSecs(expiresIn);

            // 保存更新后的认证数据
            if (m_rememberMe) {
                saveAuthData();
            }

            Application::instance().logger()->info("AuthManager", QString::fromUtf8("Token 刷新成功"));
            emit tokenRefreshed();
        },
        [this](int statusCode, const QString& error) {
            Application::instance().logger()->error("AuthManager", QString::fromUtf8("Token 刷新失败: %1").arg(error));

            // Token 刷新失败，需要重新登录
            logout();
            emit tokenRefreshFailed();
        }
    );
}

bool AuthManager::isTokenExpired() const
{
    if (!m_tokenExpireTime.isValid()) {
        return true;
    }
    return QDateTime::currentDateTime() >= m_tokenExpireTime;
}

bool AuthManager::isTokenExpiringSoon() const
{
    if (!m_tokenExpireTime.isValid()) {
        return true;
    }
    // 检查是否在 5 分钟内过期
    return QDateTime::currentDateTime().addSecs(5 * 60) >= m_tokenExpireTime;
}

void AuthManager::clearAuthData()
{
    m_accessToken.clear();
    m_refreshToken.clear();
    m_tokenExpireTime = QDateTime();

    if (m_currentUser) {
        m_currentUser->clear();
    }

    // 清除本地存储
    QSettings settings;
    settings.beginGroup("Auth");
    settings.remove("");
    settings.endGroup();
}

void AuthManager::saveAuthData()
{
    if (!m_rememberMe) {
        return;
    }

    QSettings settings;
    settings.beginGroup("Auth");
    settings.setValue("access_token", m_accessToken);
    settings.setValue("refresh_token", m_refreshToken);
    settings.setValue("token_expire_time", m_tokenExpireTime);
    settings.setValue("remember_me", m_rememberMe);

    // 保存用户信息
    if (m_currentUser) {
        QJsonObject userJson = m_currentUser->toJson();
        QJsonDocument doc(userJson);
        settings.setValue("user_data", QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    }

    settings.endGroup();
    settings.sync();

    Application::instance().logger()->debug("AuthManager", QString::fromUtf8("认证数据已保存"));
}

void AuthManager::loadAuthData()
{
    QSettings settings;
    settings.beginGroup("Auth");

    m_rememberMe = settings.value("remember_me", false).toBool();

    if (!m_rememberMe) {
        settings.endGroup();
        return;
    }

    m_accessToken = settings.value("access_token").toString();
    m_refreshToken = settings.value("refresh_token").toString();
    m_tokenExpireTime = settings.value("token_expire_time").toDateTime();

    // 加载用户信息
    QString userDataStr = settings.value("user_data").toString();
    if (!userDataStr.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(userDataStr.toUtf8());
        if (!doc.isNull()) {
            m_currentUser = User::fromJson(doc.object(), this);
            m_isLoggedIn = true;
        }
    }

    settings.endGroup();

    if (!m_accessToken.isEmpty()) {
        Application::instance().logger()->info("AuthManager", QString::fromUtf8("从本地加载认证数据"));
    }
}

void AuthManager::setTokens(const QString& accessToken, const QString& refreshToken)
{
    m_accessToken = accessToken;
    m_refreshToken = refreshToken;

    // 更新 HttpClient 的 Token
    HttpClient::instance().setAccessToken(accessToken);
}

void AuthManager::setCurrentUser(User* user)
{
    if (m_currentUser && m_currentUser != user) {
        delete m_currentUser;
    }
    m_currentUser = user;
    emit userInfoUpdated();
}

void AuthManager::setLoggedIn(bool loggedIn)
{
    if (m_isLoggedIn != loggedIn) {
        m_isLoggedIn = loggedIn;
        emit authStateChanged(loggedIn);
    }
}

void AuthManager::startTokenRefreshTimer()
{
    if (!m_tokenRefreshTimer->isActive()) {
        m_tokenRefreshTimer->start();
        Application::instance().logger()->debug("AuthManager", QString::fromUtf8("启动 Token 刷新定时器"));
    }
}

void AuthManager::stopTokenRefreshTimer()
{
    if (m_tokenRefreshTimer->isActive()) {
        m_tokenRefreshTimer->stop();
        Application::instance().logger()->debug("AuthManager", QString::fromUtf8("停止 Token 刷新定时器"));
    }
}

void AuthManager::autoRefreshToken()
{
    // 检查 Token 是否即将过期
    if (isTokenExpiringSoon()) {
        Application::instance().logger()->info("AuthManager", QString::fromUtf8("Token 即将过期，自动刷新"));
        refreshToken();
    }
}
