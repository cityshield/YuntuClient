/**
 * @file test_main.cpp
 * @brief 测试程序 - 验证核心功能
 *
 * 测试内容：
 * 1. Maya 环境检测
 * 2. HTTP 请求
 * 3. WebSocket 连接
 * 4. 配置管理
 * 5. 日志系统
 */

#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <iostream>

#include "core/Application.h"
#include "core/Config.h"
#include "core/Logger.h"
#include "services/MayaDetector.h"
#include "network/HttpClient.h"
#include "network/WebSocketClient.h"
#include "network/FileUploader.h"
#include "network/ApiService.h"

void printSeparator(const QString& title = QString())
{
    std::cout << "\n========================================\n";
    if (!title.isEmpty()) {
        std::cout << "  " << title.toStdString() << "\n";
        std::cout << "========================================\n";
    }
}

/**
 * @brief 测试 Maya 环境检测
 */
void testMayaDetector()
{
    printSeparator("测试 Maya 环境检测");

    MayaDetector detector;

    // 连接进度信号
    QObject::connect(&detector, &MayaDetector::detectProgress,
        [](int progress, const QString &message) {
            qDebug() << QString("[%1%] %2").arg(progress).arg(message);
        });

    // 检测所有 Maya 版本
    qDebug() << "\n开始检测系统中的 Maya 版本...";
    QVector<MayaSoftwareInfo> mayaVersions = detector.detectAllMayaVersions();

    qDebug() << "\n检测结果：";
    qDebug() << "找到" << mayaVersions.size() << "个 Maya 版本\n";

    if (mayaVersions.isEmpty()) {
        qDebug() << "未检测到 Maya 安装";
        qDebug() << "注意: 请确保 Maya 已正确安装在系统中";
    } else {
        for (int i = 0; i < mayaVersions.size(); ++i) {
            const MayaSoftwareInfo &info = mayaVersions[i];
            qDebug() << "\n========== Maya" << (i+1) << "==========";
            qDebug() << "软件名称:" << info.name;
            qDebug() << "版本号:" << info.version;
            qDebug() << "完整版本:" << info.fullVersion;
            qDebug() << "安装路径:" << info.installPath;
            qDebug() << "可执行文件:" << info.executablePath;
            qDebug() << "有效性:" << (info.isValid ? "是" : "否");

            if (!info.renderers.isEmpty()) {
                qDebug() << "\n支持的渲染器:";
                for (const QString &renderer : info.renderers) {
                    qDebug() << "  -" << renderer;
                }
            }

            if (!info.plugins.isEmpty()) {
                qDebug() << "\n已安装的插件 (" << info.plugins.size() << "个):";
                int displayCount = qMin(10, info.plugins.size());
                for (int j = 0; j < displayCount; ++j) {
                    qDebug() << "  -" << info.plugins[j];
                }
                if (info.plugins.size() > 10) {
                    qDebug() << "  ... 还有" << (info.plugins.size() - 10) << "个插件";
                }
            }
        }
    }

    // 测试场景文件分析（如果存在测试场景文件）
    qDebug() << "\n\n场景文件分析测试:";
    qDebug() << "如果你有 Maya 场景文件，可以手动测试：";
    qDebug() << "  QString scene = \"C:/path/to/scene.ma\";";
    qDebug() << "  QString version = detector.extractMayaVersionFromScene(scene);";
    qDebug() << "  QString renderer = detector.extractRendererFromScene(scene);";
    qDebug() << "  QStringList missing = detector.detectMissingAssets(scene);";
}

/**
 * @brief 测试配置管理
 */
void testConfig()
{
    printSeparator("测试配置管理");

    Config* config = Application::instance().config();

    qDebug() << "API 地址:" << config->apiBaseUrl();
    qDebug() << "WebSocket 地址:" << config->wsBaseUrl();
    qDebug() << "下载路径:" << config->downloadPath();
    qDebug() << "自动下载:" << (config->autoDownload() ? "启用" : "禁用");
    qDebug() << "通知:" << (config->notificationEnabled() ? "启用" : "禁用");
    qDebug() << "通知音效:" << (config->notificationSound() ? "启用" : "禁用");
    qDebug() << "开机启动:" << (config->startWithSystem() ? "启用" : "禁用");
    qDebug() << "最小化到托盘:" << (config->minimizeToTray() ? "启用" : "禁用");

    // 测试配置修改
    qDebug() << "\n测试配置修改...";
    config->setNotificationEnabled(true);
    config->setAutoDownload(true);
    qDebug() << "配置已更新并保存";
}

/**
 * @brief 测试日志系统
 */
void testLogger()
{
    printSeparator("测试日志系统");

    Logger logger;
    logger.initialize();

    qDebug() << "测试不同级别的日志输出:";
    logger.debug("TestModule", "这是一条 DEBUG 日志");
    logger.info("TestModule", "这是一条 INFO 日志");
    logger.warning("TestModule", "这是一条 WARNING 日志");
    logger.error("TestModule", "这是一条 ERROR 日志");

    qDebug() << "\n日志文件位置: AppData/Roaming/YunTu/logs/";
}

/**
 * @brief 测试 HTTP 请求（需要后端服务器）
 */
void testHttpClient()
{
    printSeparator("测试 HTTP 客户端");

    Config* config = Application::instance().config();
    HttpClient::instance().setBaseUrl(config->apiBaseUrl());

    qDebug() << "API 地址:" << config->apiBaseUrl();
    qDebug() << "\n注意: HTTP 测试需要后端服务器运行";
    qDebug() << "如果后端服务器未运行，请求会失败（这是正常的）\n";

    // 测试 GET 请求
    qDebug() << "发送 GET 请求到 /api/v1/test...";
    HttpClient::instance().get(
        "/api/v1/test",
        {},
        [](const QJsonObject& response) {
            qDebug() << "✓ HTTP GET 成功:";
            qDebug() << "  响应:" << response;
        },
        [](int statusCode, const QString& error) {
            qDebug() << "✗ HTTP GET 失败:";
            qDebug() << "  状态码:" << statusCode;
            qDebug() << "  错误:" << error;
            qDebug() << "  提示: 这是正常的，如果后端服务器未运行";
        }
    );
}

