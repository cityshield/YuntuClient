/**
 * @file TaskDetailDialog.cpp
 * @brief 任务详情对话框实现
 */

#include "TaskDetailDialog.h"
#include "../ThemeManager.h"
#include "../../managers/TaskManager.h"
#include "../../core/Logger.h"
#include "../../core/Application.h"
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QMessageBox>

TaskDetailDialog::TaskDetailDialog(Task *task, QWidget *parent)
    : QDialog(parent)
    , m_task(task)
    , m_dialogPanel(nullptr)
    , m_titleLabel(nullptr)
    , m_closeButton(nullptr)
    , m_taskNameLabel(nullptr)
    , m_statusLabel(nullptr)
    , m_progressBar(nullptr)
    , m_progressLabel(nullptr)
    , m_sceneFileLabel(nullptr)
    , m_rendererLabel(nullptr)
    , m_framesLabel(nullptr)
    , m_resolutionLabel(nullptr)
    , m_outputLabel(nullptr)
    , m_createdAtLabel(nullptr)
    , m_startedAtLabel(nullptr)
    , m_completedAtLabel(nullptr)
    , m_durationLabel(nullptr)
    , m_estimatedCostLabel(nullptr)
    , m_actualCostLabel(nullptr)
    , m_errorLabel(nullptr)
    , m_logsTextEdit(nullptr)
    , m_pauseButton(nullptr)
    , m_resumeButton(nullptr)
    , m_cancelButton(nullptr)
    , m_downloadButton(nullptr)
    , m_tabWidget(nullptr)
{
    // 设置对话框属性
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
    resize(800, 600);

    initUI();
    connectSignals();
    updateDisplay();
}

TaskDetailDialog::~TaskDetailDialog()
{
}

void TaskDetailDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制半透明遮罩背景
    painter.fillRect(rect(), QColor(0, 0, 0, 100));
}

void TaskDetailDialog::onTaskDataChanged()
{
    updateDisplay();
}

void TaskDetailDialog::onPauseClicked()
{
    if (m_task) {
        TaskManager::instance().pauseTask(m_task->taskId());
        Application::instance().logger()->info("TaskDetailDialog",
            QString::fromUtf8("暂停任务: %1").arg(m_task->taskId()));
    }
}

void TaskDetailDialog::onResumeClicked()
{
    if (m_task) {
        TaskManager::instance().resumeTask(m_task->taskId());
        Application::instance().logger()->info("TaskDetailDialog",
            QString::fromUtf8("恢复任务: %1").arg(m_task->taskId()));
    }
}

void TaskDetailDialog::onCancelClicked()
{
    if (m_task) {
        QMessageBox::StandardButton reply = QMessageBox::question(this,
            QString::fromUtf8("确认"),
            QString::fromUtf8("确定要取消这个任务吗？"),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            TaskManager::instance().cancelTask(m_task->taskId());
            Application::instance().logger()->info("TaskDetailDialog",
                QString::fromUtf8("取消任务: %1").arg(m_task->taskId()));
        }
    }
}

void TaskDetailDialog::onDownloadClicked()
{
    if (m_task) {
        // TODO: 实现下载功能
        QMessageBox::information(this, QString::fromUtf8("下载"),
            QString::fromUtf8("下载功能开发中..."));
    }
}

void TaskDetailDialog::onCloseClicked()
{
    accept();
}

