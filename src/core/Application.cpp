#include "Application.h"
#include "Config.h"
#include "Logger.h"
#include "../network/HttpClient.h"
#include <QDir>
#include <QStandardPaths>

Application& Application::instance()
{
    static Application instance;
    return instance;
}

Application::Application()
{
    m_config = std::make_unique<Config>();
    m_logger = std::make_unique<Logger>();
}

Application::~Application()
{
    cleanup();
}

void Application::initialize()
{
    // 初始化日志系统
    m_logger->initialize();
    m_logger->info("Application", "应用程序启动");

    // 加载配置
    m_config->load();

    // 配置 HTTP 客户端
    HttpClient::instance().setBaseUrl(m_config->apiBaseUrl());
    m_logger->info("Application", QString("API Base URL: %1").arg(m_config->apiBaseUrl()));

    // 创建必要的目录
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataPath);
    QDir().mkpath(appDataPath + "/cache");
    QDir().mkpath(appDataPath + "/logs");
    QDir().mkpath(appDataPath + "/temp");

    m_logger->info("Application", "应用程序初始化完成");
}

void Application::cleanup()
{
    m_logger->info("Application", "应用程序关闭");
    m_config->save();
}
