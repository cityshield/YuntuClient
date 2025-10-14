#include "Application.h"
#include "Config.h"
#include "Logger.h"
#include "../network/HttpClient.h"
#include "../services/LogUploader.h"
#include <QDir>
#include <QStandardPaths>
#include <QTimer>

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
    m_logger->info("Application", QString::fromUtf8("应用程序启动"));

    // 记录系统信息
    m_logger->logSystemInfo();

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

    // 延迟 3 秒后上传日志（等待网络初始化完成）
    QTimer::singleShot(3000, this, &Application::uploadLogsToOSS);

    m_logger->info("Application", QString::fromUtf8("应用程序初始化完成"));
}

void Application::uploadLogsToOSS()
{
    m_logger->info("Application", QString::fromUtf8("开始上传日志文件到 OSS"));

    // 获取所有日志文件
    QStringList logFiles = m_logger->getAllLogFiles();

    if (logFiles.isEmpty()) {
        m_logger->info("Application", QString::fromUtf8("没有日志文件需要上传"));
        return;
    }

    // 创建日志上传器
    LogUploader* uploader = new LogUploader(this);

    // 连接信号
    connect(uploader, &LogUploader::allLogsUploaded, this, [this, uploader]() {
        m_logger->info("Application", QString::fromUtf8("所有日志上传完成"));
        uploader->deleteLater();
    });

    connect(uploader, &LogUploader::logUploadFailed, this, [this](const QString& filePath, const QString& error) {
        m_logger->warning("Application", QString::fromUtf8("日志上传失败: %1, 错误: %2").arg(filePath).arg(error));
    });

    // 开始上传
    uploader->uploadAllLogs(logFiles);
}

void Application::cleanup()
{
    m_logger->info("Application", "应用程序关闭");
    m_config->save();
}
