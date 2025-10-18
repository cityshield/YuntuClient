#ifndef TOASTWIDGET_H
#define TOASTWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QPropertyAnimation>

/**
 * @brief Toast 通知组件
 *
 * 在窗口底部显示临时通知消息，支持成功/失败/信息三种类型
 */
class ToastWidget : public QWidget
{
    Q_OBJECT

public:
    enum ToastType {
        Success,  // 成功 - 绿色
        Error,    // 错误 - 红色
        Info      // 信息 - 蓝色
    };

    explicit ToastWidget(QWidget *parent = nullptr);

    /**
     * @brief 显示 Toast 消息
     * @param message 消息内容
     * @param type 消息类型
     * @param duration 显示时长（毫秒），默认 3000ms
     */
    void show(const QString& message, ToastType type = Info, int duration = 3000);

    /**
     * @brief 显示 API 响应的 Toast
     * @param endpoint API 端点（如 "/auth/login"）
     * @param statusCode HTTP 状态码
     * @param success 是否成功
     */
    void showApiResponse(const QString& endpoint, int statusCode, bool success);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void setupUI();
    void updatePosition();
    QString getColorForType(ToastType type) const;

    QLabel *m_messageLabel;
    QTimer *m_hideTimer;
    QPropertyAnimation *m_fadeAnimation;
    ToastType m_currentType;
};

#endif // TOASTWIDGET_H
