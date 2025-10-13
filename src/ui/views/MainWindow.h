/**
 * @file MainWindow.h
 * @brief 主窗口
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QListWidget>
#include "../components/FluentButton.h"
#include "../components/FluentCard.h"

/**
 * @brief 主窗口
 *
 * 应用程序主界面，包含：
 * - 顶部标题栏（窗口控制、用户信息）
 * - 左侧导航栏（任务、设置等）
 * - 主内容区域（任务列表、设置等页面）
 */
class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    /**
     * @brief 显示指定页面
     */
    void showPage(int index);

protected:
    /**
     * @brief 绘制事件（背景）
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief 鼠标按下事件（用于窗口拖动）
     */
    void mousePressEvent(QMouseEvent *event) override;

    /**
     * @brief 鼠标移动事件（用于窗口拖动）
     */
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    /**
     * @brief 导航项点击
     */
    void onNavigationItemClicked(int index);

    /**
     * @brief 最小化按钮点击
     */
    void onMinimizeClicked();

    /**
     * @brief 最大化按钮点击
     */
    void onMaximizeClicked();

    /**
     * @brief 关闭按钮点击
     */
    void onCloseClicked();

    /**
     * @brief 用户头像点击
     */
    void onUserAvatarClicked();

    /**
     * @brief 主题切换按钮点击
     */
    void onThemeToggleClicked();

    /**
     * @brief 新建任务按钮点击
     */
    void onCreateTaskClicked();

    /**
     * @brief 刷新按钮点击
     */
    void onRefreshClicked();

    /**
     * @brief 登出
     */
    void onLogoutClicked();

private:
    /**
     * @brief 初始化 UI
     */
    void initUI();

    /**
     * @brief 创建标题栏
     */
    QWidget* createTitleBar();

    /**
     * @brief 创建导航栏
     */
    QWidget* createNavigationBar();

    /**
     * @brief 创建任务页面
     */
    QWidget* createTaskPage();

    /**
     * @brief 创建设置页面
     */
    QWidget* createSettingsPage();

    /**
     * @brief 创建关于页面
     */
    QWidget* createAboutPage();

    /**
     * @brief 连接信号
     */
    void connectSignals();

    /**
     * @brief 更新用户信息显示
     */
    void updateUserInfo();

private:
    // 窗口拖动相关
    QPoint m_dragPosition;
    bool m_isDragging;

    // 标题栏组件
    QWidget *m_titleBar;
    QLabel *m_titleLabel;
    QLabel *m_userNameLabel;
    QLabel *m_userAvatarLabel;
    FluentButton *m_themeToggleButton;
    FluentButton *m_minimizeButton;
    FluentButton *m_maximizeButton;
    FluentButton *m_closeButton;

    // 导航栏组件
    QWidget *m_navigationBar;
    QListWidget *m_navigationList;

    // 主内容区域
    QStackedWidget *m_contentStack;
    QWidget *m_taskPage;
    QWidget *m_settingsPage;
    QWidget *m_aboutPage;

    // 任务页面组件
    QListWidget *m_taskListWidget;
    FluentButton *m_createTaskButton;
    FluentButton *m_refreshButton;

    // 布局
    QVBoxLayout *m_mainLayout;
};

#endif // MAINWINDOW_H
