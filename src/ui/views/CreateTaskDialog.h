/**
 * @file CreateTaskDialog.h
 * @brief 新建任务对话框
 */

#ifndef CREATETASKDIALOG_H
#define CREATETASKDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QSpinBox>
#include "../components/FluentButton.h"
#include "../components/FluentLineEdit.h"
#include "../../models/Task.h"
#include "../../models/RenderConfig.h"

/**
 * @brief 新建任务对话框
 *
 * 创建新的渲染任务，包括：
 * - 任务基本信息（名称、场景文件）
 * - 渲染参数（帧范围、分辨率、格式）
 * - 渲染器选择
 * - 优先级设置
 */
class CreateTaskDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateTaskDialog(QWidget *parent = nullptr);
    ~CreateTaskDialog();

    /**
     * @brief 获取创建的任务
     */
    Task* getCreatedTask() const { return m_task; }

protected:
    /**
     * @brief 绘制事件（背景）
     */
    void paintEvent(QPaintEvent *event) override;

private slots:
    /**
     * @brief 浏览场景文件
     */
    void onBrowseSceneClicked();

    /**
     * @brief 检测场景信息
     */
    void onDetectSceneClicked();

    /**
     * @brief 创建任务
     */
    void onCreateClicked();

    /**
     * @brief 取消创建
     */
    void onCancelClicked();

private:
    /**
     * @brief 初始化 UI
     */
    void initUI();

    /**
     * @brief 验证输入
     */
    bool validateInput();

    /**
     * @brief 创建任务对象
     */
    void createTask();

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
    FluentLineEdit *m_taskNameEdit;
    FluentLineEdit *m_sceneFileEdit;
    FluentButton *m_browseButton;
    FluentButton *m_detectButton;

    // 渲染参数
    QSpinBox *m_startFrameSpinBox;
    QSpinBox *m_endFrameSpinBox;
    QSpinBox *m_frameStepSpinBox;
    QSpinBox *m_widthSpinBox;
    QSpinBox *m_heightSpinBox;

    // 渲染器和输出
    QComboBox *m_rendererComboBox;
    QComboBox *m_outputFormatComboBox;
    FluentLineEdit *m_outputPathEdit;

    // 优先级
    QComboBox *m_priorityComboBox;

    // 操作按钮
    FluentButton *m_createButton;
    FluentButton *m_cancelButton;

    // 检测状态
    QLabel *m_detectStatusLabel;
};

#endif // CREATETASKDIALOG_H
