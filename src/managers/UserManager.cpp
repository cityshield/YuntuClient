/**
 * @file UserManager.cpp
 * @brief 用户管理器实现
 */

#include "UserManager.h"
#include "../core/Logger.h"
#include "../core/Application.h"
#include <QJsonObject>
#include <QJsonArray>

UserManager::UserManager(QObject *parent)
    : QObject(parent)
    , m_apiService(new ApiService(this))
    , m_currentUser(nullptr)
{
}

UserManager::~UserManager()
{
    if (m_currentUser) {
        delete m_currentUser;
        m_currentUser = nullptr;
    }
}

UserManager& UserManager::instance()
{
    static UserManager instance;
    return instance;
}

void UserManager::initialize()
{
    Application::instance().logger()->info("UserManager", QString::fromUtf8("初始化用户管理器"));
}

void UserManager::setCurrentUser(User* user)
{
    if (m_currentUser && m_currentUser != user) {
        delete m_currentUser;
    }
    m_currentUser = user;
    emit userInfoUpdated();
}

void UserManager::refreshUserInfo()
{
    if (!m_currentUser || m_currentUser->userId().isEmpty()) {
        Application::instance().logger()->warning("UserManager", QString::fromUtf8("无法刷新用户信息: 用户未登录"));
        return;
    }

    Application::instance().logger()->info("UserManager", QString::fromUtf8("刷新用户信息"));

    m_apiService->getUserInfo(
        [this](const QJsonObject& response) {
            // 更新用户信息
            if (m_currentUser) {
                m_currentUser->setUsername(response["username"].toString());
                m_currentUser->setEmail(response["email"].toString());
                m_currentUser->setPhone(response["phone"].toString());
                m_currentUser->setAvatar(response["avatar"].toString());
                m_currentUser->setBalance(response["balance"].toDouble());
                m_currentUser->setMemberLevel(static_cast<MemberLevel>(response["memberLevel"].toInt()));

                QString createdAtStr = response["createdAt"].toString();
                if (!createdAtStr.isEmpty()) {
                    m_currentUser->setCreatedAt(QDateTime::fromString(createdAtStr, Qt::ISODate));
                }

                QString lastLoginAtStr = response["lastLoginAt"].toString();
                if (!lastLoginAtStr.isEmpty()) {
                    m_currentUser->setLastLoginAt(QDateTime::fromString(lastLoginAtStr, Qt::ISODate));
                }
            }

            Application::instance().logger()->info("UserManager", QString::fromUtf8("用户信息刷新成功"));
            emit userInfoUpdated();
        },
        [this](int statusCode, const QString& error) {
            Application::instance().logger()->error("UserManager", QString::fromUtf8("刷新用户信息失败: %1").arg(error));
        }
    );
}

void UserManager::updateProfile(const QString& username, const QString& email,
                                const QString& phone, const QString& avatar)
{
    Application::instance().logger()->info("UserManager", QString::fromUtf8("更新用户资料"));

    QJsonObject profileData;
    profileData["username"] = username;
    profileData["email"] = email;
    profileData["phone"] = phone;
    profileData["avatar"] = avatar;

    m_apiService->updateProfile(
        profileData,
        [this, username, email, phone, avatar](const QJsonObject& response) {
            // 更新本地用户信息
            if (m_currentUser) {
                m_currentUser->setUsername(username);
                m_currentUser->setEmail(email);
                m_currentUser->setPhone(phone);
                m_currentUser->setAvatar(avatar);
            }

            Application::instance().logger()->info("UserManager", QString::fromUtf8("用户资料更新成功"));
            emit profileUpdated();
            emit userInfoUpdated();
        },
        [this](int statusCode, const QString& error) {
            Application::instance().logger()->error("UserManager", QString::fromUtf8("更新用户资料失败: %1").arg(error));
            emit profileUpdateFailed(error);
        }
    );
}

void UserManager::changePassword(const QString& oldPassword, const QString& newPassword)
{
    Application::instance().logger()->info("UserManager", QString::fromUtf8("修改密码"));

    m_apiService->changePassword(
        oldPassword,
        newPassword,
        [this](const QJsonObject& response) {
            Application::instance().logger()->info("UserManager", QString::fromUtf8("密码修改成功"));
            emit passwordChanged();
        },
        [this](int statusCode, const QString& error) {
            Application::instance().logger()->error("UserManager", QString::fromUtf8("修改密码失败: %1").arg(error));
            emit passwordChangeFailed(error);
        }
    );
}

