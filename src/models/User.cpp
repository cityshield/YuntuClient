/**
 * @file User.cpp
 * @brief 用户信息模型实现
 */

#include "User.h"

User::User(QObject *parent)
    : QObject(parent)
    , m_balance(0.0)
    , m_memberLevel(MemberLevel::Free)
    , m_isLoggedIn(false)
{
}

User::~User()
{
}

void User::setUserId(const QString &userId)
{
    if (m_userId != userId) {
        m_userId = userId;
        emit userIdChanged();
        emit userDataChanged();
    }
}

void User::setUsername(const QString &username)
{
    if (m_username != username) {
        m_username = username;
        emit usernameChanged();
        emit userDataChanged();
    }
}

void User::setEmail(const QString &email)
{
    if (m_email != email) {
        m_email = email;
        emit emailChanged();
        emit userDataChanged();
    }
}

void User::setPhone(const QString &phone)
{
    if (m_phone != phone) {
        m_phone = phone;
        emit phoneChanged();
        emit userDataChanged();
    }
}

void User::setAvatar(const QString &avatar)
{
    if (m_avatar != avatar) {
        m_avatar = avatar;
        emit avatarChanged();
        emit userDataChanged();
    }
}

void User::setBalance(double balance)
{
    if (qAbs(m_balance - balance) > 0.01) {
        m_balance = balance;
        emit balanceChanged();
        emit userDataChanged();
    }
}

void User::setMemberLevel(MemberLevel level)
{
    if (m_memberLevel != level) {
        m_memberLevel = level;
        emit memberLevelChanged();
        emit userDataChanged();
    }
}

void User::setCreatedAt(const QDateTime &time)
{
    m_createdAt = time;
}

void User::setLastLoginAt(const QDateTime &time)
{
    m_lastLoginAt = time;
}

void User::setIsLoggedIn(bool isLoggedIn)
{
    if (m_isLoggedIn != isLoggedIn) {
        m_isLoggedIn = isLoggedIn;
        emit isLoggedInChanged();
    }
}

QJsonObject User::toJson() const
{
    QJsonObject json;
    json["userId"] = m_userId;
    json["username"] = m_username;
    json["email"] = m_email;
    json["phone"] = m_phone;
    json["avatar"] = m_avatar;
    json["balance"] = m_balance;
    json["memberLevel"] = static_cast<int>(m_memberLevel);
    json["createdAt"] = m_createdAt.toString(Qt::ISODate);
    json["lastLoginAt"] = m_lastLoginAt.toString(Qt::ISODate);
    json["isLoggedIn"] = m_isLoggedIn;
    return json;
}

User* User::fromJson(const QJsonObject &json, QObject *parent)
{
    User *user = new User(parent);

    user->setUserId(json["userId"].toString());
    user->setUsername(json["username"].toString());
    user->setEmail(json["email"].toString());
    user->setPhone(json["phone"].toString());
    user->setAvatar(json["avatar"].toString());
    user->setBalance(json["balance"].toDouble());
    user->setMemberLevel(static_cast<MemberLevel>(json["memberLevel"].toInt()));

    QString createdAtStr = json["createdAt"].toString();
    if (!createdAtStr.isEmpty()) {
        user->setCreatedAt(QDateTime::fromString(createdAtStr, Qt::ISODate));
    }

    QString lastLoginAtStr = json["lastLoginAt"].toString();
    if (!lastLoginAtStr.isEmpty()) {
        user->setLastLoginAt(QDateTime::fromString(lastLoginAtStr, Qt::ISODate));
    }

    user->setIsLoggedIn(json["isLoggedIn"].toBool());

    return user;
}

QString User::memberLevelString() const
{
    switch (m_memberLevel) {
        case MemberLevel::Free:
            return QString::fromUtf8("免费用户");
        case MemberLevel::Basic:
            return QString::fromUtf8("基础会员");
        case MemberLevel::Pro:
            return QString::fromUtf8("专业会员");
        case MemberLevel::Enterprise:
            return QString::fromUtf8("企业会员");
        default:
            return QString::fromUtf8("未知");
    }
}

bool User::isPaidMember() const
{
    return m_memberLevel != MemberLevel::Free;
}

void User::clear()
{
    setUserId(QString());
    setUsername(QString());
    setEmail(QString());
    setPhone(QString());
    setAvatar(QString());
    setBalance(0.0);
    setMemberLevel(MemberLevel::Free);
    setIsLoggedIn(false);
    m_createdAt = QDateTime();
    m_lastLoginAt = QDateTime();
}
