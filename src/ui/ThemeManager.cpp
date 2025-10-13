/**
 * @file ThemeManager.cpp
 * @brief 主题管理器实现
 */

#include "ThemeManager.h"
#include "../core/Logger.h"
#include "../core/Application.h"
#include "../core/Config.h"
#include <QApplication>
#include <QFile>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsBlurEffect>
#include <QPropertyAnimation>
#include <QSettings>

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
    , m_currentTheme(ThemeType::Light)
{
}

ThemeManager::~ThemeManager()
{
}

ThemeManager& ThemeManager::instance()
{
    static ThemeManager instance;
    return instance;
}

void ThemeManager::initialize()
{
    Application::instance().logger()->info("ThemeManager", QString::fromUtf8("初始化主题管理器"));

    // 加载保存的主题设置
    loadThemeSettings();

    // 应用主题
    applyTheme();
}

void ThemeManager::setTheme(ThemeType theme)
{
    if (m_currentTheme != theme) {
        m_currentTheme = theme;
        updateThemeColors();
        applyTheme();
        saveThemeSettings();

        Application::instance().logger()->info("ThemeManager",
            QString::fromUtf8("主题切换: %1").arg(theme == ThemeType::Dark ? "暗色" : "亮色"));

        emit themeChanged(theme);
    }
}

void ThemeManager::toggleTheme()
{
    setTheme(m_currentTheme == ThemeType::Light ? ThemeType::Dark : ThemeType::Light);
}

void ThemeManager::applyTheme()
{
    updateThemeColors();

    // 获取并应用样式表
    QString styleSheet = getStyleSheet();
    qApp->setStyleSheet(styleSheet);

    Application::instance().logger()->debug("ThemeManager", QString::fromUtf8("主题已应用"));
}

void ThemeManager::applyShadowEffect(QWidget* widget, int blurRadius, int offsetX, int offsetY)
{
    if (!widget) return;

    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setBlurRadius(blurRadius);
    shadow->setXOffset(offsetX);
    shadow->setYOffset(offsetY);
    shadow->setColor(m_shadowColor);
    widget->setGraphicsEffect(shadow);
}

void ThemeManager::applyBlurEffect(QWidget* widget, int blurRadius)
{
    if (!widget) return;

    QGraphicsBlurEffect* blur = new QGraphicsBlurEffect(widget);
    blur->setBlurRadius(blurRadius);
    widget->setGraphicsEffect(blur);
}

void ThemeManager::applyHoverAnimation(QWidget* widget)
{
    if (!widget) return;

    // 这里需要配合自定义控件的 enterEvent 和 leaveEvent 实现
    // 暂时预留接口
}

QString ThemeManager::getStyleSheet() const
{
    // 加载基础样式表
    QString qss = loadStyleSheet(m_currentTheme == ThemeType::Dark ?
        ":/styles/fluent_dark.qss" : ":/styles/fluent_light.qss");

    // 如果加载失败，使用内嵌样式
    if (qss.isEmpty()) {
        qss = getInlineStyleSheet();
    }

    // 处理颜色变量替换
    return processStyleSheet(qss);
}

void ThemeManager::saveThemeSettings()
{
    QSettings settings;
    settings.beginGroup("Theme");
    settings.setValue("current_theme", static_cast<int>(m_currentTheme));
    settings.endGroup();
}

void ThemeManager::loadThemeSettings()
{
    QSettings settings;
    settings.beginGroup("Theme");
    int themeValue = settings.value("current_theme", static_cast<int>(ThemeType::Light)).toInt();
    m_currentTheme = static_cast<ThemeType>(themeValue);
    settings.endGroup();

    updateThemeColors();
}

QString ThemeManager::loadStyleSheet(const QString& fileName)
{
    QFile file(fileName);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString content = QString::fromUtf8(file.readAll());
        file.close();
        return content;
    }

    Application::instance().logger()->warning("ThemeManager",
        QString::fromUtf8("无法加载样式表文件: %1，使用内嵌样式").arg(fileName));

    // 返回内嵌的基础样式
    return "";  // 将在 getStyleSheet 中使用内嵌样式
}