void UserManager::recharge(double amount, const QString& paymentMethod)
{
    Application::instance().logger()->info("UserManager", QString::fromUtf8("账户充值: %1 元").arg(amount));

    QJsonObject rechargeData;
    rechargeData["amount"] = amount;
    rechargeData["paymentMethod"] = paymentMethod;

    // 假设有一个充值 API 接口
    // 这里暂时模拟实现
    HttpClient::instance().post(
        "/api/v1/user/recharge",
        rechargeData,
        [this, amount](const QJsonObject& response) {
            // 更新余额
            double newBalance = response["balance"].toDouble();
            updateBalance(newBalance);

            Application::instance().logger()->info("UserManager", QString::fromUtf8("充值成功: %1 元").arg(amount));
            emit rechargeSuccess(amount);
        },
        [this](int statusCode, const QString& error) {
            Application::instance().logger()->error("UserManager", QString::fromUtf8("充值失败: %1").arg(error));
            emit rechargeFailed(error);
        }
    );
}

void UserManager::upgradeMembership(MemberLevel level)
{
    Application::instance().logger()->info("UserManager", QString::fromUtf8("会员升级: %1").arg(static_cast<int>(level)));

    QJsonObject upgradeData;
    upgradeData["targetLevel"] = static_cast<int>(level);

    // 假设有一个会员升级 API 接口
    HttpClient::instance().post(
        "/api/v1/user/upgrade",
        upgradeData,
        [this, level](const QJsonObject& response) {
            // 更新会员等级
            updateMemberLevel(level);

            // 更新余额（扣费）
            double newBalance = response["balance"].toDouble();
            updateBalance(newBalance);

            Application::instance().logger()->info("UserManager", QString::fromUtf8("会员升级成功"));
            emit membershipUpgraded(level);
        },
        [this](int statusCode, const QString& error) {
            Application::instance().logger()->error("UserManager", QString::fromUtf8("会员升级失败: %1").arg(error));
            emit membershipUpgradeFailed(error);
        }
    );
}

double UserManager::getBalance() const
{
    return m_currentUser ? m_currentUser->balance() : 0.0;
}

MemberLevel UserManager::getMemberLevel() const
{
    return m_currentUser ? m_currentUser->memberLevel() : MemberLevel::Free;
}

bool UserManager::isPaidMember() const
{
    return m_currentUser ? m_currentUser->isPaidMember() : false;
}

void UserManager::fetchTransactions(int page, int pageSize)
{
    Application::instance().logger()->info("UserManager", QString::fromUtf8("获取交易记录: 第 %1 页").arg(page));

    QString endpoint = QString("/api/v1/user/transactions?page=%1&pageSize=%2").arg(page).arg(pageSize);

    HttpClient::instance().get(
        endpoint,
        {},
        [this](const QJsonObject& response) {
            QJsonArray transactions = response["transactions"].toArray();
            Application::instance().logger()->info("UserManager", QString::fromUtf8("获取交易记录成功: %1 条").arg(transactions.size()));
            emit transactionsFetched(transactions);
        },
        [this](int statusCode, const QString& error) {
            Application::instance().logger()->error("UserManager", QString::fromUtf8("获取交易记录失败: %1").arg(error));
        }
    );
}

void UserManager::fetchBills(int page, int pageSize)
{
    Application::instance().logger()->info("UserManager", QString::fromUtf8("获取账单记录: 第 %1 页").arg(page));

    QString endpoint = QString("/api/v1/user/bills?page=%1&pageSize=%2").arg(page).arg(pageSize);

    HttpClient::instance().get(
        endpoint,
        {},
        [this](const QJsonObject& response) {
            QJsonArray bills = response["bills"].toArray();
            Application::instance().logger()->info("UserManager", QString::fromUtf8("获取账单记录成功: %1 条").arg(bills.size()));
            emit billsFetched(bills);
        },
        [this](int statusCode, const QString& error) {
            Application::instance().logger()->error("UserManager", QString::fromUtf8("获取账单记录失败: %1").arg(error));
        }
    );
}

void UserManager::clearUserData()
{
    Application::instance().logger()->info("UserManager", QString::fromUtf8("清除用户数据"));

    if (m_currentUser) {
        m_currentUser->clear();
    }
}

void UserManager::updateBalance(double balance)
{
    if (m_currentUser) {
        m_currentUser->setBalance(balance);
        emit balanceChanged(balance);
        emit userInfoUpdated();
    }
}

void UserManager::updateMemberLevel(MemberLevel level)
{
    if (m_currentUser) {
        m_currentUser->setMemberLevel(level);
        emit memberLevelChanged(level);
        emit userInfoUpdated();
    }
}
