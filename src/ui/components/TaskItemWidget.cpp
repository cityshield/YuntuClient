/**
 * @file TaskItemWidget.cpp
 * @brief 任务列表项组件实现
 */

#include "TaskItemWidget.h"
#include "../ThemeManager.h"
#include <QPainter>
#include <QPainterPath>

TaskItemWidget::TaskItemWidget(Task *task, QWidget *parent)
    : QWidget(parent)
    , m_task(task)
    , m_isHovered(false)
    , m_taskNameLabel(nullptr)
    , m_statusLabel(nullptr)
    , m_timeLabel(nullptr)
    , m_framesLabel(nullptr)
    , m_progressBar(nullptr)
    , m_progressLabel(nullptr)
    , m_viewButton(nullptr)
    , m_pauseButton(nullptr)
    , m_resumeButton(nullptr)
    , m_cancelButton(nullptr)
    , m_deleteButton(nullptr)
    , m_mainLayout(nullptr)
{
    initUI();
    connectSignals();
    updateDisplay();

    setMinimumHeight(120);
}

TaskItemWidget::~TaskItemWidget()
{
}

void TaskItemWidget::updateDisplay()
{
    if (!m_task) return;

    // 更新任务名称
    m_taskNameLabel->setText(m_task->taskName());

    // 更新状态
    updateStatusLabel();

    // 更新进度
    updateProgressBar();

    // 更新时间信息
    QString timeInfo;
    if (m_task->startedAt().isValid()) {
        timeInfo = QString::fromUtf8("开始: %1 | 用时: %2")
            .arg(m_task->startedAt().toString("yyyy-MM-dd hh:mm"))
            .arg(m_task->durationString());
    } else {
        timeInfo = QString::fromUtf8("创建: %1")
            .arg(m_task->createdAt().toString("yyyy-MM-dd hh:mm"));
    }
    m_timeLabel->setText(timeInfo);

    // 更新帧信息
    QString frameInfo = QString::fromUtf8("帧范围: %1-%2 (步长%3) | 分辨率: %4x%5")
        .arg(m_task->startFrame())
        .arg(m_task->endFrame())
        .arg(m_task->frameStep())
        .arg(m_task->width())
        .arg(m_task->height());
    m_framesLabel->setText(frameInfo);

    // 更新按钮状态
    updateButtons();
}

void TaskItemWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF rect = this->rect();

    // 获取主题颜色
    ThemeManager &theme = ThemeManager::instance();
    QColor bgColor = theme.getSurfaceColor();
    QColor borderColor = theme.getBorderColor();

    // Hover 效果
    if (m_isHovered) {
        bgColor = theme.getHoverColor();
        borderColor = theme.getAccentColor();
    }

    // 绘制背景
    QPainterPath path;
    path.addRoundedRect(rect, 8, 8);
    painter.fillPath(path, bgColor);

    // 绘制边框
    painter.setPen(QPen(borderColor, 1));
    painter.drawPath(path);
}

void TaskItemWidget::enterEvent(QEvent *event)
{
    QWidget::enterEvent(event);
    m_isHovered = true;
    update();
}

void TaskItemWidget::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    m_isHovered = false;
    update();
}

void TaskItemWidget::onTaskDataChanged()
{
    updateDisplay();
}

void TaskItemWidget::onStatusChanged()
{
    updateStatusLabel();
    updateButtons();
}

void TaskItemWidget::onProgressChanged()
{
    updateProgressBar();
}

void TaskItemWidget::initUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(16, 12, 16, 12);
    m_mainLayout->setSpacing(8);

    // 第一行：任务名称 + 状态 + 操作按钮
    QHBoxLayout *firstRow = new QHBoxLayout();
    firstRow->setSpacing(12);

    m_taskNameLabel = new QLabel(this);
    QFont nameFont = m_taskNameLabel->font();
    nameFont.setPointSize(12);
    nameFont.setBold(true);
    m_taskNameLabel->setFont(nameFont);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("padding: 4px 8px; border-radius: 4px;");

    // 操作按钮
    m_viewButton = new FluentButton(QString::fromUtf8("查看"), this);
    m_viewButton->setFixedHeight(28);
    m_viewButton->setMinimumWidth(60);

    m_pauseButton = new FluentButton(QString::fromUtf8("暂停"), this);
    m_pauseButton->setFixedHeight(28);
    m_pauseButton->setMinimumWidth(60);

    m_resumeButton = new FluentButton(QString::fromUtf8("恢复"), this);
    m_resumeButton->setFixedHeight(28);
    m_resumeButton->setMinimumWidth(60);

    m_cancelButton = new FluentButton(QString::fromUtf8("取消"), this);
    m_cancelButton->setFixedHeight(28);
    m_cancelButton->setMinimumWidth(60);

    m_deleteButton = new FluentButton(QString::fromUtf8("删除"), this);
    m_deleteButton->setFixedHeight(28);
    m_deleteButton->setMinimumWidth(60);

    firstRow->addWidget(m_taskNameLabel);
    firstRow->addWidget(m_statusLabel);
    firstRow->addStretch();
    firstRow->addWidget(m_viewButton);
    firstRow->addWidget(m_pauseButton);
    firstRow->addWidget(m_resumeButton);
    firstRow->addWidget(m_cancelButton);
    firstRow->addWidget(m_deleteButton);

    // 第二行：进度条 + 进度百分比
    QHBoxLayout *secondRow = new QHBoxLayout();
    secondRow->setSpacing(8);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setFixedHeight(8);
    m_progressBar->setTextVisible(false);
    m_progressBar->setRange(0, 100);

    m_progressLabel = new QLabel(QString::fromUtf8("0%"), this);
    m_progressLabel->setFixedWidth(40);

    secondRow->addWidget(m_progressBar);
    secondRow->addWidget(m_progressLabel);

    // 第三行：时间信息
    m_timeLabel = new QLabel(this);
    m_timeLabel->setStyleSheet("color: #808080; font-size: 11px;");

    // 第四行：帧信息
    m_framesLabel = new QLabel(this);
    m_framesLabel->setStyleSheet("color: #808080; font-size: 11px;");

    // 添加到主布局
    m_mainLayout->addLayout(firstRow);
    m_mainLayout->addLayout(secondRow);
    m_mainLayout->addWidget(m_timeLabel);
    m_mainLayout->addWidget(m_framesLabel);
}

