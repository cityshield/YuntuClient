/**
 * @file MainWindow.cpp
 * @brief 主窗口实现
 */

#include "MainWindow.h"
#include "CreateTaskDialog.h"
#include "TaskDetailDialog.h"
#include "../ThemeManager.h"
#include "../components/TaskItemWidget.h"
#include "../../managers/AuthManager.h"
#include "../../managers/TaskManager.h"
#include "../../managers/UserManager.h"
#include "../../models/Task.h"
#include "../../core/Logger.h"
#include "../../core/Application.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QScreen>
#include <QApplication>
#include <QScrollArea>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , m_dragPosition()
    , m_isDragging(false)
    , m_titleBar(nullptr)
    , m_titleLabel(nullptr)
    , m_userNameLabel(nullptr)
    , m_userAvatarLabel(nullptr)
    , m_themeToggleButton(nullptr)
    , m_minimizeButton(nullptr)
    , m_maximizeButton(nullptr)
    , m_closeButton(nullptr)
    , m_navigationBar(nullptr)
    , m_navigationList(nullptr)
    , m_contentStack(nullptr)
    , m_taskPage(nullptr)
    , m_settingsPage(nullptr)
    , m_aboutPage(nullptr)
    , m_createTaskButton(nullptr)
    , m_refreshButton(nullptr)
    , m_mainLayout(nullptr)
{
    initUI();
    connectSignals();

    // 设置窗口属性
    setWindowTitle(QString::fromUtf8("盛世云图 - Maya 云渲染平台"));
    resize(1280, 800);

    // 无边框窗口
    setWindowFlag(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // 居中显示
    QScreen *screen = QApplication::primaryScreen();
    QPoint screenCenter = screen->geometry().center();
    move(screenCenter - rect().center());

    // 初始化管理器
    TaskManager::instance().initialize();
    UserManager::instance().initialize();

    // 更新用户信息
    updateUserInfo();

    // 显示任务页面
    showPage(0);
}

MainWindow::~MainWindow()
{
}

void MainWindow::showPage(int index)
{
    if (index >= 0 && index < m_contentStack->count()) {
        m_contentStack->setCurrentIndex(index);
        m_navigationList->setCurrentRow(index);
    }
}

void MainWindow::paintEvent(QPaintEvent *event)
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

    // 绘制边框
    painter.setPen(QPen(theme.getBorderColor(), 1));
    painter.drawPath(path);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // 检查是否点击在标题栏区域
        if (m_titleBar && m_titleBar->geometry().contains(event->pos())) {
            m_isDragging = true;
            m_dragPosition = event->globalPos() - frameGeometry().topLeft();
            event->accept();
        }
    }
    QWidget::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
    QWidget::mouseMoveEvent(event);
}

void MainWindow::onNavigationItemClicked(int index)
{
    showPage(index);
}

void MainWindow::onMinimizeClicked()
{
    showMinimized();
}

void MainWindow::onMaximizeClicked()
{
    if (isMaximized()) {
        showNormal();
        m_maximizeButton->setText(QString::fromUtf8("□"));
    } else {
        showMaximized();
        m_maximizeButton->setText(QString::fromUtf8("❐"));
    }
}

void MainWindow::onCloseClicked()
{
    close();
}

void MainWindow::onUserAvatarClicked()
{
    // TODO: 显示用户菜单
}

void MainWindow::onThemeToggleClicked()
{
    ThemeManager::instance().toggleTheme();
}

void MainWindow::onCreateTaskClicked()
{
    Application::instance().logger()->info("MainWindow", QString::fromUtf8("打开创建任务对话框"));

    CreateTaskDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        Task* createdTask = dialog.getCreatedTask();
        if (createdTask) {
            Application::instance().logger()->info("MainWindow",
                QString::fromUtf8("任务创建成功: %1").arg(createdTask->taskName()));

            // 提交任务到任务管理器
            TaskManager::instance().submitTask(createdTask);

            // 刷新任务列表
            onRefreshClicked();
        }
    }
}

void MainWindow::onRefreshClicked()
{
    Application::instance().logger()->info("MainWindow", QString::fromUtf8("刷新任务列表"));
    TaskManager::instance().refreshTaskList();
}

