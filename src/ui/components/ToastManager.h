#ifndef TOASTMANAGER_H
#define TOASTMANAGER_H

#include <QObject>
#include <QWidget>
#include "ToastWidget.h"

/**
 * @brief Toast 全局管理器
 *
 * 单例模式，管理所有窗口的 Toast 显示
 */
class ToastManager : public QObject
{
    Q_OBJECT

public:
    static ToastManager& instance();

    /**
     * @brief 设置主窗口（Toast 将显示在此窗口上）
     */
    void setMainWindow(QWidget* window);

    /**
     * @brief 显示 API 响应 Toast
     * @param endpoint API 端点
     * @param statusCode HTTP 状态码
     * @param success 是否成功
     */
    void showApiResponse(const QString& endpoint, int statusCode, bool success);

    /**
     * @brief 显示普通 Toast
     * @param message 消息内容
     * @param type 消息类型
     * @param duration 显示时长（毫秒）
     */
    void showToast(const QString& message,
                   ToastWidget::ToastType type = ToastWidget::Info,
                   int duration = 3000);

private:
    ToastManager();
    ~ToastManager() = default;
    ToastManager(const ToastManager&) = delete;
    ToastManager& operator=(const ToastManager&) = delete;

    QWidget* m_mainWindow;
    ToastWidget* m_toastWidget;
};

#endif // TOASTMANAGER_H
