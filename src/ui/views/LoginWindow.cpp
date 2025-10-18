/**
 * @file LoginWindow.cpp
 * @brief 登录窗口实现
 */

#include "LoginWindow.h"
#include "RegisterDialog.h"
#include "MayaDetectionDialog.h"
#include "../ThemeManager.h"
#include "../components/ToastManager.h"
#include "../../managers/AuthManager.h"
#include "../../core/Logger.h"
#include "../../core/Application.h"
#include "../../network/HttpClient.h"
#include <QPainter>
#include <QPainterPath>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>
#include <QTimer>

LoginWindow::LoginWindow(QWidget *parent)
    : QWidget(parent)
    , m_titleBar(nullptr)
    , m_logoLabel(nullptr)
    , m_titleLabel(nullptr)
    , m_subtitleLabel(nullptr)
    , m_usernameEdit(nullptr)
    , m_passwordEdit(nullptr)
    , m_rememberCheck(nullptr)
    , m_forgotPasswordLabel(nullptr)
    , m_loginButton(nullptr)
    , m_registerButton(nullptr)
    , m_demoButton(nullptr)
    , m_mayaDetectionButton(nullptr)
    , m_errorLabel(nullptr)
    , m_mainLayout(nullptr)
    , m_loginPanel(nullptr)
{
    initUI();
    connectSignals();

    // 设置窗口属性
    setWindowTitle(QString::fromUtf8("盛世云图 - 登录"));
    resize(1000, 650);

    // 设置窗口居中
    setWindowFlag(Qt::FramelessWindowHint);  // 无边框窗口
    setAttribute(Qt::WA_TranslucentBackground);  // 透明背景

    // 初始化 Toast Manager
    ToastManager::instance().setMainWindow(this);

    // 连接 HttpClient 的 apiResponse 信号到 ToastManager
    connect(&HttpClient::instance(), &HttpClient::apiResponse,
            [](const QString& endpoint, int statusCode, bool success) {
                ToastManager::instance().showApiResponse(endpoint, statusCode, success);
            });
}

LoginWindow::~LoginWindow()
{
}

void LoginWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 获取主题颜色
    ThemeManager &theme = ThemeManager::instance();
    QColor bgColor = theme.getBackgroundColor();

    // 绘制圆角背景
    QPainterPath path;
    path.addRoundedRect(rect(), 12, 12);
    painter.fillPath(path, bgColor);
}

void LoginWindow::onLoginClicked()
{
    if (!validateInput()) {
        return;
    }

    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text();
    bool remember = m_rememberCheck->isChecked();

    // 禁用登录按钮
    m_loginButton->setEnabled(false);
    m_loginButton->setText(QString::fromUtf8("登录中..."));

    // 清除错误提示
    m_errorLabel->clear();

    // 调用认证管理器登录
    AuthManager::instance().login(username, password, remember);

    Application::instance().logger()->info("LoginWindow",
        QString::fromUtf8("尝试登录: %1").arg(username));
}

void LoginWindow::onRegisterClicked()
{
    RegisterDialog *registerDialog = new RegisterDialog(this);

    if (registerDialog->exec() == QDialog::Accepted) {
        // 注册成功后，可以自动填充登录信息或提示用户登录
        Application::instance().logger()->info("LoginWindow",
            QString::fromUtf8("用户完成注册，可以登录"));
    }

    registerDialog->deleteLater();
}

void LoginWindow::onForgotPasswordClicked()
{
    // TODO: 打开忘记密码窗口
    QMessageBox::information(this, QString::fromUtf8("忘记密码"),
        QString::fromUtf8("密码找回功能即将推出"));
}

void LoginWindow::onDemoModeClicked()
{
    Application::instance().logger()->info("LoginWindow", QString::fromUtf8("进入演示模式"));

    // 创建一个演示用户
    User* demoUser = new User(this);
    demoUser->setUserId("demo_user_001");
    demoUser->setUsername(QString::fromUtf8("演示用户"));
    demoUser->setEmail("demo@yuntu.com");
    demoUser->setBalance(1000.0);
    demoUser->setMemberLevel(MemberLevel::Pro);
    demoUser->setIsLoggedIn(true);

    // 触发登录成功信号
    emit AuthManager::instance().loginSuccess(demoUser);
}

