/**
 * @file FluentButton.h
 * @brief Fluent Design 风格按钮
 */

#ifndef FLUENTBUTTON_H
#define FLUENTBUTTON_H

#include <QPushButton>
#include <QPropertyAnimation>

/**
 * @brief Fluent Design 风格按钮
 *
 * 支持：
 * - Hover 动画效果
 * - 主要按钮和次要按钮样式
 * - 圆角和阴影
 */
class FluentButton : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(bool isPrimary READ isPrimary WRITE setIsPrimary)
    Q_PROPERTY(qreal hoverProgress READ hoverProgress WRITE setHoverProgress)

public:
    explicit FluentButton(QWidget *parent = nullptr);
    explicit FluentButton(const QString &text, QWidget *parent = nullptr);
    ~FluentButton();

    /**
     * @brief 是否为主要按钮
     */
    bool isPrimary() const { return m_isPrimary; }
    void setIsPrimary(bool primary);

    /**
     * @brief 设置图标
     */
    void setIcon(const QIcon &icon, const QSize &size = QSize(16, 16));

protected:
    /**
     * @brief 鼠标进入事件
     */
    void enterEvent(QEvent *event) override;

    /**
     * @brief 鼠标离开事件
     */
    void leaveEvent(QEvent *event) override;

    /**
     * @brief 绘制事件
     */
    void paintEvent(QPaintEvent *event) override;

private:
    qreal hoverProgress() const { return m_hoverProgress; }
    void setHoverProgress(qreal progress);

    void setupAnimation();
    void updateStyle();

private:
    bool m_isPrimary;
    qreal m_hoverProgress;
    QPropertyAnimation *m_hoverAnimation;
};

#endif // FLUENTBUTTON_H
