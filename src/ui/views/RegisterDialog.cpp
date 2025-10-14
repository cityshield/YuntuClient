/**
 * @file RegisterDialog.cpp
 * @brief 注册对话框实现
 */

#include "RegisterDialog.h"
#include "../ThemeManager.h"
#include "../../managers/AuthManager.h"
#include "../../core/Logger.h"
#include "../../core/Application.h"
#include <QMessageBox>
#include <QTimer>
#include <QRegularExpression>

RegisterDialog::RegisterDialog(QWidget *parent)
    : QDialog(parent)
    , m_titleLabel(nullptr)
    , m_subtitleLabel(nullptr)
    , m_usernameEdit(nullptr)
    , m_phoneEdit(nullptr)
    , m_verificationCodeEdit(nullptr)
    , m_passwordEdit(nullptr)
    , m_confirmPasswordEdit(nullptr)
    , m_sendCodeButton(nullptr)
    , m_registerButton(nullptr)
    , m_cancelButton(nullptr)
    , m_errorLabel(nullptr)
    , m_mainLayout(nullptr)
    , m_countdownTimer(new QTimer(this))
    , m_countdown(0)
{
    initUI();
    connectSignals();

    // 设置对话框属性
    setWindowTitle(QString::fromUtf8("创建新账号"));
    setModal(true);
    setFixedSize(450, 650);
}

RegisterDialog::~RegisterDialog()
{
}

void RegisterDialog::onRegisterClicked()
{
    if (!validateInput()) {
        return;
    }

    QString username = m_usernameEdit->text().trimmed();
    QString phone = m_phoneEdit->text().trimmed();
    QString verificationCode = m_verificationCodeEdit->text().trimmed();
    QString password = m_passwordEdit->text();

    // 禁用注册按钮
    m_registerButton->setEnabled(false);
    m_registerButton->setText(QString::fromUtf8("注册中..."));

    // 清除错误提示
    m_errorLabel->clear();

    // 调用认证管理器注册（使用手机号和验证码）
    AuthManager::instance().registerUser(username, phone, verificationCode, password);

    Application::instance().logger()->info("RegisterDialog",
        QString::fromUtf8("尝试注册: %1 (手机: %2)").arg(username, phone));
}

void RegisterDialog::onCancelClicked()
{
    reject();
}

void RegisterDialog::onSendCodeClicked()
{
    QString phone = m_phoneEdit->text().trimmed();

    // 验证手机号
    if (phone.isEmpty()) {
        showError(QString::fromUtf8("请输入手机号"));
        m_phoneEdit->setFocus();
        return;
    }

    if (!isValidPhone(phone)) {
        showError(QString::fromUtf8("手机号格式不正确"));
        m_phoneEdit->setFocus();
        return;
    }

    // 调用API发送验证码
    AuthManager::instance().sendVerificationCode(phone);

    // 开始倒计时
    startCountdown();

    Application::instance().logger()->info("RegisterDialog",
        QString::fromUtf8("发送验证码到: %1").arg(phone));
}

void RegisterDialog::onCountdownTick()
{
    m_countdown--;

    if (m_countdown > 0) {
        m_sendCodeButton->setText(QString::fromUtf8("%1秒后重新发送").arg(m_countdown));
    } else {
        // 倒计时结束，恢复按钮
        m_countdownTimer->stop();
        m_sendCodeButton->setEnabled(true);
        m_sendCodeButton->setText(QString::fromUtf8("发送验证码"));
    }
}

void RegisterDialog::onRegisterSuccess()
{
    Application::instance().logger()->info("RegisterDialog", QString::fromUtf8("注册成功"));

    // 显示成功消息
    QMessageBox::information(this, QString::fromUtf8("注册成功"),
        QString::fromUtf8("注册成功！请使用您的手机号和密码登录。"));

    accept();
}