QString ThemeManager::getInlineStyleSheet() const
{
    // 内嵌的 Fluent Design 样式表
    QString qss = R"(
/* ===== 全局样式 ===== */
* {
    font-family: "Segoe UI", "Microsoft YaHei UI", "微软雅黑", sans-serif;
    font-size: 14px;
}

QWidget {
    background-color: @backgroundColor;
    color: @textColor;
}

/* ===== 按钮样式 ===== */
QPushButton {
    background-color: @surfaceColor;
    color: @textColor;
    border: 1px solid @borderColor;
    border-radius: 4px;
    padding: 8px 16px;
    min-height: 32px;
}

QPushButton:hover {
    background-color: @hoverColor;
    border-color: @accentColor;
}

QPushButton:pressed {
    background-color: @accentColor;
    color: white;
}

QPushButton:disabled {
    background-color: @surfaceColor;
    color: @secondaryTextColor;
    border-color: @borderColor;
}

/* 主要按钮 */
QPushButton[primary="true"] {
    background-color: @accentColor;
    color: white;
    border: none;
}

QPushButton[primary="true"]:hover {
    background-color: @primaryColor;
}

/* ===== 输入框样式 ===== */
QLineEdit {
    background-color: @surfaceColor;
    color: @textColor;
    border: 1px solid @borderColor;
    border-radius: 4px;
    padding: 8px 12px;
    min-height: 32px;
}

QLineEdit:hover {
    border-color: @accentColor;
}

QLineEdit:focus {
    border: 2px solid @accentColor;
    border-radius: 4px;
}

/* ===== 文本框样式 ===== */
QTextEdit {
    background-color: @surfaceColor;
    color: @textColor;
    border: 1px solid @borderColor;
    border-radius: 4px;
    padding: 8px;
}

QTextEdit:focus {
    border: 2px solid @accentColor;
}

/* ===== 下拉框样式 ===== */
QComboBox {
    background-color: @surfaceColor;
    color: @textColor;
    border: 1px solid @borderColor;
    border-radius: 4px;
    padding: 6px 12px;
    min-height: 32px;
}

QComboBox:hover {
    border-color: @accentColor;
}

QComboBox::drop-down {
    border: none;
    width: 30px;
}

QComboBox::down-arrow {
    image: url(:/icons/chevron-down.png);
    width: 12px;
    height: 12px;
}

/* ===== 列表样式 ===== */
QListView {
    background-color: @surfaceColor;
    color: @textColor;
    border: 1px solid @borderColor;
    border-radius: 8px;
    padding: 4px;
}

QListView::item {
    border-radius: 4px;
    padding: 8px;
    margin: 2px 0;
}

QListView::item:hover {
    background-color: @hoverColor;
}

QListView::item:selected {
    background-color: @accentColor;
    color: white;
}

/* ===== 滚动条样式 ===== */
QScrollBar:vertical {
    background-color: transparent;
    width: 12px;
    margin: 0;
}

QScrollBar::handle:vertical {
    background-color: @borderColor;
    border-radius: 6px;
    min-height: 30px;
}

QScrollBar::handle:vertical:hover {
    background-color: @secondaryTextColor;
}

QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical {
    height: 0;
}

QScrollBar:horizontal {
    background-color: transparent;
    height: 12px;
    margin: 0;
}

QScrollBar::handle:horizontal {
    background-color: @borderColor;
    border-radius: 6px;
    min-width: 30px;
}

QScrollBar::handle:horizontal:hover {
    background-color: @secondaryTextColor;
}

/* ===== 标签页样式 ===== */
QTabWidget::pane {
    border: 1px solid @borderColor;
    border-radius: 8px;
    background-color: @surfaceColor;
}

QTabBar::tab {
    background-color: transparent;
    color: @secondaryTextColor;
    padding: 8px 16px;
    border-radius: 4px 4px 0 0;
    margin-right: 2px;
}

QTabBar::tab:hover {
    background-color: @hoverColor;
}

QTabBar::tab:selected {
    background-color: @surfaceColor;
    color: @textColor;
    border-bottom: 2px solid @accentColor;
}

/* ===== 复选框样式 ===== */
QCheckBox {
    color: @textColor;
    spacing: 8px;
}

QCheckBox::indicator {
    width: 18px;
    height: 18px;
    border: 2px solid @borderColor;
    border-radius: 4px;
    background-color: @surfaceColor;
}

QCheckBox::indicator:hover {
    border-color: @accentColor;
}

QCheckBox::indicator:checked {
    background-color: @accentColor;
    border-color: @accentColor;
    image: url(:/icons/check.png);
}

/* ===== 单选框样式 ===== */
QRadioButton {
    color: @textColor;
    spacing: 8px;
}

QRadioButton::indicator {
    width: 18px;
    height: 18px;
    border: 2px solid @borderColor;
    border-radius: 9px;
    background-color: @surfaceColor;
}

QRadioButton::indicator:hover {
    border-color: @accentColor;
}

QRadioButton::indicator:checked {
    border-color: @accentColor;
    background-color: @accentColor;
}

/* ===== 进度条样式 ===== */
QProgressBar {
    background-color: @surfaceColor;
    border: 1px solid @borderColor;
    border-radius: 4px;
    text-align: center;
    color: @textColor;
    height: 20px;
}

QProgressBar::chunk {
    background-color: @accentColor;
    border-radius: 3px;
}

/* ===== 菜单样式 ===== */
QMenuBar {
    background-color: @backgroundColor;
    color: @textColor;
    border-bottom: 1px solid @borderColor;
}

QMenuBar::item {
    padding: 6px 12px;
    background-color: transparent;
}

QMenuBar::item:selected {
    background-color: @hoverColor;
}

QMenu {
    background-color: @surfaceColor;
    color: @textColor;
    border: 1px solid @borderColor;
    border-radius: 8px;
    padding: 4px;
}

QMenu::item {
    padding: 8px 24px;
    border-radius: 4px;
}

QMenu::item:selected {
    background-color: @hoverColor;
}

/* ===== 工具提示样式 ===== */
QToolTip {
    background-color: @surfaceColor;
    color: @textColor;
    border: 1px solid @borderColor;
    border-radius: 4px;
    padding: 6px 10px;
}
)";

    return qss;
}

