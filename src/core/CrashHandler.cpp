/**
 * @file CrashHandler.cpp
 * @brief 崩溃处理器实现
 */

#include "CrashHandler.h"
#include "Application.h"
#include "Logger.h"
#include <csignal>
#include <QDebug>
#include <QDateTime>
#include <QCoreApplication>

CrashHandler::CrashHandler()
    : QObject(nullptr)
    , m_installed(false)
{
}

CrashHandler::~CrashHandler()
{
    uninstall();
}

CrashHandler& CrashHandler::instance()
{
    static CrashHandler instance;
    return instance;
}

void CrashHandler::install()
{
    if (m_installed) {
        return;
    }

    // 安装信号处理器（用于捕获崩溃信号）
    signal(SIGSEGV, handleCrash);  // 段错误
    signal(SIGABRT, handleCrash);  // 异常终止
    signal(SIGFPE, handleCrash);   // 浮点异常
    signal(SIGILL, handleCrash);   // 非法指令

#ifndef Q_OS_WIN
    signal(SIGBUS, handleCrash);   // 总线错误（非 Windows）
#endif

    // 安装 Qt 消息处理器（用于捕获 Qt 错误/警告）
    qInstallMessageHandler(messageHandler);

    m_installed = true;

    Application::instance().logger()->info("CrashHandler",
        QString::fromUtf8("崩溃处理器已安装"));
}

void CrashHandler::uninstall()
{
    if (!m_installed) {
        return;
    }

    // 恢复默认信号处理器
    signal(SIGSEGV, SIG_DFL);
    signal(SIGABRT, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    signal(SIGILL, SIG_DFL);

#ifndef Q_OS_WIN
    signal(SIGBUS, SIG_DFL);
#endif

    // 恢复默认消息处理器
    qInstallMessageHandler(nullptr);

    m_installed = false;
}

void CrashHandler::handleCrash(int signal)
{
    // 获取信号名称
    QString signalName;
    switch (signal) {
        case SIGSEGV:
            signalName = "SIGSEGV (Segmentation Fault)";
            break;
        case SIGABRT:
            signalName = "SIGABRT (Abort)";
            break;
        case SIGFPE:
            signalName = "SIGFPE (Floating Point Exception)";
            break;
        case SIGILL:
            signalName = "SIGILL (Illegal Instruction)";
            break;
#ifndef Q_OS_WIN
        case SIGBUS:
            signalName = "SIGBUS (Bus Error)";
            break;
#endif
        default:
            signalName = QString("Unknown Signal (%1)").arg(signal);
            break;
    }

    QString crashMessage = QString::fromUtf8("应用程序崩溃: 信号 %1").arg(signalName);

    // 记录崩溃日志
    Logger* logger = Application::instance().logger();
    if (logger) {
        logger->logCrash(crashMessage);
    }

    // 输出到控制台
    qCritical() << "CRASH DETECTED:" << crashMessage;

    // 恢复默认处理器并重新抛出信号（让系统处理崩溃）
    std::signal(signal, SIG_DFL);
    std::raise(signal);
}

void CrashHandler::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Logger* logger = Application::instance().logger();
    if (!logger) {
        // 如果 Logger 未初始化，输出到控制台
        QByteArray localMsg = msg.toLocal8Bit();
        fprintf(stderr, "%s\n", localMsg.constData());
        return;
    }

    QString category = "Qt";
    if (context.category) {
        category = QString(context.category);
    }

    // 根据消息类型记录日志
    switch (type) {
        case QtDebugMsg:
            logger->debug(category, msg);
            break;
        case QtInfoMsg:
            logger->info(category, msg);
            break;
        case QtWarningMsg:
            logger->warning(category, msg);
            break;
        case QtCriticalMsg:
            logger->error(category, QString::fromUtf8("Critical: %1").arg(msg));
            break;
        case QtFatalMsg:
            // Fatal 消息 - 记录崩溃
            logger->logCrash(QString::fromUtf8("Fatal Error: %1").arg(msg));
            // 继续原有的 fatal 处理（会终止程序）
            abort();
            break;
    }
}
