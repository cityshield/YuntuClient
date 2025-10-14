/**
 * @file TitleBar.h
 * @brief 自定义标题栏组件
 */

#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMouseEvent>

/**
 * @brief 自定义标题栏
 *
 * 提供窗口拖动、最小化、最大化、关闭功能
 */
class TitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(QWidget *parent = nullptr);
    ~TitleBar();

    /**
     * @brief 设置标题文本
     */
    void setTitle(const QString &title);

    /**
     * @brief 设置标题栏高度
     */
    void setTitleBarHeight(int height);

    /**
     * @brief 设置是否显示最大化按钮
     */
    void setMaximizeButtonVisible(bool visible);

protected:
    /**
     * @brief 鼠标按下事件（用于窗口拖动）
     */
    void mousePressEvent(QMouseEvent *event) override;

    /**
     * @brief 鼠标移动事件（用于窗口拖动）
     */
    void mouseMoveEvent(QMouseEvent *event) override;

    /**
     * @brief 鼠标释放事件
     */
    void mouseReleaseEvent(QMouseEvent *event) override;

    /**
     * @brief 双击事件（最大化/还原）
     */
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    /**
     * @brief 最小化按钮点击
     */
    void onMinimizeClicked();

    /**
     * @brief 最大化/还原按钮点击
     */
    void onMaximizeClicked();

    /**
     * @brief 关闭按钮点击
     */
    void onCloseClicked();

private:
    /**
     * @brief 初始化 UI
     */
    void initUI();

    /**
     * @brief 更新最大化按钮图标
     */
    void updateMaximizeButton();

private:
    QLabel *m_titleLabel;            // 标题标签
    QPushButton *m_minimizeButton;   // 最小化按钮
    QPushButton *m_maximizeButton;   // 最大化按钮
    QPushButton *m_closeButton;      // 关闭按钮

    QPoint m_dragPosition;           // 拖动起始位置
    bool m_isDragging;               // 是否正在拖动
    bool m_isMaximized;              // 是否最大化状态
};

#endif // TITLEBAR_H
