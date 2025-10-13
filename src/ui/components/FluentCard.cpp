/**
 * @file FluentCard.cpp
 * @brief Fluent Design 风格卡片容器实现
 */

#include "FluentCard.h"
#include "../ThemeManager.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>

FluentCard::FluentCard(QWidget *parent)
    : QWidget(parent)
    , m_contentLayout(nullptr)
    , m_isHoverable(false)
    , m_isClickable(false)
    , m_isHovered(false)
    , m_isPressed(false)
    , m_borderRadius(8)
{
    // 创建内容布局
    m_contentLayout = new QVBoxLayout(this);
    m_contentLayout->setContentsMargins(16, 16, 16, 16);
    m_contentLayout->setSpacing(12);

    updateStyle();
}

FluentCard::~FluentCard()
{
}

void FluentCard::setIsHoverable(bool hoverable)
{
    if (m_isHoverable != hoverable) {
        m_isHoverable = hoverable;
        updateStyle();
    }
}

void FluentCard::setIsClickable(bool clickable)
{
    if (m_isClickable != clickable) {
        m_isClickable = clickable;
        updateStyle();
    }
}

void FluentCard::setPadding(int padding)
{
    m_contentLayout->setContentsMargins(padding, padding, padding, padding);
}

void FluentCard::setPadding(int left, int top, int right, int bottom)
{
    m_contentLayout->setContentsMargins(left, top, right, bottom);
}

void FluentCard::setBorderRadius(int radius)
{
    if (m_borderRadius != radius) {
        m_borderRadius = radius;
        update();
    }
}

void FluentCard::addWidget(QWidget* widget)
{
    if (widget) {
        m_contentLayout->addWidget(widget);
    }
}

void FluentCard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF rect = this->rect();

    // 获取主题颜色
    ThemeManager &theme = ThemeManager::instance();
    QColor bgColor = theme.getSurfaceColor();
    QColor borderColor = theme.getBorderColor();

    // 根据状态调整颜色
    if (m_isPressed && m_isClickable) {
        bgColor = bgColor.darker(105);
    } else if (m_isHovered && m_isHoverable) {
        bgColor = theme.getHoverColor();
        borderColor = theme.getAccentColor();
    }

    // 绘制背景
    QPainterPath path;
    path.addRoundedRect(rect, m_borderRadius, m_borderRadius);
    painter.fillPath(path, bgColor);

    // 绘制边框
    painter.setPen(QPen(borderColor, 1));
    painter.drawPath(path);
}

void FluentCard::enterEvent(QEvent *event)
{
    QWidget::enterEvent(event);

    if (m_isHoverable) {
        m_isHovered = true;
        update();
    }
}

void FluentCard::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);

    if (m_isHoverable) {
        m_isHovered = false;
        m_isPressed = false;
        update();
    }
}

void FluentCard::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);

    if (m_isClickable && event->button() == Qt::LeftButton) {
        m_isPressed = true;
        update();
    }
}

void FluentCard::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);

    if (m_isClickable && event->button() == Qt::LeftButton) {
        if (m_isPressed && rect().contains(event->pos())) {
            emit clicked();
        }
        m_isPressed = false;
        update();
    }
}

void FluentCard::updateStyle()
{
    // 设置光标
    if (m_isClickable) {
        setCursor(Qt::PointingHandCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }

    // 应用阴影效果
    ThemeManager::instance().applyShadowEffect(this, 15, 0, 4);
}
