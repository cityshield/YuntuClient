#include "ToastManager.h"
#include <QDebug>

ToastManager& ToastManager::instance()
{
    static ToastManager instance;
    return instance;
}

ToastManager::ToastManager()
    : QObject(nullptr)
    , m_mainWindow(nullptr)
    , m_toastWidget(nullptr)
{
}

void ToastManager::setMainWindow(QWidget* window)
{
    m_mainWindow = window;

    // 创建 ToastWidget 并设置父窗口
    if (m_mainWindow) {
        m_toastWidget = new ToastWidget(m_mainWindow);
        qDebug() << "ToastManager: 主窗口已设置";
    }
}

void ToastManager::showApiResponse(const QString& endpoint, int statusCode, bool success)
{
    if (!m_toastWidget) {
        qWarning() << "ToastManager: Toast widget 未初始化，请先调用 setMainWindow()";
        return;
    }

    m_toastWidget->showApiResponse(endpoint, statusCode, success);
    qDebug() << "ToastManager: 显示 API 响应 -" << endpoint << statusCode << (success ? "成功" : "失败");
}

void ToastManager::showToast(const QString& message, ToastWidget::ToastType type, int duration)
{
    if (!m_toastWidget) {
        qWarning() << "ToastManager: Toast widget 未初始化，请先调用 setMainWindow()";
        return;
    }

    m_toastWidget->show(message, type, duration);
    qDebug() << "ToastManager: 显示 Toast -" << message;
}
