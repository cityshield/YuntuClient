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
#include "../components/FluentButton.h"
#include "../components/FluentLineEdit.h"

/**
 * @brief 注册对话框
 *
 * Fluent Design 风格的注册界面
 * 支持：
 * - 用户名注册
 * - 邮箱验证
 * - 密码强度检查
 * - 手机号（可选）
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
     * @brief 注册成功处理
     */
    void onRegisterSuccess();

    /**
     * @brief 注册失败处理
     */
    void onRegisterFailed(const QString &error);

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
     * @brief 验证邮箱格式
     */
    bool isValidEmail(const QString &email);

    /**
     * @brief 验证手机号格式
     */
    bool isValidPhone(const QString &phone);

private:
    // 标题
    QLabel *m_titleLabel;
    QLabel *m_subtitleLabel;

    // 输入框
    FluentLineEdit *m_usernameEdit;
    FluentLineEdit *m_emailEdit;
    FluentLineEdit *m_phoneEdit;
    FluentLineEdit *m_passwordEdit;
    FluentLineEdit *m_confirmPasswordEdit;

    // 按钮
    FluentButton *m_registerButton;
    FluentButton *m_cancelButton;

    // 错误提示
    QLabel *m_errorLabel;

    // 布局
    QVBoxLayout *m_mainLayout;
};

#endif // REGISTERDIALOG_H