void LoginWindow::onMayaDetectionClicked()
{
    Application::instance().logger()->info("LoginWindow", QString::fromUtf8("打开 Maya 检测对话框"));

    // 创建并显示 Maya 检测对话框
    MayaDetectionDialog *dialog = new MayaDetectionDialog(this);
    dialog->exec();
    dialog->deleteLater();
}

void LoginWindow::onLoginSuccess()
{
    Application::instance().logger()->info("LoginWindow", QString::fromUtf8("登录成功"));

    // 关闭登录窗口，打开主窗口
    // TODO: 打开主窗口
    close();
}

void LoginWindow::onLoginFailed(const QString &error)
{
    Application::instance().logger()->error("LoginWindow",
        QString::fromUtf8("登录失败: %1").arg(error));

    // 恢复登录按钮
    m_loginButton->setEnabled(true);
    m_loginButton->setText(QString::fromUtf8("登录"));

    // 显示错误提示
    showError(error);
}

void LoginWindow::initUI()
{
    // 创建主布局
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // 创建标题栏
    m_titleBar = new TitleBar(this);
    m_titleBar->setTitle(QString::fromUtf8("盛世云图 - 登录"));
    m_mainLayout->addWidget(m_titleBar);

    // 创建登录面板
    m_loginPanel = new QWidget(this);
    m_loginPanel->setFixedSize(400, 550);

    // 登录面板布局
    QVBoxLayout *panelLayout = new QVBoxLayout(m_loginPanel);
    panelLayout->setContentsMargins(40, 40, 40, 40);
    panelLayout->setSpacing(20);

    // Logo（如果有）
    m_logoLabel = new QLabel(m_loginPanel);
    m_logoLabel->setAlignment(Qt::AlignCenter);
    m_logoLabel->setFixedHeight(60);
    // TODO: 设置 Logo 图片
    // m_logoLabel->setPixmap(QPixmap(":/images/logo.png"));

    // 标题
    m_titleLabel = new QLabel(QString::fromUtf8("盛世云图"), m_loginPanel);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);

    // 副标题
    m_subtitleLabel = new QLabel(QString::fromUtf8("Maya 云渲染平台"), m_loginPanel);
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    QFont subtitleFont = m_subtitleLabel->font();
    subtitleFont.setPointSize(12);
    m_subtitleLabel->setFont(subtitleFont);
    m_subtitleLabel->setStyleSheet("color: #808080;");

    // 用户名输入框
    m_usernameEdit = new FluentLineEdit(QString::fromUtf8("用户名或邮箱"), m_loginPanel);

    // 密码输入框
    m_passwordEdit = new FluentLineEdit(QString::fromUtf8("密码"), m_loginPanel);
    m_passwordEdit->setPasswordMode(true);

    // 记住密码和忘记密码
    QHBoxLayout *optionsLayout = new QHBoxLayout();
    m_rememberCheck = new QCheckBox(QString::fromUtf8("记住密码"), m_loginPanel);
    m_rememberCheck->setChecked(true);

    m_forgotPasswordLabel = new QLabel(
        QString::fromUtf8("<a href='#' style='color:#0078D4;text-decoration:none;'>忘记密码？</a>"),
        m_loginPanel);
    m_forgotPasswordLabel->setAlignment(Qt::AlignRight);
    m_forgotPasswordLabel->setCursor(Qt::PointingHandCursor);

    optionsLayout->addWidget(m_rememberCheck);
    optionsLayout->addStretch();
    optionsLayout->addWidget(m_forgotPasswordLabel);

    // 错误提示标签
    m_errorLabel = new QLabel(m_loginPanel);
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setStyleSheet("color: #D13438; font-size: 12px;");
    m_errorLabel->setWordWrap(true);
    m_errorLabel->hide();

    // 登录按钮
    m_loginButton = new FluentButton(QString::fromUtf8("登录"), m_loginPanel);
    m_loginButton->setIsPrimary(true);
    m_loginButton->setMinimumHeight(40);

    // 注册按钮
    m_registerButton = new FluentButton(QString::fromUtf8("创建新账号"), m_loginPanel);

    // 演示模式按钮
    m_demoButton = new FluentButton(QString::fromUtf8("演示模式（无需登录）"), m_loginPanel);
    m_demoButton->setStyleSheet("QPushButton { color: #0078D4; background-color: transparent; border: 1px solid #0078D4; }");

    // Maya 检测按钮
    m_mayaDetectionButton = new FluentButton(QString::fromUtf8("🔍 检测 Maya 环境"), m_loginPanel);
    m_mayaDetectionButton->setStyleSheet("QPushButton { color: #16C60C; background-color: transparent; border: 1px solid #16C60C; }");

    // 添加到面板布局
    panelLayout->addWidget(m_logoLabel);
    panelLayout->addWidget(m_titleLabel);
    panelLayout->addWidget(m_subtitleLabel);
    panelLayout->addSpacing(20);
    panelLayout->addWidget(m_usernameEdit);
    panelLayout->addWidget(m_passwordEdit);
    panelLayout->addLayout(optionsLayout);
    panelLayout->addWidget(m_errorLabel);
    panelLayout->addSpacing(10);
    panelLayout->addWidget(m_loginButton);
    panelLayout->addWidget(m_registerButton);
    panelLayout->addSpacing(10);
    panelLayout->addWidget(m_demoButton);
    panelLayout->addWidget(m_mayaDetectionButton);
    panelLayout->addStretch();

    // 将登录面板居中
    m_mainLayout->addStretch();
    m_mainLayout->addWidget(m_loginPanel, 0, Qt::AlignCenter);
    m_mainLayout->addStretch();

    // 为登录面板添加阴影效果
    ThemeManager::instance().applyShadowEffect(m_loginPanel, 30, 0, 10);

    // 设置面板背景色
    ThemeManager &theme = ThemeManager::instance();
    QString panelStyle = QString("background-color: %1; border-radius: 12px;")
        .arg(theme.getSurfaceColor().name());
    m_loginPanel->setStyleSheet(panelStyle);
}

