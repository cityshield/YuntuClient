/**
 * @file FluentDialog.cpp
 * @brief Fluent Design 风格对话框实现
 */

#include "FluentDialog.h"
#include "../ThemeManager.h"
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QApplication>
#include <QScreen>

FluentDialog::FluentDialog(QWidget *parent)
    : QDialog(parent)
    , m_overlay(nullptr)
    , m_dialogPanel(nullptr)
    , m_titleLabel(nullptr)
    , m_contentLayout(nullptr)
    , m_contentLabel(nullptr)
    , m_confirmButton(nullptr)
    , m_cancelButton(nullptr)
{
    // 设置对话框属性
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);

    initUI();
    setupAnimation();
}

FluentDialog::~FluentDialog()
{
}

void FluentDialog::setTitle(const QString &title)
{
    m_titleLabel->setText(title);
}

void FluentDialog::setContent(const QString &content)
{
    m_contentLabel->setText(content);
    m_contentLabel->show();
}

void FluentDialog::setContentWidget(QWidget *widget)
{
    if (widget) {
        // 隐藏默认内容标签
        m_contentLabel->hide();

        // 添加自定义控件
        m_contentLayout->addWidget(widget);
    }
}

void FluentDialog::setButtonText(const QString &confirmText, const QString &cancelText)
{
    m_confirmButton->setText(confirmText);

    if (!cancelText.isEmpty()) {
        m_cancelButton->setText(cancelText);
        m_cancelButton->show();
    }
}

void FluentDialog::hideCancelButton()
{
    m_cancelButton->hide();
}

void FluentDialog::showCentered()
{
    // 居中显示
    if (parentWidget()) {
        QPoint parentCenter = parentWidget()->geometry().center();
        move(parentCenter - rect().center());
    } else {
        QScreen *screen = QApplication::primaryScreen();
        QPoint screenCenter = screen->geometry().center();
        move(screenCenter - rect().center());
    }

    show();
}

void FluentDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制半透明遮罩背景
    painter.fillRect(rect(), QColor(0, 0, 0, 100));
}

void FluentDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);

    // 淡入动画
    QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(m_dialogPanel);
    m_dialogPanel->setGraphicsEffect(effect);

    QPropertyAnimation *animation = new QPropertyAnimation(effect, "opacity");
    animation->setDuration(200);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void FluentDialog::onConfirmClicked()
{
    accept();
}

void FluentDialog::onCancelClicked()
{
    reject();
}

void FluentDialog::initUI()
{
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 对话框面板
    m_dialogPanel = new QWidget(this);
    m_dialogPanel->setFixedSize(480, 320);

    // 面板布局
    QVBoxLayout *panelLayout = new QVBoxLayout(m_dialogPanel);
    panelLayout->setContentsMargins(30, 30, 30, 30);
    panelLayout->setSpacing(20);

    // 标题
    m_titleLabel = new QLabel(QString::fromUtf8("提示"), m_dialogPanel);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);

    // 内容布局
    m_contentLayout = new QVBoxLayout();
    m_contentLayout->setSpacing(12);

    // 默认内容标签
    m_contentLabel = new QLabel(m_dialogPanel);
    m_contentLabel->setWordWrap(true);
    m_contentLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_contentLayout->addWidget(m_contentLabel);

    // 按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(12);

    m_cancelButton = new FluentButton(QString::fromUtf8("取消"), m_dialogPanel);
    m_cancelButton->setMinimumWidth(100);

    m_confirmButton = new FluentButton(QString::fromUtf8("确定"), m_dialogPanel);
    m_confirmButton->setIsPrimary(true);
    m_confirmButton->setMinimumWidth(100);

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_confirmButton);

    // 添加到面板布局
    panelLayout->addWidget(m_titleLabel);
    panelLayout->addLayout(m_contentLayout);
    panelLayout->addStretch();
    panelLayout->addLayout(buttonLayout);

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

    // 连接信号
    connect(m_confirmButton, &FluentButton::clicked, this, &FluentDialog::onConfirmClicked);
    connect(m_cancelButton, &FluentButton::clicked, this, &FluentDialog::onCancelClicked);

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

void FluentDialog::setupAnimation()
{
    // 动画在 showEvent 中设置
}

void FluentDialog::createOverlay()
{
    // 遮罩在 paintEvent 中绘制
}
