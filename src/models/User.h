/**
 * @file User.h
 * @brief 用户信息模型
 */

#ifndef USER_H
#define USER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QJsonObject>

/**
 * @brief 用户会员等级
 */
enum class MemberLevel {
    Free = 0,       // 免费用户
    Basic = 1,      // 基础会员
    Pro = 2,        // 专业会员
    Enterprise = 3  // 企业会员
};

/**
 * @brief 用户模型
 *
 * 存储用户基本信息、账户余额、会员等级等
 */
class User : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString userId READ userId WRITE setUserId NOTIFY userIdChanged)
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
    Q_PROPERTY(QString email READ email WRITE setEmail NOTIFY emailChanged)
    Q_PROPERTY(QString phone READ phone WRITE setPhone NOTIFY phoneChanged)
    Q_PROPERTY(QString avatar READ avatar WRITE setAvatar NOTIFY avatarChanged)
    Q_PROPERTY(double balance READ balance WRITE setBalance NOTIFY balanceChanged)
    Q_PROPERTY(MemberLevel memberLevel READ memberLevel WRITE setMemberLevel NOTIFY memberLevelChanged)
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn WRITE setIsLoggedIn NOTIFY isLoggedInChanged)

public:
    explicit User(QObject *parent = nullptr);
    ~User();

    // Getters
    QString userId() const { return m_userId; }
    QString username() const { return m_username; }
    QString email() const { return m_email; }
    QString phone() const { return m_phone; }
    QString avatar() const { return m_avatar; }
    double balance() const { return m_balance; }
    MemberLevel memberLevel() const { return m_memberLevel; }
    QDateTime createdAt() const { return m_createdAt; }
    QDateTime lastLoginAt() const { return m_lastLoginAt; }
    bool isLoggedIn() const { return m_isLoggedIn; }

    // Setters
    void setUserId(const QString &userId);
    void setUsername(const QString &username);
    void setEmail(const QString &email);
    void setPhone(const QString &phone);
    void setAvatar(const QString &avatar);
    void setBalance(double balance);
    void setMemberLevel(MemberLevel level);
    void setCreatedAt(const QDateTime &time);
    void setLastLoginAt(const QDateTime &time);
    void setIsLoggedIn(bool isLoggedIn);

    // 序列化/反序列化
    QJsonObject toJson() const;
    static User* fromJson(const QJsonObject &json, QObject *parent = nullptr);

    // 工具方法
    QString memberLevelString() const;
    bool isPaidMember() const;
    void clear();

signals:
    void userIdChanged();
    void usernameChanged();
    void emailChanged();
    void phoneChanged();
    void avatarChanged();
    void balanceChanged();
    void memberLevelChanged();
    void isLoggedInChanged();
    void userDataChanged();

private:
    QString m_userId;
    QString m_username;
    QString m_email;
    QString m_phone;
    QString m_avatar;
    double m_balance;
    MemberLevel m_memberLevel;
    QDateTime m_createdAt;
    QDateTime m_lastLoginAt;
    bool m_isLoggedIn;
};

#endif // USER_H
