/**
 * @file TitleBar.cpp
 * @brief 自定义标题栏实现
 */

#include "TitleBar.h"
#include "../ThemeManager.h"
#include <QApplication>
#include <QStyle>

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
    , m_titleLabel(nullptr)
    , m_minimizeButton(nullptr)
    , m_maximizeButton(nullptr)
    , m_closeButton(nullptr)
    , m_isDragging(false)
    , m_isMaximized(false)
{
    initUI();
    setFixedHeight(40);
}

TitleBar::~TitleBar()
{
}

void TitleBar::setTitle(const QString &title)
{
    m_titleLabel->setText(title);
}

void TitleBar::setTitleBarHeight(int height)
{
    setFixedHeight(height);
}

void TitleBar::setMaximizeButtonVisible(bool visible)
{
    m_maximizeButton->setVisible(visible);
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_dragPosition = event->globalPos() - window()->frameGeometry().topLeft();
        event->accept();
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        // 如果窗口是最大化状态，先还原
        if (m_isMaximized) {
            onMaximizeClicked();
            // 重新计算拖动位置
            m_dragPosition = QPoint(window()->width() / 2, event->pos().y());
        }
        window()->move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}

void TitleBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        event->accept();
    }
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_maximizeButton->isVisible()) {
        onMaximizeClicked();
        event->accept();
    }
}

void TitleBar::onMinimizeClicked()
{
    window()->showMinimized();
}

void TitleBar::onMaximizeClicked()
{
    if (m_isMaximized) {
        window()->showNormal();
        m_isMaximized = false;
    } else {
        window()->showMaximized();
        m_isMaximized = true;
    }
    updateMaximizeButton();
}

void TitleBar::onCloseClicked()
{
    window()->close();
}

void TitleBar::initUI()
{
    // 创建主布局
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(0);

    // 标题标签
    m_titleLabel = new QLabel(this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(10);
    m_titleLabel->setFont(titleFont);

    // 最小化按钮
    m_minimizeButton = new QPushButton(this);
    m_minimizeButton->setText("—");
    m_minimizeButton->setFixedSize(46, 32);
    m_minimizeButton->setFlat(true);
    m_minimizeButton->setCursor(Qt::PointingHandCursor);
    connect(m_minimizeButton, &QPushButton::clicked,
            this, &TitleBar::onMinimizeClicked);

    // 最大化/还原按钮
    m_maximizeButton = new QPushButton(this);
    m_maximizeButton->setText("□");
    m_maximizeButton->setFixedSize(46, 32);
    m_maximizeButton->setFlat(true);
    m_maximizeButton->setCursor(Qt::PointingHandCursor);
    connect(m_maximizeButton, &QPushButton::clicked,
            this, &TitleBar::onMaximizeClicked);

    // 关闭按钮
    m_closeButton = new QPushButton(this);
    m_closeButton->setText("✕");
    m_closeButton->setFixedSize(46, 32);
    m_closeButton->setFlat(true);
    m_closeButton->setCursor(Qt::PointingHandCursor);
    connect(m_closeButton, &QPushButton::clicked,
            this, &TitleBar::onCloseClicked);

    // 添加到布局
    layout->addWidget(m_titleLabel);
    layout->addStretch();
    layout->addWidget(m_minimizeButton);
    layout->addWidget(m_maximizeButton);
    layout->addWidget(m_closeButton);

    // 设置样式
    ThemeManager &theme = ThemeManager::instance();
    QString bgColor = theme.getSurfaceColor().name();
    QString textColor = theme.getTextColor().name();
    QString hoverBgColor = theme.getHoverColor().name();

    setStyleSheet(QString(
        "TitleBar {"
        "    background-color: %1;"
        "}"
        "QLabel {"
        "    color: %2;"
        "}"
        "QPushButton {"
        "    background-color: transparent;"
        "    color: %2;"
        "    border: none;"
        "    font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "    background-color: %3;"
        "}"
        "QPushButton#closeButton:hover {"
        "    background-color: #E81123;"
        "    color: white;"
        "}"
    ).arg(bgColor, textColor, hoverBgColor));

    m_closeButton->setObjectName("closeButton");

    // 监听主题变化
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [this](ThemeType) {
                ThemeManager &theme = ThemeManager::instance();
                QString bgColor = theme.getSurfaceColor().name();
                QString textColor = theme.getTextColor().name();
                QString hoverBgColor = theme.getHoverColor().name();

                setStyleSheet(QString(
                    "TitleBar {"
                    "    background-color: %1;"
                    "}"
                    "QLabel {"
                    "    color: %2;"
                    "}"
                    "QPushButton {"
                    "    background-color: transparent;"
                    "    color: %2;"
                    "    border: none;"
                    "    font-size: 14px;"
                    "}"
                    "QPushButton:hover {"
                    "    background-color: %3;"
                    "}"
                    "QPushButton#closeButton:hover {"
                    "    background-color: #E81123;"
                    "    color: white;"
                    "}"
                ).arg(bgColor, textColor, hoverBgColor));
            });
}

void TitleBar::updateMaximizeButton()
{
    if (m_isMaximized) {
        m_maximizeButton->setText("❐");
    } else {
        m_maximizeButton->setText("□");
    }
}
