/**
 * @file MayaDetectionDialog.h
 * @brief Maya 环境检测对话框
 */

#ifndef MAYADETECTIONDIALOG_H
#define MAYADETECTIONDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include "../../services/MayaDetector.h"
#include "../components/FluentButton.h"

/**
 * @brief Maya 环境检测对话框
 *
 * 详细显示系统中检测到的所有 Maya 信息：
 * - Maya 版本列表
 * - 安装路径
 * - 渲染器信息
 * - 插件列表
 * - 检测进度
 */
class MayaDetectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MayaDetectionDialog(QWidget *parent = nullptr);
    ~MayaDetectionDialog();

private slots:
    /**
     * @brief 开始检测按钮点击
     */
    void onStartDetection();

    /**
     * @brief 导出结果按钮点击
     */
    void onExportResults();

    /**
     * @brief 刷新按钮点击
     */
    void onRefreshClicked();

    /**
     * @brief 检测进度更新
     */
    void onDetectProgress(int progress, const QString &message);

    /**
     * @brief 检测完成
     */
    void onDetectFinished();

private:
    /**
     * @brief 初始化 UI
     */
    void initUI();

    /**
     * @brief 连接信号
     */
    void connectSignals();

    /**
     * @brief 显示检测结果
     */
    void displayResults(const QVector<MayaSoftwareInfo> &mayaVersions);

    /**
     * @brief 格式化 Maya 信息为文本
     * @param info Maya 信息
     * @param index 索引
     * @param fullPluginList 是否显示完整插件列表（用于导出）
     */
    QString formatMayaInfo(const MayaSoftwareInfo &info, int index, bool fullPluginList = false) const;

    /**
     * @brief 生成摘要信息
     */
    QString generateSummary(const QVector<MayaSoftwareInfo> &mayaVersions) const;

    /**
     * @brief 添加分割线
     */
    QString addSeparator(const QString &title = QString()) const;

    /**
     * @brief 生成完整的检测报告（用于导出）
     */
    QString generateFullReport(const QVector<MayaSoftwareInfo> &mayaVersions) const;

private:
    // 检测器
    MayaDetector *m_detector;

    // UI 组件
    QLabel *m_titleLabel;
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;
    QTextEdit *m_resultText;
    FluentButton *m_startButton;
    FluentButton *m_refreshButton;
    FluentButton *m_exportButton;
    FluentButton *m_closeButton;

    // 布局
    QVBoxLayout *m_mainLayout;

    // 检测结果
    QVector<MayaSoftwareInfo> m_detectedMayaVersions;

    // 状态
    bool m_isDetecting;
};

#endif // MAYADETECTIONDIALOG_H
