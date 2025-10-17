/**
 * @file LoginWindow.h
 * @brief 登录窗口
 */

#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "../components/FluentButton.h"
#include "../components/FluentLineEdit.h"
#include "../components/TitleBar.h"

/**
 * @brief 登录窗口
 *
 * Fluent Design 风格的登录界面
 * 支持：
 * - 用户名/邮箱登录
 * - 记住密码
 * - 注册账号
 * - 忘记密码
 */
class LoginWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow();

protected:
    /**
     * @brief 绘制事件（背景）
     */
    void paintEvent(QPaintEvent *event) override;

private slots:
    /**
     * @brief 登录按钮点击
     */
    void onLoginClicked();

    /**
     * @brief 注册按钮点击
     */
    void onRegisterClicked();

    /**
     * @brief 忘记密码点击
     */
    void onForgotPasswordClicked();

    /**
     * @brief 登录成功处理
     */
    void onLoginSuccess();

    /**
     * @brief 登录失败处理
     */
    void onLoginFailed(const QString &error);

    /**
     * @brief 演示模式按钮点击
     */
    void onDemoModeClicked();

    /**
     * @brief Maya 检测按钮点击
     */
    void onMayaDetectionClicked();

private:
    /**
     * @brief 初始化 UI
     */
    void initUI();

    /**
     * @brief 连接信号
     */
    void connectSignals();

    /**
     * @brief 验证输入
     */
    bool validateInput();

    /**
     * @brief 显示错误提示
     */
    void showError(const QString &message);

private:
    // 标题栏
    TitleBar *m_titleBar;

    // Logo 和标题
    QLabel *m_logoLabel;
    QLabel *m_titleLabel;
    QLabel *m_subtitleLabel;

    // 输入框
    FluentLineEdit *m_usernameEdit;
    FluentLineEdit *m_passwordEdit;

    // 选项
    QCheckBox *m_rememberCheck;
    QLabel *m_forgotPasswordLabel;

    // 按钮
    FluentButton *m_loginButton;
    FluentButton *m_registerButton;
    FluentButton *m_demoButton;
    FluentButton *m_mayaDetectionButton;

    // 错误提示
    QLabel *m_errorLabel;

    // 布局
    QVBoxLayout *m_mainLayout;
    QWidget *m_loginPanel;
};

#endif // LOGINWINDOW_H
