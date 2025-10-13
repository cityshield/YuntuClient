/**
 * @file TaskItemWidget.h
 * @brief 任务列表项组件
 */

#ifndef TASKITEMWIDGET_H
#define TASKITEMWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "FluentButton.h"
#include "../../models/Task.h"

/**
 * @brief 任务列表项组件
 *
 * 显示单个任务的信息，包括：
 * - 任务名称和状态
 * - 进度条
 * - 时间信息
 * - 操作按钮
 */
class TaskItemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TaskItemWidget(Task *task, QWidget *parent = nullptr);
    ~TaskItemWidget();

    /**
     * @brief 获取关联的任务
     */
    Task* task() const { return m_task; }

    /**
     * @brief 更新显示
     */
    void updateDisplay();

signals:
    /**
     * @brief 查看详情按钮点击
     */
    void viewDetailsClicked(Task *task);

    /**
     * @brief 暂停按钮点击
     */
    void pauseClicked(Task *task);

    /**
     * @brief 恢复按钮点击
     */
    void resumeClicked(Task *task);

    /**
     * @brief 取消按钮点击
     */
    void cancelClicked(Task *task);

    /**
     * @brief 删除按钮点击
     */
    void deleteClicked(Task *task);

protected:
    /**
     * @brief 绘制事件
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief 鼠标进入事件
     */
    void enterEvent(QEnterEvent *event) override;

    /**
     * @brief 鼠标离开事件
     */
    void leaveEvent(QEvent *event) override;

private slots:
    void onTaskDataChanged();
    void onStatusChanged();
    void onProgressChanged();

private:
    void initUI();
    void updateStatusLabel();
    void updateProgressBar();
    void updateButtons();
    void connectSignals();

    /**
     * @brief 获取状态颜色
     */
    QColor getStatusColor() const;

    /**
     * @brief 获取状态图标
     */
    QString getStatusIcon() const;

private:
    Task *m_task;
    bool m_isHovered;

    // UI 组件
    QLabel *m_taskNameLabel;
    QLabel *m_statusLabel;
    QLabel *m_timeLabel;
    QLabel *m_framesLabel;
    QProgressBar *m_progressBar;
    QLabel *m_progressLabel;

    // 操作按钮
    FluentButton *m_viewButton;
    FluentButton *m_pauseButton;
    FluentButton *m_resumeButton;
    FluentButton *m_cancelButton;
    FluentButton *m_deleteButton;

    // 布局
    QVBoxLayout *m_mainLayout;
};

#endif // TASKITEMWIDGET_H
