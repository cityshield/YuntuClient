/**
 * @file FluentDialog.h
 * @brief Fluent Design 风格对话框
 */

#ifndef FLUENTDIALOG_H
#define FLUENTDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "FluentButton.h"

/**
 * @brief Fluent Design 风格对话框
 *
 * 支持：
 * - 半透明背景遮罩
 * - 圆角对话框
 * - 标题栏
 * - 内容区域
 * - 按钮区域（确定/取消）
 * - 淡入淡出动画
 */
class FluentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FluentDialog(QWidget *parent = nullptr);
    ~FluentDialog();

    /**
     * @brief 设置标题
     */
    void setTitle(const QString &title);

    /**
     * @brief 设置内容文本
     */
    void setContent(const QString &content);

    /**
     * @brief 设置内容控件
     */
    void setContentWidget(QWidget *widget);

    /**
     * @brief 设置按钮文本
     */
    void setButtonText(const QString &confirmText, const QString &cancelText = QString());

    /**
     * @brief 隐藏取消按钮
     */
    void hideCancelButton();

    /**
     * @brief 显示并居中
     */
    void showCentered();

    /**
     * @brief 获取内容布局
     */
    QVBoxLayout* contentLayout() const { return m_contentLayout; }

protected:
    /**
     * @brief 绘制事件（背景）
     */
    void paintEvent(QPaintEvent *event) override;

    /**
     * @brief 显示事件（动画）
     */
    void showEvent(QShowEvent *event) override;

private slots:
    void onConfirmClicked();
    void onCancelClicked();

private:
    void initUI();
    void setupAnimation();
    void createOverlay();

private:
    QWidget *m_overlay;              // 半透明遮罩
    QWidget *m_dialogPanel;          // 对话框面板

    QLabel *m_titleLabel;            // 标题标签
    QVBoxLayout *m_contentLayout;    // 内容布局
    QLabel *m_contentLabel;          // 内容文本标签

    FluentButton *m_confirmButton;   // 确定按钮
    FluentButton *m_cancelButton;    // 取消按钮
};

#endif // FLUENTDIALOG_H
