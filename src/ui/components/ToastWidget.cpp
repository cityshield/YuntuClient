#include "ToastWidget.h"
#include <QPainter>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>

ToastWidget::ToastWidget(QWidget *parent)
    : QWidget(parent)
    , m_currentType(Info)
{
    setupUI();

    // 初始时隐藏
    hide();
}

void ToastWidget::setupUI()
{
    // 设置窗口属性
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    // 创建布局
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);

    // 创建消息标签
    m_messageLabel = new QLabel(this);
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setStyleSheet(
        "QLabel {"
        "    color: white;"
        "    font-size: 14px;"
        "    font-weight: 500;"
        "}"
    );
    layout->addWidget(m_messageLabel);

    // 创建阴影效果
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 80));
    shadow->setOffset(0, 4);
    setGraphicsEffect(shadow);

    // 创建隐藏定时器
    m_hideTimer = new QTimer(this);
    m_hideTimer->setSingleShot(true);
    connect(m_hideTimer, &QTimer::timeout, this, [this]() {
        // 淡出动画
        m_fadeAnimation = new QPropertyAnimation(this, "windowOpacity");
        m_fadeAnimation->setDuration(300);
        m_fadeAnimation->setStartValue(1.0);
        m_fadeAnimation->setEndValue(0.0);
        connect(m_fadeAnimation, &QPropertyAnimation::finished, this, &QWidget::hide);
        m_fadeAnimation->start(QAbstractAnimation::DeleteWhenStopped);
    });

    // 设置固定高度
    setFixedHeight(50);
}

void ToastWidget::show(const QString& message, ToastType type, int duration)
{
    m_currentType = type;
    m_messageLabel->setText(message);

    // 根据消息长度调整宽度
    QFontMetrics fm(m_messageLabel->font());
    int textWidth = fm.horizontalAdvance(message);
    int width = qMin(qMax(textWidth + 60, 200), 500);
    setFixedWidth(width);

    // 更新位置
    updatePosition();

    // 重置透明度
    setWindowOpacity(1.0);

    // 显示
    QWidget::show();
    raise();

    // 启动隐藏定时器
    m_hideTimer->start(duration);

    // 强制重绘以应用新颜色
    update();
}

void ToastWidget::showApiResponse(const QString& endpoint, int statusCode, bool success)
{
    QString message;
    ToastType type;

    if (success) {
        message = QString("%1 - %2").arg(endpoint).arg(statusCode);
        type = Success;
    } else {
        message = QString("%1 - %2 失败").arg(endpoint).arg(statusCode);
        type = Error;
    }

    show(message, type, 3000);
}

void ToastWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 获取颜色
    QString color = getColorForType(m_currentType);

    // 绘制圆角矩形背景
    painter.setBrush(QColor(color));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 8, 8);
}

void ToastWidget::updatePosition()
{
    if (!parentWidget()) {
        return;
    }

    // 定位到父窗口底部中央，距离底部 40px
    int x = (parentWidget()->width() - width()) / 2;
    int y = parentWidget()->height() - height() - 40;
    move(x, y);
}

QString ToastWidget::getColorForType(ToastType type) const
{
    switch (type) {
    case Success:
        return "#10B981";  // 绿色
    case Error:
        return "#EF4444";  // 红色
    case Info:
    default:
        return "#3B82F6";  // 蓝色
    }
}
