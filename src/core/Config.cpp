#include "Config.h"
#include <QStandardPaths>
#include <QFile>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

Config::Config(QObject *parent)
    : QObject(parent)
{
    // 使用 config.ini 文件
    QString configPath = QCoreApplication::applicationDirPath() + "/config.ini";

    // 如果 config.ini 不存在，尝试从 config.ini.example 复制
    if (!QFile::exists(configPath)) {
        QString examplePath = QCoreApplication::applicationDirPath() + "/config.ini.example";
        if (QFile::exists(examplePath)) {
            qDebug() << "config.ini not found, copying from config.ini.example";
            QFile::copy(examplePath, configPath);
        } else {
            qWarning() << "Neither config.ini nor config.ini.example found!";
        }
    }

    m_settings = new QSettings(configPath, QSettings::IniFormat, this);
    qDebug() << "Loading config from:" << configPath;
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
    // 从 INI 文件的 Server 节读取
    return m_settings->value("Server/url", "http://localhost:8000").toString();
}

void Config::setApiBaseUrl(const QString &url)
{
    m_settings->setValue("Server/url", url);
    emit configChanged();
}

QString Config::wsBaseUrl() const
{
    // 从服务器 URL 推导 WebSocket URL
    QString serverUrl = apiBaseUrl();
    QString wsUrl = serverUrl;
    wsUrl.replace("http://", "ws://");
    wsUrl.replace("https://", "wss://");
    wsUrl += "/ws";
    return wsUrl;
}

void Config::setWsBaseUrl(const QString &url)
{
    // WebSocket URL 由 Server/url 自动推导，不需要单独设置
    // 保留此方法以保持兼容性
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

// OSS配置
QString Config::ossAccessKey() const
{
    return m_settings->value("oss/accessKey").toString();
}

void Config::setOssAccessKey(const QString &key)
{
    m_settings->setValue("oss/accessKey", key);
    emit configChanged();
}

QString Config::ossSecretKey() const
{
    return m_settings->value("oss/secretKey").toString();
}

void Config::setOssSecretKey(const QString &key)
{
    m_settings->setValue("oss/secretKey", key);
    emit configChanged();
}

QString Config::ossBucket() const
{
    return m_settings->value("oss/bucket").toString();
}

void Config::setOssBucket(const QString &bucket)
{
    m_settings->setValue("oss/bucket", bucket);
    emit configChanged();
}

QString Config::ossEndpoint() const
{
    return m_settings->value("oss/endpoint").toString();
}

void Config::setOssEndpoint(const QString &endpoint)
{
    m_settings->setValue("oss/endpoint", endpoint);
    emit configChanged();
}
