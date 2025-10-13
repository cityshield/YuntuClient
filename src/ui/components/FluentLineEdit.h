/**
 * @file FluentLineEdit.h
 * @brief Fluent Design 风格输入框
 */

#ifndef FLUENTLINEEDIT_H
#define FLUENTLINEEDIT_H

#include <QLineEdit>
#include <QPropertyAnimation>

/**
 * @brief Fluent Design 风格输入框
 *
 * 支持：
 * - Focus 时边框高亮动画
 * - 占位符文字动画
 * - 清除按钮
 * - 密码显示切换
 */
class FluentLineEdit : public QLineEdit
{
    Q_OBJECT
    Q_PROPERTY(qreal focusProgress READ focusProgress WRITE setFocusProgress)

public:
    explicit FluentLineEdit(QWidget *parent = nullptr);
    explicit FluentLineEdit(const QString &placeholder, QWidget *parent = nullptr);
    ~FluentLineEdit();

    /**
     * @brief 设置占位符文字
     */
    void setPlaceholder(const QString &text);

    /**
     * @brief 启用清除按钮
     */
    void setClearButtonEnabled(bool enable);

    /**
     * @brief 设置为密码输入框
     */
    void setPasswordMode(bool enabled);

protected:
    /**
     * @brief 焦点进入事件
     */
    void focusInEvent(QFocusEvent *event) override;

    /**
     * @brief 焦点离开事件
     */
    void focusOutEvent(QFocusEvent *event) override;

    /**
     * @brief 绘制事件
     */
    void paintEvent(QPaintEvent *event) override;

private:
    qreal focusProgress() const { return m_focusProgress; }
    void setFocusProgress(qreal progress);

    void setupAnimation();
    void updateStyle();

private:
    qreal m_focusProgress;
    QPropertyAnimation *m_focusAnimation;
    bool m_isPassword;
};

#endif // FLUENTLINEEDIT_H
