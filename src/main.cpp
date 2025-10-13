#include <QApplication>
#include "core/Application.h"
#include "ui/ThemeManager.h"
#include "ui/views/LoginWindow.h"
#include "ui/views/MainWindow.h"
#include "managers/AuthManager.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

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