/**
 * @brief 测试 WebSocket（需要后端服务器）
 */
void testWebSocket()
{
    printSeparator("测试 WebSocket 客户端");

    Config* config = Application::instance().config();

    qDebug() << "WebSocket 地址:" << config->wsBaseUrl();
    qDebug() << "\n注意: WebSocket 测试需要后端服务器运行";
    qDebug() << "如果后端服务器未运行，连接会失败（这是正常的）\n";

    WebSocketClient* ws = new WebSocketClient();

    QObject::connect(ws, &WebSocketClient::connected, []() {
        qDebug() << "✓ WebSocket 连接成功";
    });

    QObject::connect(ws, &WebSocketClient::disconnected, []() {
        qDebug() << "✗ WebSocket 连接断开";
    });

    QObject::connect(ws, &WebSocketClient::error, [](const QString& error) {
        qDebug() << "✗ WebSocket 错误:" << error;
        qDebug() << "  提示: 这是正常的，如果后端服务器未运行";
    });

    qDebug() << "尝试连接 WebSocket...";
    ws->connectToServer(config->wsBaseUrl(), "test-user-123");

    // 等待2秒看连接结果
    QTimer::singleShot(2000, [ws]() {
        if (ws->isConnected()) {
            qDebug() << "\nWebSocket 连接状态: 已连接";
        } else {
            qDebug() << "\nWebSocket 连接状态: 未连接（后端服务器可能未运行）";
        }
    });
}

/**
 * @brief 显示功能菜单
 */
void showMenu()
{
    printSeparator("盛世云图客户端 - 功能测试");

    std::cout << "可用测试项:\n";
    std::cout << "  1. Maya 环境检测\n";
    std::cout << "  2. 配置管理\n";
    std::cout << "  3. 日志系统\n";
    std::cout << "  4. HTTP 客户端（需要后端）\n";
    std::cout << "  5. WebSocket 客户端（需要后端）\n";
    std::cout << "  0. 退出\n";
    std::cout << "\n选择测试项 (0-5): ";
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // 设置应用信息
    QCoreApplication::setOrganizationName("YunTu");
    QCoreApplication::setOrganizationDomain("yuntu.com");
    QCoreApplication::setApplicationName("盛世云图客户端");
    QCoreApplication::setApplicationVersion("1.0.0");

    // 初始化应用程序
    Application::instance().initialize();

    printSeparator("盛世云图客户端 - 测试程序");
    std::cout << "版本: 1.0.0-alpha\n";
    std::cout << "测试模式: 控制台\n";
    std::cout << "\n已实现的功能:\n";
    std::cout << "  ✓ 核心模块 (Application, Config, Logger)\n";
    std::cout << "  ✓ Maya 环境检测\n";
    std::cout << "  ✓ 网络层 (HTTP, WebSocket, FileUploader)\n";
    std::cout << "\n待实现的功能:\n";
    std::cout << "  ○ 数据模型\n";
    std::cout << "  ○ UI 界面\n";
    std::cout << "  ○ 任务管理\n";

    // 命令行参数模式
    if (argc > 1) {
        QString arg = argv[1];

        if (arg == "--maya" || arg == "-m") {
            testMayaDetector();
        } else if (arg == "--config" || arg == "-c") {
            testConfig();
        } else if (arg == "--log" || arg == "-l") {
            testLogger();
        } else if (arg == "--http" || arg == "-h") {
            testHttpClient();
        } else if (arg == "--ws" || arg == "-w") {
            testWebSocket();
        } else if (arg == "--all" || arg == "-a") {
            testConfig();
            testLogger();
            testMayaDetector();
            testHttpClient();
            testWebSocket();
        } else {
            std::cout << "\n用法: YuntuClient_Test [选项]\n";
            std::cout << "选项:\n";
            std::cout << "  -m, --maya     测试 Maya 检测\n";
            std::cout << "  -c, --config   测试配置管理\n";
            std::cout << "  -l, --log      测试日志系统\n";
            std::cout << "  -h, --http     测试 HTTP 客户端\n";
            std::cout << "  -w, --ws       测试 WebSocket\n";
            std::cout << "  -a, --all      运行所有测试\n";
            return 0;
        }

        // 等待异步操作完成
        QTimer::singleShot(5000, &app, &QCoreApplication::quit);
        return app.exec();
    }

    // 交互模式
    std::cout << "\n按 Enter 继续进入测试菜单...";
    std::cin.get();

    while (true) {
        showMenu();

        int choice;
        std::cin >> choice;

        if (choice == 0) {
            qDebug() << "\n退出测试程序";
            break;
        }

        switch (choice) {
            case 1:
                testMayaDetector();
                break;
            case 2:
                testConfig();
                break;
            case 3:
                testLogger();
                break;
            case 4:
                testHttpClient();
                // 等待异步请求
                QTimer::singleShot(3000, []() {});
                QCoreApplication::processEvents();
                break;
            case 5:
                testWebSocket();
                // 等待连接
                QTimer::singleShot(3000, []() {});
                QCoreApplication::processEvents();
                break;
            default:
                std::cout << "无效选择，请重新输入\n";
        }

        std::cout << "\n按 Enter 继续...";
        std::cin.ignore();
        std::cin.get();
    }

    Application::instance().cleanup();

    return 0;
}