void RegisterDialog::onRegisterFailed(const QString &error)
{
    Application::instance().logger()->error("RegisterDialog",
        QString::fromUtf8("注册失败: %1").arg(error));

    // 恢复注册按钮
    m_registerButton->setEnabled(true);
    m_registerButton->setText(QString::fromUtf8("注册"));

    // 显示错误提示
    showError(error);
}

void RegisterDialog::initUI()
{
    // 创建主布局
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(40, 30, 40, 30);
    m_mainLayout->setSpacing(15);

    // 标题
    m_titleLabel = new QLabel(QString::fromUtf8("创建新账号"), this);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);

    // 副标题
    m_subtitleLabel = new QLabel(QString::fromUtf8("填写以下信息完成注册"), this);
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    m_subtitleLabel->setStyleSheet("color: #808080;");

    // 用户名输入框
    m_usernameEdit = new FluentLineEdit(QString::fromUtf8("用户名"), this);
    m_usernameEdit->setPlaceholderText(QString::fromUtf8("请输入用户名（3-20个字符）"));

    // 手机号输入框（必填）
    m_phoneEdit = new FluentLineEdit(QString::fromUtf8("手机号"), this);
    m_phoneEdit->setPlaceholderText(QString::fromUtf8("请输入手机号"));

    // 验证码输入框和发送按钮布局
    QHBoxLayout *codeLayout = new QHBoxLayout();
    codeLayout->setSpacing(10);

    m_verificationCodeEdit = new FluentLineEdit(QString::fromUtf8("验证码"), this);
    m_verificationCodeEdit->setPlaceholderText(QString::fromUtf8("请输入6位验证码"));

    m_sendCodeButton = new FluentButton(QString::fromUtf8("发送验证码"), this);
    m_sendCodeButton->setMinimumHeight(40);
    m_sendCodeButton->setMinimumWidth(120);

    codeLayout->addWidget(m_verificationCodeEdit, 1);
    codeLayout->addWidget(m_sendCodeButton);

    // 密码输入框
    m_passwordEdit = new FluentLineEdit(QString::fromUtf8("密码"), this);
    m_passwordEdit->setPasswordMode(true);
    m_passwordEdit->setPlaceholderText(QString::fromUtf8("请输入密码（至少6位）"));

    // 确认密码输入框
    m_confirmPasswordEdit = new FluentLineEdit(QString::fromUtf8("确认密码"), this);
    m_confirmPasswordEdit->setPasswordMode(true);
    m_confirmPasswordEdit->setPlaceholderText(QString::fromUtf8("请再次输入密码"));

    // 错误提示标签
    m_errorLabel = new QLabel(this);
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setStyleSheet("color: #D13438; font-size: 12px;");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();

    // 按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    // 取消按钮
    m_cancelButton = new FluentButton(QString::fromUtf8("取消"), this);
    m_cancelButton->setMinimumHeight(36);

    // 注册按钮
    m_registerButton = new FluentButton(QString::fromUtf8("注册"), this);
    m_registerButton->setIsPrimary(true);
    m_registerButton->setMinimumHeight(36);

    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_registerButton);

    // 添加到主布局
    m_mainLayout->addWidget(m_titleLabel);
    m_mainLayout->addWidget(m_subtitleLabel);
    m_mainLayout->addSpacing(15);
    m_mainLayout->addWidget(m_usernameEdit);
    m_mainLayout->addWidget(m_phoneEdit);
    m_mainLayout->addLayout(codeLayout);
    m_mainLayout->addWidget(m_passwordEdit);
    m_mainLayout->addWidget(m_confirmPasswordEdit);
    m_mainLayout->addWidget(m_errorLabel);
    m_mainLayout->addSpacing(10);
    m_mainLayout->addLayout(buttonLayout);

    // 设置对话框背景色
    ThemeManager &theme = ThemeManager::instance();
    QString dialogStyle = QString("QDialog { background-color: %1; }")
        .arg(theme.getSurfaceColor().name());
    setStyleSheet(dialogStyle);
}

