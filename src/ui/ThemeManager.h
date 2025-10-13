/**
 * @file ThemeManager.h
 * @brief 主题管理器 - Fluent Design 风格
 */

#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QString>
#include <QColor>
#include <QWidget>

/**
 * @brief 主题类型枚举
 */
enum class ThemeType {
    Light,      // 亮色主题
    Dark        // 暗色主题
};

/**
 * @brief 主题管理器
 *
 * 管理应用程序的主题切换、样式表加载、组件效果应用等
 * 使用单例模式
 */
class ThemeManager : public QObject
{
    Q_OBJECT

public:
    static ThemeManager& instance();

    // 禁用拷贝构造和赋值
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    /**
     * @brief 初始化主题管理器
     */
    void initialize();

    /**
     * @brief 获取当前主题
     */
    ThemeType currentTheme() const { return m_currentTheme; }

    /**
     * @brief 设置主题
     * @param theme 主题类型
     */
    void setTheme(ThemeType theme);

    /**
     * @brief 切换主题（亮色<->暗色）
     */
    void toggleTheme();

    /**
     * @brief 应用主题到应用程序
     */
    void applyTheme();

    /**
     * @brief 为组件添加阴影效果
     * @param widget 目标组件
     * @param blurRadius 模糊半径（默认20）
     * @param offsetX X轴偏移（默认0）
     * @param offsetY Y轴偏移（默认4）
     */
    void applyShadowEffect(QWidget* widget, int blurRadius = 20, int offsetX = 0, int offsetY = 4);

    /**
     * @brief 为组件添加高斯模糊效果（毛玻璃）
     * @param widget 目标组件
     * @param blurRadius 模糊半径（默认10）
     */
    void applyBlurEffect(QWidget* widget, int blurRadius = 10);

    /**
     * @brief 为组件添加 Hover 高亮动画
     * @param widget 目标组件
     */
    void applyHoverAnimation(QWidget* widget);

    /**
     * @brief 获取主题颜色
     */
    QColor getPrimaryColor() const { return m_primaryColor; }
    QColor getAccentColor() const { return m_accentColor; }
    QColor getBackgroundColor() const { return m_backgroundColor; }
    QColor getSurfaceColor() const { return m_surfaceColor; }
    QColor getTextColor() const { return m_textColor; }
    QColor getSecondaryTextColor() const { return m_secondaryTextColor; }
    QColor getBorderColor() const { return m_borderColor; }
    QColor getHoverColor() const { return m_hoverColor; }

    /**
     * @brief 获取样式表字符串
     */
    QString getStyleSheet() const;

    /**
     * @brief 保存主题设置
     */
    void saveThemeSettings();

    /**
     * @brief 加载主题设置
     */
    void loadThemeSettings();

signals:
    /**
     * @brief 主题改变信号
     */
    void themeChanged(ThemeType theme);

private:
    explicit ThemeManager(QObject *parent = nullptr);
    ~ThemeManager();

    /**
     * @brief 加载样式表文件
     */
    QString loadStyleSheet(const QString& fileName) const;

    /**
     * @brief 获取内嵌样式表
     */
    QString getInlineStyleSheet() const;

    /**
     * @brief 更新主题颜色
     */
    void updateThemeColors();

    /**
     * @brief 替换样式表中的颜色变量
     */
    QString processStyleSheet(const QString& qss) const;

private:
    ThemeType m_currentTheme;

    // Fluent Design 主题颜色
    QColor m_primaryColor;          // 主色调
    QColor m_accentColor;           // 强调色
    QColor m_backgroundColor;       // 背景色
    QColor m_surfaceColor;          // 表面色（卡片、面板）
    QColor m_textColor;             // 主要文字颜色
    QColor m_secondaryTextColor;    // 次要文字颜色
    QColor m_borderColor;           // 边框颜色
    QColor m_hoverColor;            // 悬停颜色
    QColor m_shadowColor;           // 阴影颜色
};

#endif // THEMEMANAGER_H
