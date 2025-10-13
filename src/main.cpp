#include <QApplication>
#include "core/Application.h"
#include "ui/LoginDialog.h"
#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 设置应用信息
    QApplication::setOrganizationName("YunTu");
    QApplication::setOrganizationDomain("yuntu.com");
    QApplication::setApplicationName("盛世云图客户端");
    QApplication::setApplicationVersion("1.0.0");

    // 初始化应用程序
    Application::instance().initialize();

    // 显示登录对话框
    LoginDialog loginDialog;
    if (loginDialog.exec() == QDialog::Accepted) {
        // 登录成功，显示主窗口
        MainWindow *mainWindow = new MainWindow();
        mainWindow->show();

        return app.exec();
    }

    return 0;
}
