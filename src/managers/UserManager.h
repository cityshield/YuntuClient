/**
 * @file UserManager.h
 * @brief 用户管理器
 */

#ifndef USERMANAGER_H
#define USERMANAGER_H

#include <QObject>
#include <QString>
#include "../models/User.h"
#include "../network/ApiService.h"

/**
 * @brief 用户管理器
 *
 * 管理用户信息、账户操作等
 * 使用单例模式
 */
class UserManager : public QObject
{
    Q_OBJECT

public:
    static UserManager& instance();

    // 禁用拷贝构造和赋值
    UserManager(const UserManager&) = delete;
    UserManager& operator=(const UserManager&) = delete;

    /**
     * @brief 初始化用户管理器
     */
    void initialize();

    /**
     * @brief 获取当前用户
     */
    User* currentUser() const { return m_currentUser; }

    /**
     * @brief 设置当前用户
     */
    void setCurrentUser(User* user);

    /**
     * @brief 刷新用户信息
     */
    void refreshUserInfo();

    /**
     * @brief 更新用户资料
     * @param username 用户名
     * @param email 邮箱
     * @param phone 手机号
     * @param avatar 头像URL
     */
    void updateProfile(const QString& username, const QString& email,
                      const QString& phone, const QString& avatar);

    /**
     * @brief 修改密码
     * @param oldPassword 旧密码
     * @param newPassword 新密码
     */
    void changePassword(const QString& oldPassword, const QString& newPassword);

    /**
     * @brief 账户充值
     * @param amount 充值金额
     * @param paymentMethod 支付方式
     */
    void recharge(double amount, const QString& paymentMethod);

    /**
     * @brief 会员升级
     * @param level 目标会员等级
     */
    void upgradeMembership(MemberLevel level);

    /**
     * @brief 获取账户余额
     */
    double getBalance() const;

    /**
     * @brief 获取会员等级
     */
    MemberLevel getMemberLevel() const;

    /**
     * @brief 是否为付费会员
     */
    bool isPaidMember() const;

    /**
     * @brief 获取交易记录
     * @param page 页码
     * @param pageSize 每页大小
     */
    void fetchTransactions(int page = 1, int pageSize = 20);

    /**
     * @brief 获取账单记录
     * @param page 页码
     * @param pageSize 每页大小
     */
    void fetchBills(int page = 1, int pageSize = 20);

    /**
     * @brief 清除用户数据
     */
    void clearUserData();

signals:
    /**
     * @brief 用户信息更新信号
     */
    void userInfoUpdated();

    /**
     * @brief 用户资料更新成功信号
     */
    void profileUpdated();

    /**
     * @brief 用户资料更新失败信号
     */
    void profileUpdateFailed(const QString& error);

    /**
     * @brief 密码修改成功信号
     */
    void passwordChanged();

    /**
     * @brief 密码修改失败信号
     */
    void passwordChangeFailed(const QString& error);

    /**
     * @brief 充值成功信号
     */
    void rechargeSuccess(double amount);

    /**
     * @brief 充值失败信号
     */
    void rechargeFailed(const QString& error);

    /**
     * @brief 会员升级成功信号
     */
    void membershipUpgraded(MemberLevel level);

    /**
     * @brief 会员升级失败信号
     */
    void membershipUpgradeFailed(const QString& error);

    /**
     * @brief 余额变更信号
     */
    void balanceChanged(double newBalance);

    /**
     * @brief 会员等级变更信号
     */
    void memberLevelChanged(MemberLevel newLevel);

    /**
     * @brief 交易记录获取成功信号
     */
    void transactionsFetched(const QJsonArray& transactions);

    /**
     * @brief 账单记录获取成功信号
     */
    void billsFetched(const QJsonArray& bills);

private:
    explicit UserManager(QObject *parent = nullptr);
    ~UserManager();

    /**
     * @brief 更新用户余额
     */
    void updateBalance(double balance);

    /**
     * @brief 更新会员等级
     */
    void updateMemberLevel(MemberLevel level);

private:
    ApiService* m_apiService;
    User* m_currentUser;
};

#endif // USERMANAGER_H
