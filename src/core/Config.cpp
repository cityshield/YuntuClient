#include "Config.h"
#include <QStandardPaths>

Config::Config(QObject *parent)
    : QObject(parent)
{
    m_settings = new QSettings(this);
}

Config::~Config()
{
    save();
}

void Config::load()
{
    m_settings->sync();
}

void Config::save()
{
    m_settings->sync();
}

// API配置
QString Config::apiBaseUrl() const
{
    return m_settings->value("api/baseUrl", "https://api.yuntucv.com").toString();
}

void Config::setApiBaseUrl(const QString &url)
{
    m_settings->setValue("api/baseUrl", url);
    emit configChanged();
}

QString Config::wsBaseUrl() const
{
    return m_settings->value("api/wsBaseUrl", "wss://api.yuntucv.com/ws").toString();
}

void Config::setWsBaseUrl(const QString &url)
{
    m_settings->setValue("api/wsBaseUrl", url);
    emit configChanged();
}

// 用户配置
QString Config::accessToken() const
{
    return m_settings->value("user/accessToken").toString();
}

void Config::setAccessToken(const QString &token)
{
    m_settings->setValue("user/accessToken", token);
}

bool Config::autoLogin() const
{
    return m_settings->value("user/autoLogin", false).toBool();
}

void Config::setAutoLogin(bool enabled)
{
    m_settings->setValue("user/autoLogin", enabled);
}

QString Config::lastLoginPhone() const
{
    return m_settings->value("user/lastLoginPhone").toString();
}

void Config::setLastLoginPhone(const QString &phone)
{
    m_settings->setValue("user/lastLoginPhone", phone);
}

// 下载配置
QString Config::downloadPath() const
{
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/yuntu";
    return m_settings->value("download/path", defaultPath).toString();
}

void Config::setDownloadPath(const QString &path)
{
    m_settings->setValue("download/path", path);
    emit configChanged();
}

bool Config::autoDownload() const
{
    return m_settings->value("download/auto", true).toBool();
}

void Config::setAutoDownload(bool enabled)
{
    m_settings->setValue("download/auto", enabled);
}

// 通知配置
bool Config::notificationEnabled() const
{
    return m_settings->value("notification/enabled", true).toBool();
}

void Config::setNotificationEnabled(bool enabled)
{
    m_settings->setValue("notification/enabled", enabled);
}

bool Config::notificationSound() const
{
    return m_settings->value("notification/sound", true).toBool();
}

void Config::setNotificationSound(bool enabled)
{
    m_settings->setValue("notification/sound", enabled);
}

// 通用配置
bool Config::startWithSystem() const
{
    return m_settings->value("general/startWithSystem", false).toBool();
}

void Config::setStartWithSystem(bool enabled)
{
    m_settings->setValue("general/startWithSystem", enabled);
    // TODO: 实现开机自启动
}

bool Config::minimizeToTray() const
{
    return m_settings->value("general/minimizeToTray", true).toBool();
}

void Config::setMinimizeToTray(bool enabled)
{
    m_settings->setValue("general/minimizeToTray", enabled);
}

QString Config::cachePath() const
{
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    return m_settings->value("general/cachePath", defaultPath).toString();
}

void Config::setCachePath(const QString &path)
{
    m_settings->setValue("general/cachePath", path);
    emit configChanged();
}

qint64 Config::cacheMaxSize() const
{
    return m_settings->value("general/cacheMaxSize", 5 * 1024 * 1024 * 1024LL).toLongLong(); // 默认5GB
}

void Config::setCacheMaxSize(qint64 size)
{
    m_settings->setValue("general/cacheMaxSize", size);
    emit configChanged();
}
