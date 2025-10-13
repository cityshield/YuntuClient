/**
 * @file TaskDetailDialog.h
 * @brief 任务详情对话框
 */

#ifndef TASKDETAILDIALOG_H
#define TASKDETAILDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QTextEdit>
#include <QProgressBar>
#include "../components/FluentButton.h"
#include "../../models/Task.h"

/**
 * @brief 任务详情对话框
 *
 * 显示任务的完整信息，包括：
 * - 基本信息（名称、状态、进度）
 * - 渲染参数（帧范围、分辨率、渲染器等）
 * - 时间信息（创建、开始、完成时间）
 * - 费用信息
 * - 渲染日志
 */
class TaskDetailDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TaskDetailDialog(Task *task, QWidget *parent = nullptr);
    ~TaskDetailDialog();

protected:
    /**
     * @brief 绘制事件（背景）
     */
    void paintEvent(QPaintEvent *event) override;

private slots:
    /**
     * @brief 任务数据变更
     */
    void onTaskDataChanged();

    /**
     * @brief 暂停任务
     */
    void onPauseClicked();

    /**
     * @brief 恢复任务
     */
    void onResumeClicked();

    /**
     * @brief 取消任务
     */
    void onCancelClicked();

    /**
     * @brief 下载结果
     */
    void onDownloadClicked();

    /**
     * @brief 关闭对话框
     */
    void onCloseClicked();

private:
    /**
     * @brief 初始化 UI
     */
    void initUI();

    /**
     * @brief 创建基本信息标签页
     */
    QWidget* createBasicInfoTab();

    /**
     * @brief 创建渲染日志标签页
     */
    QWidget* createLogsTab();

    /**
     * @brief 更新显示
     */
    void updateDisplay();

    /**
     * @brief 更新按钮状态
     */
    void updateButtons();

    /**
     * @brief 连接信号
     */
    void connectSignals();

private:
    Task *m_task;
    QWidget *m_dialogPanel;

    // 标题栏
    QLabel *m_titleLabel;
    FluentButton *m_closeButton;

    // 基本信息
    QLabel *m_taskNameLabel;
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;
    QLabel *m_progressLabel;

    // 详细信息标签
    QLabel *m_sceneFileLabel;
    QLabel *m_rendererLabel;
    QLabel *m_framesLabel;
    QLabel *m_resolutionLabel;
    QLabel *m_outputLabel;
    QLabel *m_createdAtLabel;
    QLabel *m_startedAtLabel;
    QLabel *m_completedAtLabel;
    QLabel *m_durationLabel;
    QLabel *m_estimatedCostLabel;
    QLabel *m_actualCostLabel;
    QLabel *m_errorLabel;

    // 日志
    QTextEdit *m_logsTextEdit;

    // 操作按钮
    FluentButton *m_pauseButton;
    FluentButton *m_resumeButton;
    FluentButton *m_cancelButton;
    FluentButton *m_downloadButton;

    // 标签页
    QTabWidget *m_tabWidget;
};

#endif // TASKDETAILDIALOG_H
