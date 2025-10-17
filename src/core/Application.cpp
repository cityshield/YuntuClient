#include "Application.h"
#include "Config.h"
#include "Logger.h"
#include "../network/HttpClient.h"
#include "../services/LogUploader.h"
#include <QDir>
#include <QStandardPaths>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>

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

    // 从 .env 文件加载 OSS 配置（如果还没有配置）
    if (m_config->ossAccessKey().isEmpty()) {
        loadOssConfigFromEnv();
    }

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

void Application::loadOssConfigFromEnv()
{
    // 尝试从 .env 文件加载配置
    QString envFilePath = QCoreApplication::applicationDirPath() + "/.env";
    QFile envFile(envFilePath);

    if (!envFile.exists()) {
        m_logger->warning("Application", QString::fromUtf8(".env 文件不存在: %1").arg(envFilePath));
        return;
    }

    if (!envFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_logger->warning("Application", QString::fromUtf8("无法打开 .env 文件: %1").arg(envFilePath));
        return;
    }

    QTextStream in(&envFile);
    QMap<QString, QString> envVars;

    // 读取所有环境变量
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        // 跳过注释和空行
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        // 解析 KEY=VALUE
        int equalPos = line.indexOf('=');
        if (equalPos > 0) {
            QString key = line.left(equalPos).trimmed();
            QString value = line.mid(equalPos + 1).trimmed();
            envVars[key] = value;
        }
    }

    envFile.close();

    // 设置 OSS 配置
    if (envVars.contains("OSS_ACCESS_KEY_ID")) {
        m_config->setOssAccessKey(envVars["OSS_ACCESS_KEY_ID"]);
    }
    if (envVars.contains("OSS_ACCESS_KEY_SECRET")) {
        m_config->setOssSecretKey(envVars["OSS_ACCESS_KEY_SECRET"]);
    }
    if (envVars.contains("OSS_BUCKET_NAME")) {
        m_config->setOssBucket(envVars["OSS_BUCKET_NAME"]);
    }
    if (envVars.contains("OSS_ENDPOINT")) {
        m_config->setOssEndpoint(envVars["OSS_ENDPOINT"]);
    }

    m_logger->info("Application", QString::fromUtf8("已从 .env 文件加载 OSS 配置"));
}

void Application::cleanup()
{
    m_logger->info("Application", "应用程序关闭");
    m_config->save();
}
