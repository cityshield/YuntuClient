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

    // 使用 QPalette 强制设置文字颜色（比样式表更可靠）
    QPalette palette = m_messageLabel->palette();
    palette.setColor(QPalette::WindowText, QColor(255, 255, 255));  // 白色文字
    palette.setColor(QPalette::Text, QColor(255, 255, 255));
    m_messageLabel->setPalette(palette);
    m_messageLabel->setAutoFillBackground(false);  // 不自动填充背景

    // 设置字体
    QFont font = m_messageLabel->font();
    font.setPixelSize(14);
    m_messageLabel->setFont(font);

    layout->addWidget(m_messageLabel);

    // 不使用阴影效果，简洁样式

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
    setFixedHeight(48);
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

    // 黑底白字，无圆角无边框
    painter.setBrush(QColor(0, 0, 0, 220));  // 半透明黑色背景
    painter.setPen(Qt::NoPen);
    painter.drawRect(rect());
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
    // 统一使用黑色背景，不区分类型
    Q_UNUSED(type);
    return "#000000";
}