void MainWindow::onLogoutClicked()
{
    AuthManager::instance().logout();
    close();
    // TODO: 打开登录窗口
}

void MainWindow::onViewTaskDetails(Task *task)
{
    if (!task) {
        return;
    }

    Application::instance().logger()->info("MainWindow",
        QString::fromUtf8("打开任务详情: %1").arg(task->taskName()));

    TaskDetailDialog dialog(task, this);
    dialog.exec();
}

void MainWindow::initUI()
{
    // 主布局
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // 标题栏
    m_titleBar = createTitleBar();

    // 内容布局（导航栏 + 主内容）
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    // 导航栏
    m_navigationBar = createNavigationBar();

    // 主内容区域
    m_contentStack = new QStackedWidget(this);

    // 创建各个页面
    m_taskPage = createTaskPage();
    m_settingsPage = createSettingsPage();
    m_aboutPage = createAboutPage();

    m_contentStack->addWidget(m_taskPage);
    m_contentStack->addWidget(m_settingsPage);
    m_contentStack->addWidget(m_aboutPage);

    contentLayout->addWidget(m_navigationBar);
    contentLayout->addWidget(m_contentStack, 1);

    // 添加到主布局
    m_mainLayout->addWidget(m_titleBar);
    m_mainLayout->addLayout(contentLayout, 1);
}

QWidget* MainWindow::createTitleBar()
{
    QWidget *titleBar = new QWidget(this);
    titleBar->setFixedHeight(50);

    QHBoxLayout *layout = new QHBoxLayout(titleBar);
    layout->setContentsMargins(16, 0, 8, 0);
    layout->setSpacing(12);

    // 标题
    m_titleLabel = new QLabel(QString::fromUtf8("盛世云图"), titleBar);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);

    // 用户信息
    m_userAvatarLabel = new QLabel(titleBar);
    m_userAvatarLabel->setFixedSize(32, 32);
    m_userAvatarLabel->setStyleSheet("background-color: #0078D4; border-radius: 16px; color: white;");
    m_userAvatarLabel->setAlignment(Qt::AlignCenter);
    m_userAvatarLabel->setCursor(Qt::PointingHandCursor);

    m_userNameLabel = new QLabel(QString::fromUtf8("用户"), titleBar);

    // 主题切换按钮
    m_themeToggleButton = new FluentButton(QString::fromUtf8("🌙"), titleBar);
    m_themeToggleButton->setFixedSize(36, 36);

    // 窗口控制按钮
    m_minimizeButton = new FluentButton(QString::fromUtf8("−"), titleBar);
    m_minimizeButton->setFixedSize(36, 36);

    m_maximizeButton = new FluentButton(QString::fromUtf8("□"), titleBar);
    m_maximizeButton->setFixedSize(36, 36);

    m_closeButton = new FluentButton(QString::fromUtf8("×"), titleBar);
    m_closeButton->setFixedSize(36, 36);

    layout->addWidget(m_titleLabel);
    layout->addStretch();
    layout->addWidget(m_userAvatarLabel);
    layout->addWidget(m_userNameLabel);
    layout->addWidget(m_themeToggleButton);
    layout->addWidget(m_minimizeButton);
    layout->addWidget(m_maximizeButton);
    layout->addWidget(m_closeButton);

    // 设置背景色
    ThemeManager &theme = ThemeManager::instance();
    QString style = QString("background-color: %1;").arg(theme.getSurfaceColor().name());
    titleBar->setStyleSheet(style);

    return titleBar;
}

QWidget* MainWindow::createNavigationBar()
{
    QWidget *navBar = new QWidget(this);
    navBar->setFixedWidth(200);

    QVBoxLayout *layout = new QVBoxLayout(navBar);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    // 导航列表
    m_navigationList = new QListWidget(navBar);
    m_navigationList->setFrameShape(QFrame::NoFrame);
    m_navigationList->addItem(QString::fromUtf8("📋 任务列表"));
    m_navigationList->addItem(QString::fromUtf8("⚙️ 设置"));
    m_navigationList->addItem(QString::fromUtf8("ℹ️ 关于"));

    layout->addWidget(m_navigationList);

    // 设置背景色
    ThemeManager &theme = ThemeManager::instance();
    QString style = QString("background-color: %1; border-right: 1px solid %2;")
        .arg(theme.getSurfaceColor().name())
        .arg(theme.getBorderColor().name());
    navBar->setStyleSheet(style);

    return navBar;
}