void TaskDetailDialog::initUI()
{
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 对话框面板
    m_dialogPanel = new QWidget(this);
    m_dialogPanel->setFixedSize(800, 600);

    // 面板布局
    QVBoxLayout *panelLayout = new QVBoxLayout(m_dialogPanel);
    panelLayout->setContentsMargins(0, 0, 0, 0);
    panelLayout->setSpacing(0);

    // 标题栏
    QWidget *titleBar = new QWidget(m_dialogPanel);
    titleBar->setFixedHeight(50);
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(20, 0, 12, 0);

    m_titleLabel = new QLabel(QString::fromUtf8("任务详情"), titleBar);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);

    m_closeButton = new FluentButton(QString::fromUtf8("×"), titleBar);
    m_closeButton->setFixedSize(36, 36);

    titleLayout->addWidget(m_titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(m_closeButton);

    // 任务名称和状态
    QWidget *headerWidget = new QWidget(m_dialogPanel);
    QVBoxLayout *headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setContentsMargins(20, 16, 20, 16);
    headerLayout->setSpacing(12);

    QHBoxLayout *nameStatusLayout = new QHBoxLayout();
    m_taskNameLabel = new QLabel(headerWidget);
    QFont nameFont = m_taskNameLabel->font();
    nameFont.setPointSize(16);
    nameFont.setBold(true);
    m_taskNameLabel->setFont(nameFont);

    m_statusLabel = new QLabel(headerWidget);
    m_statusLabel->setStyleSheet("padding: 6px 12px; border-radius: 4px;");

    nameStatusLayout->addWidget(m_taskNameLabel);
    nameStatusLayout->addWidget(m_statusLabel);
    nameStatusLayout->addStretch();

    // 进度条
    QHBoxLayout *progressLayout = new QHBoxLayout();
    m_progressBar = new QProgressBar(headerWidget);
    m_progressBar->setFixedHeight(10);
    m_progressBar->setTextVisible(false);
    m_progressBar->setRange(0, 100);

    m_progressLabel = new QLabel(QString::fromUtf8("0%"), headerWidget);
    m_progressLabel->setFixedWidth(50);

    progressLayout->addWidget(m_progressBar);
    progressLayout->addWidget(m_progressLabel);

    headerLayout->addLayout(nameStatusLayout);
    headerLayout->addLayout(progressLayout);

    // 标签页
    m_tabWidget = new QTabWidget(m_dialogPanel);
    m_tabWidget->addTab(createBasicInfoTab(), QString::fromUtf8("基本信息"));
    m_tabWidget->addTab(createLogsTab(), QString::fromUtf8("渲染日志"));

    // 底部按钮栏
    QWidget *buttonBar = new QWidget(m_dialogPanel);
    buttonBar->setFixedHeight(70);
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonBar);
    buttonLayout->setContentsMargins(20, 16, 20, 16);
    buttonLayout->setSpacing(12);

    m_pauseButton = new FluentButton(QString::fromUtf8("暂停"), buttonBar);
    m_resumeButton = new FluentButton(QString::fromUtf8("恢复"), buttonBar);
    m_cancelButton = new FluentButton(QString::fromUtf8("取消"), buttonBar);
    m_downloadButton = new FluentButton(QString::fromUtf8("下载结果"), buttonBar);
    m_downloadButton->setIsPrimary(true);

    buttonLayout->addWidget(m_pauseButton);
    buttonLayout->addWidget(m_resumeButton);
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_downloadButton);

    // 添加到面板布局
    panelLayout->addWidget(titleBar);
    panelLayout->addWidget(headerWidget);
    panelLayout->addWidget(m_tabWidget, 1);
    panelLayout->addWidget(buttonBar);

    // 将面板居中
    mainLayout->addStretch();
    mainLayout->addWidget(m_dialogPanel, 0, Qt::AlignCenter);
    mainLayout->addStretch();

    // 设置面板样式
    ThemeManager &theme = ThemeManager::instance();
    QString panelStyle = QString("background-color: %1; border-radius: 12px;")
        .arg(theme.getSurfaceColor().name());
    m_dialogPanel->setStyleSheet(panelStyle);

    // 应用阴影效果
    ThemeManager::instance().applyShadowEffect(m_dialogPanel, 40, 0, 10);
}

