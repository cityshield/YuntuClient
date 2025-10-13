/**
 * @file FluentLineEdit.cpp
 * @brief Fluent Design 风格输入框实现
 */

#include "FluentLineEdit.h"
#include "../ThemeManager.h"
#include <QPainter>
#include <QPainterPath>
#include <QFocusEvent>

FluentLineEdit::FluentLineEdit(QWidget *parent)
    : QLineEdit(parent)
    , m_focusProgress(0.0)
    , m_focusAnimation(nullptr)
    , m_isPassword(false)
{
    setupAnimation();
    updateStyle();
}

FluentLineEdit::FluentLineEdit(const QString &placeholder, QWidget *parent)
    : QLineEdit(parent)
    , m_focusProgress(0.0)
    , m_focusAnimation(nullptr)
    , m_isPassword(false)
{
    setPlaceholderText(placeholder);
    setupAnimation();
    updateStyle();
}

FluentLineEdit::~FluentLineEdit()
{
}

void FluentLineEdit::setPlaceholder(const QString &text)
{
    setPlaceholderText(text);
}

void FluentLineEdit::setClearButtonEnabled(bool enable)
{
    QLineEdit::setClearButtonEnabled(enable);
}

void FluentLineEdit::setPasswordMode(bool enabled)
{
    m_isPassword = enabled;
    if (enabled) {
        setEchoMode(QLineEdit::Password);
    } else {
        setEchoMode(QLineEdit::Normal);
    }
}

void FluentLineEdit::focusInEvent(QFocusEvent *event)
{
    QLineEdit::focusInEvent(event);

    if (m_focusAnimation) {
        m_focusAnimation->setDirection(QAbstractAnimation::Forward);
        m_focusAnimation->start();
    }
}

void FluentLineEdit::focusOutEvent(QFocusEvent *event)
{
    QLineEdit::focusOutEvent(event);

    if (m_focusAnimation) {
        m_focusAnimation->setDirection(QAbstractAnimation::Backward);
        m_focusAnimation->start();
    }
}

void FluentLineEdit::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF rect = this->rect();
    qreal radius = 4.0;

    // 获取主题颜色
    ThemeManager &theme = ThemeManager::instance();
    QColor bgColor = theme.getSurfaceColor();
    QColor borderColor = theme.getBorderColor();

    // Focus 时边框颜色插值
    if (m_focusProgress > 0) {
        QColor focusColor = theme.getAccentColor();
        borderColor = QColor(
            borderColor.red() + (focusColor.red() - borderColor.red()) * m_focusProgress,
            borderColor.green() + (focusColor.green() - borderColor.green()) * m_focusProgress,
            borderColor.blue() + (focusColor.blue() - borderColor.blue()) * m_focusProgress
        );
    }

    // 绘制背景
    QPainterPath path;
    path.addRoundedRect(rect, radius, radius);
    painter.fillPath(path, bgColor);

    // 绘制边框
    qreal borderWidth = m_focusProgress > 0 ? 2.0 : 1.0;
    painter.setPen(QPen(borderColor, borderWidth));
    painter.drawPath(path);

    // 调用父类绘制文字
    QLineEdit::paintEvent(event);
}

void FluentLineEdit::setFocusProgress(qreal progress)
{
    if (m_focusProgress != progress) {
        m_focusProgress = progress;
        update();
    }
}

void FluentLineEdit::setupAnimation()
{
    m_focusAnimation = new QPropertyAnimation(this, "focusProgress", this);
    m_focusAnimation->setDuration(200);
    m_focusAnimation->setStartValue(0.0);
    m_focusAnimation->setEndValue(1.0);
    m_focusAnimation->setEasingCurve(QEasingCurve::OutCubic);
}

void FluentLineEdit::updateStyle()
{
    // 设置最小尺寸
    setMinimumHeight(36);

    // 设置内边距
    setTextMargins(12, 0, 12, 0);

    // 设置字体
    QFont font = this->font();
    font.setPointSize(10);
    setFont(font);
}