void TaskItemWidget::updateStatusLabel()
{
    if (!m_task) return;

    QString statusText = getStatusIcon() + " " + m_task->statusString();
    m_statusLabel->setText(statusText);

    QColor statusColor = getStatusColor();
    QString style = QString("background-color: %1; color: white; padding: 4px 8px; border-radius: 4px;")
        .arg(statusColor.name());
    m_statusLabel->setStyleSheet(style);
}

void TaskItemWidget::updateProgressBar()
{
    if (!m_task) return;

    int progress = m_task->progress();
    m_progressBar->setValue(progress);
    m_progressLabel->setText(QString("%1%").arg(progress));
}

void TaskItemWidget::updateButtons()
{
    if (!m_task) return;

    // 根据任务状态显示/隐藏按钮
    m_pauseButton->setVisible(m_task->canPause());
    m_resumeButton->setVisible(m_task->canResume());
    m_cancelButton->setVisible(m_task->canCancel());

    // 删除按钮仅在任务完成或失败时显示
    bool canDelete = (m_task->status() == TaskStatus::Completed ||
                     m_task->status() == TaskStatus::Failed ||
                     m_task->status() == TaskStatus::Cancelled);
    m_deleteButton->setVisible(canDelete);
}

void TaskItemWidget::connectSignals()
{
    if (!m_task) return;

    // 任务数据变更信号
    connect(m_task, &Task::taskDataChanged, this, &TaskItemWidget::onTaskDataChanged);
    connect(m_task, &Task::statusChanged, this, &TaskItemWidget::onStatusChanged);
    connect(m_task, &Task::progressChanged, this, &TaskItemWidget::onProgressChanged);

    // 按钮信号
    connect(m_viewButton, &FluentButton::clicked, this, [this]() {
        emit viewDetailsClicked(m_task);
    });

    connect(m_pauseButton, &FluentButton::clicked, this, [this]() {
        emit pauseClicked(m_task);
    });

    connect(m_resumeButton, &FluentButton::clicked, this, [this]() {
        emit resumeClicked(m_task);
    });

    connect(m_cancelButton, &FluentButton::clicked, this, [this]() {
        emit cancelClicked(m_task);
    });

    connect(m_deleteButton, &FluentButton::clicked, this, [this]() {
        emit deleteClicked(m_task);
    });
}

QColor TaskItemWidget::getStatusColor() const
{
    if (!m_task) return QColor("#808080");

    switch (m_task->status()) {
        case TaskStatus::Draft:
            return QColor("#808080");  // 灰色
        case TaskStatus::Pending:
            return QColor("#FFA500");  // 橙色
        case TaskStatus::Queued:
            return QColor("#0078D4");  // 蓝色
        case TaskStatus::Rendering:
            return QColor("#107C10");  // 绿色
        case TaskStatus::Paused:
            return QColor("#FFB900");  // 黄色
        case TaskStatus::Completed:
            return QColor("#107C10");  // 绿色
        case TaskStatus::Failed:
            return QColor("#D13438");  // 红色
        case TaskStatus::Cancelled:
            return QColor("#605E5C");  // 深灰色
        default:
            return QColor("#808080");
    }
}

QString TaskItemWidget::getStatusIcon() const
{
    if (!m_task) return QString::fromUtf8("○");

    switch (m_task->status()) {
        case TaskStatus::Draft:
            return QString::fromUtf8("✏️");
        case TaskStatus::Pending:
            return QString::fromUtf8("⏳");
        case TaskStatus::Queued:
            return QString::fromUtf8("⏸️");
        case TaskStatus::Rendering:
            return QString::fromUtf8("▶️");
        case TaskStatus::Paused:
            return QString::fromUtf8("⏸️");
        case TaskStatus::Completed:
            return QString::fromUtf8("✅");
        case TaskStatus::Failed:
            return QString::fromUtf8("❌");
        case TaskStatus::Cancelled:
            return QString::fromUtf8("⛔");
        default:
            return QString::fromUtf8("○");
    }
}