QWidget* TaskDetailDialog::createBasicInfoTab()
{
    QWidget *tab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);

    // 使用网格布局显示信息
    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->setHorizontalSpacing(20);
    gridLayout->setVerticalSpacing(12);

    int row = 0;

    // 场景文件
    QLabel *sceneLabel = new QLabel(QString::fromUtf8("场景文件:"), tab);
    sceneLabel->setStyleSheet("font-weight: bold;");
    m_sceneFileLabel = new QLabel(tab);
    gridLayout->addWidget(sceneLabel, row, 0);
    gridLayout->addWidget(m_sceneFileLabel, row, 1);
    row++;

    // 渲染器
    QLabel *rendererLabel = new QLabel(QString::fromUtf8("渲染器:"), tab);
    rendererLabel->setStyleSheet("font-weight: bold;");
    m_rendererLabel = new QLabel(tab);
    gridLayout->addWidget(rendererLabel, row, 0);
    gridLayout->addWidget(m_rendererLabel, row, 1);
    row++;

    // 帧范围
    QLabel *framesLabel = new QLabel(QString::fromUtf8("帧范围:"), tab);
    framesLabel->setStyleSheet("font-weight: bold;");
    m_framesLabel = new QLabel(tab);
    gridLayout->addWidget(framesLabel, row, 0);
    gridLayout->addWidget(m_framesLabel, row, 1);
    row++;

    // 分辨率
    QLabel *resolutionLabel = new QLabel(QString::fromUtf8("分辨率:"), tab);
    resolutionLabel->setStyleSheet("font-weight: bold;");
    m_resolutionLabel = new QLabel(tab);
    gridLayout->addWidget(resolutionLabel, row, 0);
    gridLayout->addWidget(m_resolutionLabel, row, 1);
    row++;

    // 输出格式
    QLabel *outputLabel = new QLabel(QString::fromUtf8("输出格式:"), tab);
    outputLabel->setStyleSheet("font-weight: bold;");
    m_outputLabel = new QLabel(tab);
    gridLayout->addWidget(outputLabel, row, 0);
    gridLayout->addWidget(m_outputLabel, row, 1);
    row++;

    // 创建时间
    QLabel *createdLabel = new QLabel(QString::fromUtf8("创建时间:"), tab);
    createdLabel->setStyleSheet("font-weight: bold;");
    m_createdAtLabel = new QLabel(tab);
    gridLayout->addWidget(createdLabel, row, 0);
    gridLayout->addWidget(m_createdAtLabel, row, 1);
    row++;

    // 开始时间
    QLabel *startedLabel = new QLabel(QString::fromUtf8("开始时间:"), tab);
    startedLabel->setStyleSheet("font-weight: bold;");
    m_startedAtLabel = new QLabel(tab);
    gridLayout->addWidget(startedLabel, row, 0);
    gridLayout->addWidget(m_startedAtLabel, row, 1);
    row++;

    // 完成时间
    QLabel *completedLabel = new QLabel(QString::fromUtf8("完成时间:"), tab);
    completedLabel->setStyleSheet("font-weight: bold;");
    m_completedAtLabel = new QLabel(tab);
    gridLayout->addWidget(completedLabel, row, 0);
    gridLayout->addWidget(m_completedAtLabel, row, 1);
    row++;

    // 用时
    QLabel *durationLabel = new QLabel(QString::fromUtf8("用时:"), tab);
    durationLabel->setStyleSheet("font-weight: bold;");
    m_durationLabel = new QLabel(tab);
    gridLayout->addWidget(durationLabel, row, 0);
    gridLayout->addWidget(m_durationLabel, row, 1);
    row++;

    // 预估费用
    QLabel *estimatedLabel = new QLabel(QString::fromUtf8("预估费用:"), tab);
    estimatedLabel->setStyleSheet("font-weight: bold;");
    m_estimatedCostLabel = new QLabel(tab);
    gridLayout->addWidget(estimatedLabel, row, 0);
    gridLayout->addWidget(m_estimatedCostLabel, row, 1);
    row++;

    // 实际费用
    QLabel *actualLabel = new QLabel(QString::fromUtf8("实际费用:"), tab);
    actualLabel->setStyleSheet("font-weight: bold;");
    m_actualCostLabel = new QLabel(tab);
    gridLayout->addWidget(actualLabel, row, 0);
    gridLayout->addWidget(m_actualCostLabel, row, 1);
    row++;

    // 错误信息（如果有）
    QLabel *errorLabel = new QLabel(QString::fromUtf8("错误信息:"), tab);
    errorLabel->setStyleSheet("font-weight: bold; color: #D13438;");
    m_errorLabel = new QLabel(tab);
    m_errorLabel->setStyleSheet("color: #D13438;");
    m_errorLabel->setWordWrap(true);
    gridLayout->addWidget(errorLabel, row, 0);
    gridLayout->addWidget(m_errorLabel, row, 1);

    layout->addLayout(gridLayout);
    layout->addStretch();

    return tab;
}

