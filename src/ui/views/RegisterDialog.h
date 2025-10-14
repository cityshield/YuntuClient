/**
 * @file RegisterDialog.h
 * @brief 注册对话框
 */

#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include "../components/FluentButton.h"
#include "../components/FluentLineEdit.h"

/**
 * @brief 注册对话框
 *
 * Fluent Design 风格的注册界面
 * 支持：
 * - 用户名注册
 * - 手机号验证（必填）
 * - 短信验证码验证
 * - 密码强度检查
 */
class RegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = nullptr);
    ~RegisterDialog();

private slots:
    /**
     * @brief 注册按钮点击
     */
    void onRegisterClicked();

    /**
     * @brief 取消按钮点击
     */
    void onCancelClicked();

    /**
     * @brief 发送验证码按钮点击
     */
    void onSendCodeClicked();

    /**
     * @brief 注册成功处理
     */
    void onRegisterSuccess();

    /**
     * @brief 注册失败处理
     */
    void onRegisterFailed(const QString &error);

    /**
     * @brief 倒计时更新
     */
    void onCountdownTick();

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

    /**
     * @brief 验证手机号格式
     */
    bool isValidPhone(const QString &phone);

    /**
     * @brief 开始倒计时
     */
    void startCountdown();

private:
    // 标题
    QLabel *m_titleLabel;
    QLabel *m_subtitleLabel;

    // 输入框
    FluentLineEdit *m_usernameEdit;
    FluentLineEdit *m_phoneEdit;
    FluentLineEdit *m_verificationCodeEdit;
    FluentLineEdit *m_passwordEdit;
    FluentLineEdit *m_confirmPasswordEdit;

    // 按钮
    FluentButton *m_sendCodeButton;
    FluentButton *m_registerButton;
    FluentButton *m_cancelButton;

    // 错误提示
    QLabel *m_errorLabel;

    // 布局
    QVBoxLayout *m_mainLayout;

    // 倒计时
    QTimer *m_countdownTimer;
    int m_countdown;
};

#endif // REGISTERDIALOG_H
