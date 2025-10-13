/**
 * @file FluentCard.h
 * @brief Fluent Design 风格卡片容器
 */

#ifndef FLUENTCARD_H
#define FLUENTCARD_H

#include <QWidget>
#include <QVBoxLayout>

/**
 * @brief Fluent Design 风格卡片容器
 *
 * 支持：
 * - 圆角背景
 * - 阴影效果
 * - Hover 高亮
 * - 可点击状态
 */
class FluentCard : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool isHoverable READ isHoverable WRITE setIsHoverable)
    Q_PROPERTY(bool isClickable READ isClickable WRITE setIsClickable)

public:
    explicit FluentCard(QWidget *parent = nullptr);
    ~FluentCard();

    /**
     * @brief 是否可 Hover
     */
    bool isHoverable() const { return m_isHoverable; }
    void setIsHoverable(bool hoverable);

    /**
     * @brief 是否可点击
     */
    bool isClickable() const { return m_isClickable; }
    void setIsClickable(bool clickable);

    /**
     * @brief 设置内边距
     */
    void setPadding(int padding);
    void setPadding(int left, int top, int right, int bottom);

    /**
     * @brief 设置圆角半径
     */
    void setBorderRadius(int radius);

    /**
     * @brief 获取内容布局
     */
    QVBoxLayout* contentLayout() const { return m_contentLayout; }

    /**
     * @brief 添加子控件
     */
    void addWidget(QWidget* widget);

signals:
    /**
     * @brief 卡片被点击
     */
    void clicked();

protected:
    /**
     * @brief 绘制事件
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief 鼠标进入事件
     */
    void enterEvent(QEvent *event) override;

    /**
     * @brief 鼠标离开事件
     */
    void leaveEvent(QEvent *event) override;

    /**
     * @brief 鼠标按下事件
     */
    void mousePressEvent(QMouseEvent *event) override;

    /**
     * @brief 鼠标释放事件
     */
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void updateStyle();

private:
    QVBoxLayout *m_contentLayout;
    bool m_isHoverable;
    bool m_isClickable;
    bool m_isHovered;
    bool m_isPressed;
    int m_borderRadius;
};

#endif // FLUENTCARD_H
