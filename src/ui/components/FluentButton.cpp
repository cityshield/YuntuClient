/**
 * @file FluentButton.cpp
 * @brief Fluent Design 风格按钮实现
 */

#include "FluentButton.h"
#include "../ThemeManager.h"
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>

FluentButton::FluentButton(QWidget *parent)
    : QPushButton(parent)
    , m_isPrimary(false)
    , m_hoverProgress(0.0)
    , m_hoverAnimation(nullptr)
{
    setupAnimation();
    updateStyle();
}

FluentButton::FluentButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent)
    , m_isPrimary(false)
    , m_hoverProgress(0.0)
    , m_hoverAnimation(nullptr)
{
    setupAnimation();
    updateStyle();
}

FluentButton::~FluentButton()
{
}

void FluentButton::setIsPrimary(bool primary)
{
    if (m_isPrimary != primary) {
        m_isPrimary = primary;
        updateStyle();
        update();
    }
}

void FluentButton::setIcon(const QIcon &icon, const QSize &size)
{
    QPushButton::setIcon(icon);
    setIconSize(size);
}

void FluentButton::enterEvent(QEvent *event)
{
    QPushButton::enterEvent(event);

    if (m_hoverAnimation) {
        m_hoverAnimation->setDirection(QAbstractAnimation::Forward);
        m_hoverAnimation->start();
    }
}

void FluentButton::leaveEvent(QEvent *event)
{
    QPushButton::leaveEvent(event);

    if (m_hoverAnimation) {
        m_hoverAnimation->setDirection(QAbstractAnimation::Backward);
        m_hoverAnimation->start();
    }
}

void FluentButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF rect = this->rect();
    qreal radius = 4.0;

    // 获取主题颜色
    ThemeManager &theme = ThemeManager::instance();
    QColor bgColor, textColor, borderColor;

    if (m_isPrimary) {
        bgColor = theme.getAccentColor();
        textColor = QColor(255, 255, 255);
        borderColor = theme.getAccentColor();
    } else {
        bgColor = theme.getSurfaceColor();
        textColor = theme.getTextColor();
        borderColor = theme.getBorderColor();
    }

    // 根据状态调整颜色
    if (!isEnabled()) {
        bgColor = theme.getSurfaceColor();
        textColor = theme.getSecondaryTextColor();
        borderColor = theme.getBorderColor();
    } else if (isDown()) {
        if (m_isPrimary) {
            bgColor = bgColor.darker(110);
        } else {
            bgColor = theme.getAccentColor();
            textColor = QColor(255, 255, 255);
        }
    } else if (m_hoverProgress > 0) {
        // Hover 效果插值
        if (m_isPrimary) {
            QColor hoverColor = theme.getPrimaryColor();
            bgColor = QColor(
                bgColor.red() + (hoverColor.red() - bgColor.red()) * m_hoverProgress,
                bgColor.green() + (hoverColor.green() - bgColor.green()) * m_hoverProgress,
                bgColor.blue() + (hoverColor.blue() - bgColor.blue()) * m_hoverProgress
            );
        } else {
            QColor hoverColor = theme.getHoverColor();
            bgColor = QColor(
                bgColor.red() + (hoverColor.red() - bgColor.red()) * m_hoverProgress,
                bgColor.green() + (hoverColor.green() - bgColor.green()) * m_hoverProgress,
                bgColor.blue() + (hoverColor.blue() - bgColor.blue()) * m_hoverProgress
            );
            if (m_hoverProgress > 0.5) {
                borderColor = theme.getAccentColor();
            }
        }
    }

    // 绘制背景
    QPainterPath path;
    path.addRoundedRect(rect, radius, radius);
    painter.fillPath(path, bgColor);

    // 绘制边框
    if (!m_isPrimary || !isEnabled()) {
        painter.setPen(QPen(borderColor, 1));
        painter.drawPath(path);
    }

    // 绘制文字
    painter.setPen(textColor);
    QFont font = this->font();
    painter.setFont(font);
    painter.drawText(rect, Qt::AlignCenter, text());
}

void FluentButton::setHoverProgress(qreal progress)
{
    if (m_hoverProgress != progress) {
        m_hoverProgress = progress;
        update();
    }
}

void FluentButton::setupAnimation()
{
    m_hoverAnimation = new QPropertyAnimation(this, "hoverProgress", this);
    m_hoverAnimation->setDuration(200);
    m_hoverAnimation->setStartValue(0.0);
    m_hoverAnimation->setEndValue(1.0);
    m_hoverAnimation->setEasingCurve(QEasingCurve::OutCubic);
}

void FluentButton::updateStyle()
{
    // 设置属性供 QSS 使用
    setProperty("primary", m_isPrimary);

    // 设置最小尺寸
    setMinimumHeight(32);
    setMinimumWidth(80);

    // 设置光标
    setCursor(Qt::PointingHandCursor);

    // 应用阴影效果
    if (m_isPrimary) {
        ThemeManager::instance().applyShadowEffect(this, 12, 0, 2);
    }
}
