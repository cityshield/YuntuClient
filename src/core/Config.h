#pragma once

#include <QObject>
#include <QSettings>
#include <QString>

/**
 * @brief 配置管理类
 */
class Config : public QObject
{
    Q_OBJECT

public:
    explicit Config(QObject *parent = nullptr);
    ~Config();

    void load();
    void save();

    // API配置
    QString apiBaseUrl() const;
    void setApiBaseUrl(const QString &url);

    QString wsBaseUrl() const;
    void setWsBaseUrl(const QString &url);

    // 用户配置
    QString accessToken() const;
    void setAccessToken(const QString &token);

    bool autoLogin() const;
    void setAutoLogin(bool enabled);

    QString lastLoginPhone() const;
    void setLastLoginPhone(const QString &phone);

    // 下载配置
    QString downloadPath() const;
    void setDownloadPath(const QString &path);

    bool autoDownload() const;
    void setAutoDownload(bool enabled);

    // 通知配置
    bool notificationEnabled() const;
    void setNotificationEnabled(bool enabled);

    bool notificationSound() const;
    void setNotificationSound(bool enabled);

    // 通用配置
    bool startWithSystem() const;
    void setStartWithSystem(bool enabled);

    bool minimizeToTray() const;
    void setMinimizeToTray(bool enabled);

    QString cachePath() const;
    void setCachePath(const QString &path);

    qint64 cacheMaxSize() const; // 字节
    void setCacheMaxSize(qint64 size);

signals:
    void configChanged();

private:
    QSettings *m_settings;
};
