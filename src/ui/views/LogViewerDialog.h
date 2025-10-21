/**
 * @file LogViewerDialog.h
 * @brief 日志查看对话框
 */

#ifndef LOGVIEWERDIALOG_H
#define LOGVIEWERDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>

/**
 * @brief 日志查看对话框
 *
 * 提供日志文件查看、切换、刷新和导出功能
 */
class LogViewerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LogViewerDialog(QWidget *parent = nullptr);
    ~LogViewerDialog();

private slots:
    /**
     * @brief 日志文件选择变更
     */
    void onLogFileChanged(int index);

    /**
     * @brief 刷新日志内容
     */
    void onRefreshClicked();

    /**
     * @brief 打开日志文件夹
     */
    void onOpenFolderClicked();

    /**
     * @brief 复制日志内容到剪贴板
     */
    void onCopyClicked();

private:
    /**
     * @brief 初始化 UI
     */
    void initUI();

    /**
     * @brief 加载日志文件列表
     */
    void loadLogFiles();

    /**
     * @brief 加载并显示指定日志文件内容
     */
    void loadLogContent(const QString& filePath);

    QTextEdit* m_logTextEdit;
    QComboBox* m_logFileComboBox;
    QPushButton* m_refreshButton;
    QPushButton* m_openFolderButton;
    QPushButton* m_copyButton;
    QPushButton* m_closeButton;

    QStringList m_logFilePaths;
};

#endif // LOGVIEWERDIALOG_H