QWidget* MainWindow::createTaskPage()
{
    QWidget *page = new QWidget(this);

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);

    // 顶部工具栏
    QHBoxLayout *toolbarLayout = new QHBoxLayout();

    QLabel *titleLabel = new QLabel(QString::fromUtf8("任务列表"), page);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    m_createTaskButton = new FluentButton(QString::fromUtf8("+ 新建任务"), page);
    m_createTaskButton->setIsPrimary(true);

    m_refreshButton = new FluentButton(QString::fromUtf8("🔄 刷新"), page);

    toolbarLayout->addWidget(titleLabel);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(m_refreshButton);
    toolbarLayout->addWidget(m_createTaskButton);

    // 任务列表容器（使用滚动区域）
    QScrollArea *scrollArea = new QScrollArea(page);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *scrollContent = new QWidget(scrollArea);
    QVBoxLayout *taskListLayout = new QVBoxLayout(scrollContent);
    taskListLayout->setContentsMargins(0, 0, 0, 0);
    taskListLayout->setSpacing(12);

    // 创建演示任务 1 - 渲染中
    Task *task1 = new Task(this);
    task1->setTaskId("demo_task_001");
    task1->setTaskName(QString::fromUtf8("渲染场景 - 办公室内景"));
    task1->setSceneFile("C:/Projects/Maya/office_scene.ma");
    task1->setStatus(TaskStatus::Rendering);
    task1->setProgress(65);
    task1->setStartFrame(1);
    task1->setEndFrame(240);
    task1->setFrameStep(1);
    task1->setWidth(1920);
    task1->setHeight(1080);
    task1->setRenderer("Arnold");
    task1->setOutputFormat("exr");
    task1->setCreatedAt(QDateTime::currentDateTime().addSecs(-3600));
    task1->setStartedAt(QDateTime::currentDateTime().addSecs(-1800));
    task1->setEstimatedCost(45.50);

    TaskItemWidget *taskItem1 = new TaskItemWidget(task1, scrollContent);
    connect(taskItem1, &TaskItemWidget::viewDetailsClicked,
            this, &MainWindow::onViewTaskDetails);
    taskListLayout->addWidget(taskItem1);

    // 创建演示任务 2 - 队列中
    Task *task2 = new Task(this);
    task2->setTaskId("demo_task_002");
    task2->setTaskName(QString::fromUtf8("产品展示动画"));
    task2->setSceneFile("C:/Projects/Maya/product_showcase.ma");
    task2->setStatus(TaskStatus::Queued);
    task2->setProgress(0);
    task2->setStartFrame(1);
    task2->setEndFrame(180);
    task2->setFrameStep(1);
    task2->setWidth(1920);
    task2->setHeight(1080);
    task2->setRenderer("V-Ray");
    task2->setOutputFormat("png");
    task2->setCreatedAt(QDateTime::currentDateTime().addSecs(-600));
    task2->setEstimatedCost(32.00);

    TaskItemWidget *taskItem2 = new TaskItemWidget(task2, scrollContent);
    connect(taskItem2, &TaskItemWidget::viewDetailsClicked,
            this, &MainWindow::onViewTaskDetails);
    taskListLayout->addWidget(taskItem2);

    // 创建演示任务 3 - 已完成
    Task *task3 = new Task(this);
    task3->setTaskId("demo_task_003");
    task3->setTaskName(QString::fromUtf8("建筑外观渲染"));
    task3->setSceneFile("C:/Projects/Maya/building_exterior.ma");
    task3->setStatus(TaskStatus::Completed);
    task3->setProgress(100);
    task3->setStartFrame(1);
    task3->setEndFrame(120);
    task3->setFrameStep(1);
    task3->setWidth(3840);
    task3->setHeight(2160);
    task3->setRenderer("Arnold");
    task3->setOutputFormat("exr");
    task3->setCreatedAt(QDateTime::currentDateTime().addSecs(-7200));
    task3->setStartedAt(QDateTime::currentDateTime().addSecs(-6000));
    task3->setCompletedAt(QDateTime::currentDateTime().addSecs(-300));
    task3->setEstimatedCost(68.00);
    task3->setActualCost(65.50);

    TaskItemWidget *taskItem3 = new TaskItemWidget(task3, scrollContent);
    connect(taskItem3, &TaskItemWidget::viewDetailsClicked,
            this, &MainWindow::onViewTaskDetails);
    taskListLayout->addWidget(taskItem3);

    taskListLayout->addStretch();

    scrollArea->setWidget(scrollContent);

    layout->addLayout(toolbarLayout);
    layout->addWidget(scrollArea);

    return page;
}