void RegisterDialog::connectSignals()
{
    // 发送验证码按钮
    connect(m_sendCodeButton, &FluentButton::clicked,
            this, &RegisterDialog::onSendCodeClicked);

    // 注册按钮
    connect(m_registerButton, &FluentButton::clicked,
            this, &RegisterDialog::onRegisterClicked);

    // 取消按钮
    connect(m_cancelButton, &FluentButton::clicked,
            this, &RegisterDialog::onCancelClicked);

    // 倒计时定时器
    connect(m_countdownTimer, &QTimer::timeout,
            this, &RegisterDialog::onCountdownTick);

    // 认证管理器信号
    connect(&AuthManager::instance(), &AuthManager::registerSuccess,
            this, &RegisterDialog::onRegisterSuccess);

    connect(&AuthManager::instance(), &AuthManager::registerFailed,
            this, &RegisterDialog::onRegisterFailed);

    // 回车键注册
    connect(m_confirmPasswordEdit, &QLineEdit::returnPressed,
            this, &RegisterDialog::onRegisterClicked);
}

bool RegisterDialog::validateInput()
{
    QString username = m_usernameEdit->text().trimmed();
    QString phone = m_phoneEdit->text().trimmed();
    QString verificationCode = m_verificationCodeEdit->text().trimmed();
    QString password = m_passwordEdit->text();
    QString confirmPassword = m_confirmPasswordEdit->text();

    // 验证用户名
    if (username.isEmpty()) {
        showError(QString::fromUtf8("请输入用户名"));
        m_usernameEdit->setFocus();
        return false;
    }

    if (username.length() < 3 || username.length() > 20) {
        showError(QString::fromUtf8("用户名长度应为3-20个字符"));
        m_usernameEdit->setFocus();
        return false;
    }

    // 验证手机号（必填）
    if (phone.isEmpty()) {
        showError(QString::fromUtf8("请输入手机号"));
        m_phoneEdit->setFocus();
        return false;
    }

    if (!isValidPhone(phone)) {
        showError(QString::fromUtf8("手机号格式不正确"));
        m_phoneEdit->setFocus();
        return false;
    }

    // 验证验证码
    if (verificationCode.isEmpty()) {
        showError(QString::fromUtf8("请输入验证码"));
        m_verificationCodeEdit->setFocus();
        return false;
    }

    if (verificationCode.length() != 6) {
        showError(QString::fromUtf8("验证码应为6位数字"));
        m_verificationCodeEdit->setFocus();
        return false;
    }

    // 验证密码
    if (password.isEmpty()) {
        showError(QString::fromUtf8("请输入密码"));
        m_passwordEdit->setFocus();
        return false;
    }

    if (password.length() < 6) {
        showError(QString::fromUtf8("密码长度至少6位"));
        m_passwordEdit->setFocus();
        return false;
    }

    // 验证确认密码
    if (confirmPassword.isEmpty()) {
        showError(QString::fromUtf8("请确认密码"));
        m_confirmPasswordEdit->setFocus();
        return false;
    }

    if (password != confirmPassword) {
        showError(QString::fromUtf8("两次输入的密码不一致"));
        m_confirmPasswordEdit->setFocus();
        return false;
    }

    return true;
}

void RegisterDialog::showError(const QString &message)
{
    m_errorLabel->setText(message);
    m_errorLabel->show();

    // 5秒后自动隐藏
    QTimer::singleShot(5000, m_errorLabel, &QLabel::hide);
}

bool RegisterDialog::isValidPhone(const QString &phone)
{
    // 简单的中国手机号验证（11位数字，1开头）
    QRegularExpression regex("^1[3-9]\\d{9}$");
    return regex.match(phone).hasMatch();
}

void RegisterDialog::startCountdown()
{
    // 设置倒计时60秒
    m_countdown = 60;
    m_sendCodeButton->setEnabled(false);
    m_sendCodeButton->setText(QString::fromUtf8("%1秒后重新发送").arg(m_countdown));

    // 启动定时器，每秒触发一次
    m_countdownTimer->start(1000);
}
