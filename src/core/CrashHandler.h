/**
 * @file CrashHandler.h
 * @brief 崩溃处理器
 */

#pragma once

#include <QObject>
#include <QString>

/**
 * @brief 崩溃处理器
 *
 * 捕获应用程序崩溃信号并记录到日志
 */
class CrashHandler : public QObject
{
    Q_OBJECT

public:
    static CrashHandler& instance();

    /**
     * @brief 安装崩溃处理器
     */
    void install();

    /**
     * @brief 卸载崩溃处理器
     */
    void uninstall();

    // 禁用拷贝构造和赋值
    CrashHandler(const CrashHandler&) = delete;
    CrashHandler& operator=(const CrashHandler&) = delete;

private:
    CrashHandler();
    ~CrashHandler();

    /**
     * @brief 崩溃信号处理函数
     */
    static void handleCrash(int signal);

    /**
     * @brief Qt 消息处理函数
     */
    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

    bool m_installed;
};