void ThemeManager::updateThemeColors()
{
    if (m_currentTheme == ThemeType::Dark) {
        // 暗色主题 - Fluent Design Dark
        m_primaryColor = QColor(0x00, 0x78, 0xD4);        // #0078D4 蓝色
        m_accentColor = QColor(0x00, 0x78, 0xD4);         // #0078D4 强调蓝
        m_backgroundColor = QColor(0x1F, 0x1F, 0x1F);     // #1F1F1F 深灰
        m_surfaceColor = QColor(0x2D, 0x2D, 0x2D);        // #2D2D2D 中灰
        m_textColor = QColor(0xFF, 0xFF, 0xFF);           // #FFFFFF 白色
        m_secondaryTextColor = QColor(0xA0, 0xA0, 0xA0);  // #A0A0A0 灰色
        m_borderColor = QColor(0x3D, 0x3D, 0x3D);         // #3D3D3D 边框灰
        m_hoverColor = QColor(0x3D, 0x3D, 0x3D);          // #3D3D3D 悬停灰
        m_shadowColor = QColor(0x00, 0x00, 0x00, 60);     // 黑色半透明
    } else {
        // 亮色主题 - Fluent Design Light
        m_primaryColor = QColor(0x00, 0x5A, 0x9E);        // #005A9E 深蓝
        m_accentColor = QColor(0x00, 0x78, 0xD4);         // #0078D4 强调蓝
        m_backgroundColor = QColor(0xF3, 0xF3, 0xF3);     // #F3F3F3 浅灰
        m_surfaceColor = QColor(0xFF, 0xFF, 0xFF);        // #FFFFFF 白色
        m_textColor = QColor(0x00, 0x00, 0x00);           // #000000 黑色
        m_secondaryTextColor = QColor(0x60, 0x60, 0x60);  // #606060 深灰
        m_borderColor = QColor(0xE0, 0xE0, 0xE0);         // #E0E0E0 边框灰
        m_hoverColor = QColor(0xF0, 0xF0, 0xF0);          // #F0F0F0 悬停灰
        m_shadowColor = QColor(0x00, 0x00, 0x00, 30);     // 黑色半透明
    }
}

QString ThemeManager::processStyleSheet(const QString& qss)
{
    QString processed = qss;

    // 替换颜色变量
    processed.replace("@primaryColor", m_primaryColor.name());
    processed.replace("@accentColor", m_accentColor.name());
    processed.replace("@backgroundColor", m_backgroundColor.name());
    processed.replace("@surfaceColor", m_surfaceColor.name());
    processed.replace("@textColor", m_textColor.name());
    processed.replace("@secondaryTextColor", m_secondaryTextColor.name());
    processed.replace("@borderColor", m_borderColor.name());
    processed.replace("@hoverColor", m_hoverColor.name());

    return processed;
}