QWidget* TaskDetailDialog::createLogsTab()
{
    QWidget *tab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(20, 20, 20, 20);

    m_logsTextEdit = new QTextEdit(tab);
    m_logsTextEdit->setReadOnly(true);
    m_logsTextEdit->setPlaceholderText(QString::fromUtf8("暂无日志"));

    layout->addWidget(m_logsTextEdit);

    return tab;
}

void TaskDetailDialog::updateDisplay()
{
    if (!m_task) return;

    // 更新任务名称
    m_taskNameLabel->setText(m_task->taskName());

    // 更新状态
    m_statusLabel->setText(m_task->statusString());

    // 更新进度
    m_progressBar->setValue(m_task->progress());
    m_progressLabel->setText(QString("%1%").arg(m_task->progress()));

    // 更新基本信息
    m_sceneFileLabel->setText(m_task->sceneFile());
    m_rendererLabel->setText(m_task->renderer());

    m_framesLabel->setText(QString::fromUtf8("%1 - %2 (步长 %3), 共 %4 帧")
        .arg(m_task->startFrame())
        .arg(m_task->endFrame())
        .arg(m_task->frameStep())
        .arg(m_task->totalFrames()));

    m_resolutionLabel->setText(QString("%1 x %2")
        .arg(m_task->width())
        .arg(m_task->height()));

    m_outputLabel->setText(m_task->outputFormat());

    m_createdAtLabel->setText(m_task->createdAt().toString("yyyy-MM-dd hh:mm:ss"));
    m_startedAtLabel->setText(m_task->startedAt().isValid() ?
        m_task->startedAt().toString("yyyy-MM-dd hh:mm:ss") : QString::fromUtf8("未开始"));
    m_completedAtLabel->setText(m_task->completedAt().isValid() ?
        m_task->completedAt().toString("yyyy-MM-dd hh:mm:ss") : QString::fromUtf8("未完成"));

    m_durationLabel->setText(m_task->durationString());

    m_estimatedCostLabel->setText(QString::fromUtf8("¥ %1").arg(m_task->estimatedCost(), 0, 'f', 2));
    m_actualCostLabel->setText(QString::fromUtf8("¥ %1").arg(m_task->actualCost(), 0, 'f', 2));

    // 错误信息
    if (!m_task->errorMessage().isEmpty()) {
        m_errorLabel->setText(m_task->errorMessage());
    } else {
        m_errorLabel->setText(QString::fromUtf8("无"));
    }

    // 渲染日志
    m_logsTextEdit->setText(m_task->renderLogs().join("\n"));

    // 更新按钮
    updateButtons();
}

void TaskDetailDialog::updateButtons()
{
    if (!m_task) return;

    m_pauseButton->setVisible(m_task->canPause());
    m_resumeButton->setVisible(m_task->canResume());
    m_cancelButton->setVisible(m_task->canCancel());

    // 下载按钮仅在任务完成时可用
    m_downloadButton->setEnabled(m_task->status() == TaskStatus::Completed);
}

void TaskDetailDialog::connectSignals()
{
    if (m_task) {
        connect(m_task, &Task::taskDataChanged, this, &TaskDetailDialog::onTaskDataChanged);
    }

    connect(m_closeButton, &FluentButton::clicked, this, &TaskDetailDialog::onCloseClicked);
    connect(m_pauseButton, &FluentButton::clicked, this, &TaskDetailDialog::onPauseClicked);
    connect(m_resumeButton, &FluentButton::clicked, this, &TaskDetailDialog::onResumeClicked);
    connect(m_cancelButton, &FluentButton::clicked, this, &TaskDetailDialog::onCancelClicked);
    connect(m_downloadButton, &FluentButton::clicked, this, &TaskDetailDialog::onDownloadClicked);

    // 主题变更时更新面板背景
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [this](ThemeType theme) {
                Q_UNUSED(theme);
                ThemeManager &themeMgr = ThemeManager::instance();
                QString panelStyle = QString("background-color: %1; border-radius: 12px;")
                    .arg(themeMgr.getSurfaceColor().name());
                m_dialogPanel->setStyleSheet(panelStyle);
            });
}