QWidget* MainWindow::createSettingsPage()
{
    QWidget *page = new QWidget(this);

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);

    QLabel *titleLabel = new QLabel(QString::fromUtf8("设置"), page);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    QLabel *contentLabel = new QLabel(QString::fromUtf8("设置功能开发中..."), page);

    FluentButton *logoutButton = new FluentButton(QString::fromUtf8("登出"), page);
    connect(logoutButton, &FluentButton::clicked, this, &MainWindow::onLogoutClicked);

    layout->addWidget(titleLabel);
    layout->addWidget(contentLabel);
    layout->addStretch();
    layout->addWidget(logoutButton);

    return page;
}

QWidget* MainWindow::createAboutPage()
{
    QWidget *page = new QWidget(this);

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);

    QLabel *titleLabel = new QLabel(QString::fromUtf8("关于"), page);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    QLabel *appNameLabel = new QLabel(QString::fromUtf8("盛世云图"), page);
    QFont appNameFont = appNameLabel->font();
    appNameFont.setPointSize(24);
    appNameFont.setBold(true);
    appNameLabel->setFont(appNameFont);

    QLabel *versionLabel = new QLabel(QString::fromUtf8("版本: 1.0.0-alpha"), page);
    QLabel *descLabel = new QLabel(QString::fromUtf8("Maya 云渲染平台客户端"), page);

    layout->addWidget(titleLabel);
    layout->addSpacing(20);
    layout->addWidget(appNameLabel);
    layout->addWidget(versionLabel);
    layout->addWidget(descLabel);
    layout->addStretch();

    return page;
}

void MainWindow::connectSignals()
{
    // 导航列表
    connect(m_navigationList, &QListWidget::currentRowChanged,
            this, &MainWindow::onNavigationItemClicked);

    // 标题栏按钮
    connect(m_themeToggleButton, &FluentButton::clicked,
            this, &MainWindow::onThemeToggleClicked);
    connect(m_minimizeButton, &FluentButton::clicked,
            this, &MainWindow::onMinimizeClicked);
    connect(m_maximizeButton, &FluentButton::clicked,
            this, &MainWindow::onMaximizeClicked);
    connect(m_closeButton, &FluentButton::clicked,
            this, &MainWindow::onCloseClicked);

    // 任务页面按钮
    connect(m_createTaskButton, &FluentButton::clicked,
            this, &MainWindow::onCreateTaskClicked);
    connect(m_refreshButton, &FluentButton::clicked,
            this, &MainWindow::onRefreshClicked);

    // 主题变更时更新样式
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [this](ThemeType theme) {
                ThemeManager &themeMgr = ThemeManager::instance();

                // 更新标题栏背景
                QString titleBarStyle = QString("background-color: %1;")
                    .arg(themeMgr.getSurfaceColor().name());
                m_titleBar->setStyleSheet(titleBarStyle);

                // 更新导航栏背景
                QString navBarStyle = QString("background-color: %1; border-right: 1px solid %2;")
                    .arg(themeMgr.getSurfaceColor().name())
                    .arg(themeMgr.getBorderColor().name());
                m_navigationBar->setStyleSheet(navBarStyle);

                // 更新主题按钮图标
                m_themeToggleButton->setText(theme == ThemeType::Dark ?
                    QString::fromUtf8("☀️") : QString::fromUtf8("🌙"));

                update();
            });
}

void MainWindow::updateUserInfo()
{
    User* user = AuthManager::instance().currentUser();
    if (user) {
        m_userNameLabel->setText(user->username());

        // 设置头像（使用首字母）
        QString initial = user->username().left(1).toUpper();
        m_userAvatarLabel->setText(initial);
    }
}
