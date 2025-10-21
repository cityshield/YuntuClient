#pragma once

#include <QObject>
#include <QSettings>
#include <memory>

class Config;
class Logger;

/**
 * @brief 应用程序单例类
 */
class Application : public QObject
{
    Q_OBJECT

public:
    static Application& instance();

    void initialize();
    void cleanup();

    Config* config() const { return m_config.get(); }
    Logger* logger() const { return m_logger.get(); }
    QString version() const { return "1.0.0"; }

private:
    Application();
    ~Application();
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    /**
     * @brief 上传日志文件到 OSS
     */
    void uploadLogsToOSS();

    std::unique_ptr<Config> m_config;
    std::unique_ptr<Logger> m_logger;
};