void LoginWindow::connectSignals()
{
    // 登录按钮
    connect(m_loginButton, &FluentButton::clicked,
            this, &LoginWindow::onLoginClicked);

    // 注册按钮
    connect(m_registerButton, &FluentButton::clicked,
            this, &LoginWindow::onRegisterClicked);

    // 演示模式按钮
    connect(m_demoButton, &FluentButton::clicked,
            this, &LoginWindow::onDemoModeClicked);

    // Maya 检测按钮
    connect(m_mayaDetectionButton, &FluentButton::clicked,
            this, &LoginWindow::onMayaDetectionClicked);

    // 忘记密码
    connect(m_forgotPasswordLabel, &QLabel::linkActivated,
            this, &LoginWindow::onForgotPasswordClicked);

    // 认证管理器信号
    connect(&AuthManager::instance(), &AuthManager::loginSuccess,
            this, &LoginWindow::onLoginSuccess);

    connect(&AuthManager::instance(), &AuthManager::loginFailed,
            this, &LoginWindow::onLoginFailed);

    // 回车键登录
    connect(m_passwordEdit, &QLineEdit::returnPressed,
            this, &LoginWindow::onLoginClicked);

    // 主题变更时更新面板背景
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [this](ThemeType) {
                ThemeManager &themeMgr = ThemeManager::instance();
                QString panelStyle = QString("background-color: %1; border-radius: 12px;")
                    .arg(themeMgr.getSurfaceColor().name());
                m_loginPanel->setStyleSheet(panelStyle);
                update();
            });
}

bool LoginWindow::validateInput()
{
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text();

    if (username.isEmpty()) {
        showError(QString::fromUtf8("请输入用户名或邮箱"));
        m_usernameEdit->setFocus();
        return false;
    }

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

    return true;
}

void LoginWindow::showError(const QString &message)
{
    m_errorLabel->setText(message);
    m_errorLabel->show();

    // 3秒后自动隐藏
    QTimer::singleShot(3000, m_errorLabel, &QLabel::hide);
}
