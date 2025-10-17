#include <QApplication>
#include "core/Application.h"
#include "ui/ThemeManager.h"
#include "ui/views/LoginWindow.h"
#include "ui/views/MainWindow.h"
#include "managers/AuthManager.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// 自定义消息处理函数，将 qDebug 输出重定向到日志文件
static QFile *logFile = nullptr;
static QTextStream *logStream = nullptr;

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString level;
    switch (type) {
        case QtDebugMsg:    level = "DEBUG"; break;
        case QtInfoMsg:     level = "INFO "; break;
        case QtWarningMsg:  level = "WARN "; break;
        case QtCriticalMsg: level = "ERROR"; break;
        case QtFatalMsg:    level = "FATAL"; break;
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString logMessage = QString("[%1] [%2] %3").arg(timestamp).arg(level).arg(msg);

    // 输出到文件
    if (logStream) {
        *logStream << logMessage << "\n";
        logStream->flush();
    }

    // 同时输出到控制台（用于开发调试）
    fprintf(stderr, "%s\n", logMessage.toLocal8Bit().constData());

    if (type == QtFatalMsg) {
        abort();
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

#ifdef Q_OS_WIN
    // Windows 下设置 UTF-8 编码
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    // 设置应用信息
    QApplication::setOrganizationName("YunTu");
    QApplication::setOrganizationDomain("yuntu.com");
    QApplication::setApplicationName(QString::fromUtf8("盛世云图客户端"));
    QApplication::setApplicationVersion("1.0.0");

    // 设置 qDebug 日志重定向
    QString logPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    QDir().mkpath(logPath);
    QString logFileName = QDateTime::currentDateTime().toString("yyyy-MM-dd") + ".log";
    QString logFilePath = logPath + "/" + logFileName;

    logFile = new QFile(logFilePath);
    if (logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        logStream = new QTextStream(logFile);
        // 设置 UTF-8 编码，确保中文正常显示
        logStream->setEncoding(QStringConverter::Utf8);
        qInstallMessageHandler(customMessageHandler);
    }

    // 初始化应用程序
    Application::instance().initialize();

    // 初始化主题管理器
    ThemeManager::instance().initialize();

    // 创建登录窗口
    LoginWindow *loginWindow = new LoginWindow();

    // 连接登录成功信号
    QObject::connect(&AuthManager::instance(), &AuthManager::loginSuccess,
        [loginWindow]() {
            // 登录成功，关闭登录窗口，显示主窗口
            loginWindow->close();
            loginWindow->deleteLater();

            MainWindow *mainWindow = new MainWindow();
            mainWindow->show();
        });

    loginWindow->show();

    return app.exec();
}
