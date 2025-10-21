/**
 * @file LogViewerDialog.cpp
 * @brief 日志查看对话框实现
 */

#include "LogViewerDialog.h"
#include "../components/FluentButton.h"
#include "../../core/Application.h"
#include "../../core/Logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QClipboard>
#include <QApplication>
#include <QMessageBox>

LogViewerDialog::LogViewerDialog(QWidget *parent)
    : QDialog(parent)
    , m_logTextEdit(nullptr)
    , m_logFileComboBox(nullptr)
    , m_refreshButton(nullptr)
    , m_openFolderButton(nullptr)
    , m_copyButton(nullptr)
    , m_closeButton(nullptr)
{
    initUI();
    loadLogFiles();

    // 设置窗口属性
    setWindowTitle(QString::fromUtf8("查看日志"));
    resize(900, 600);
}

LogViewerDialog::~LogViewerDialog()
{
}

void LogViewerDialog::initUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    // 顶部工具栏
    QHBoxLayout* toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(8);

    QLabel* selectLabel = new QLabel(QString::fromUtf8("选择日志文件:"), this);
    m_logFileComboBox = new QComboBox(this);
    m_logFileComboBox->setMinimumWidth(250);

    m_refreshButton = new QPushButton(QString::fromUtf8("🔄 刷新"), this);
    m_openFolderButton = new QPushButton(QString::fromUtf8("📂 打开文件夹"), this);
    m_copyButton = new QPushButton(QString::fromUtf8("📋 复制"), this);

    toolbarLayout->addWidget(selectLabel);
    toolbarLayout->addWidget(m_logFileComboBox);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(m_refreshButton);
    toolbarLayout->addWidget(m_openFolderButton);
    toolbarLayout->addWidget(m_copyButton);

    // 日志内容显示区域
    m_logTextEdit = new QTextEdit(this);
    m_logTextEdit->setReadOnly(true);
    m_logTextEdit->setLineWrapMode(QTextEdit::NoWrap);
    m_logTextEdit->setFontFamily("Courier New");
    m_logTextEdit->setStyleSheet(
        "QTextEdit {"
        "    background-color: #1E1E1E;"
        "    color: #D4D4D4;"
        "    border: 1px solid #3E3E3E;"
        "    border-radius: 4px;"
        "    padding: 8px;"
        "    font-size: 12px;"
        "}"
    );

    // 底部按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_closeButton = new QPushButton(QString::fromUtf8("关闭"), this);
    m_closeButton->setMinimumWidth(100);
    buttonLayout->addWidget(m_closeButton);

    // 添加到主布局
    mainLayout->addLayout(toolbarLayout);
    mainLayout->addWidget(m_logTextEdit, 1);
    mainLayout->addLayout(buttonLayout);

    // 连接信号
    connect(m_logFileComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LogViewerDialog::onLogFileChanged);
    connect(m_refreshButton, &QPushButton::clicked,
            this, &LogViewerDialog::onRefreshClicked);
    connect(m_openFolderButton, &QPushButton::clicked,
            this, &LogViewerDialog::onOpenFolderClicked);
    connect(m_copyButton, &QPushButton::clicked,
            this, &LogViewerDialog::onCopyClicked);
    connect(m_closeButton, &QPushButton::clicked,
            this, &QDialog::accept);
}

void LogViewerDialog::loadLogFiles()
{
    m_logFileComboBox->clear();
    m_logFilePaths.clear();

    Logger* logger = Application::instance().logger();
    if (!logger) {
        return;
    }

    // 获取所有日志文件（按时间倒序）
    QStringList logFiles = logger->getAllLogFiles();

    if (logFiles.isEmpty()) {
        m_logFileComboBox->addItem(QString::fromUtf8("暂无日志文件"));
        m_logTextEdit->setPlainText(QString::fromUtf8("暂无日志文件"));
        return;
    }

    // 添加到下拉列表
    for (const QString& filePath : logFiles) {
        QFileInfo fileInfo(filePath);
        QString displayName = fileInfo.fileName();

        // 添加文件大小信息
        qint64 fileSize = fileInfo.size();
        QString sizeStr;
        if (fileSize < 1024) {
            sizeStr = QString::number(fileSize) + " B";
        } else if (fileSize < 1024 * 1024) {
            sizeStr = QString::number(fileSize / 1024.0, 'f', 1) + " KB";
        } else {
            sizeStr = QString::number(fileSize / (1024.0 * 1024.0), 'f', 1) + " MB";
        }

        displayName += QString(" (%1)").arg(sizeStr);

        m_logFileComboBox->addItem(displayName);
        m_logFilePaths.append(filePath);
    }

    // 默认加载第一个（最新的）日志文件
    if (!m_logFilePaths.isEmpty()) {
        loadLogContent(m_logFilePaths.first());
    }
}

void LogViewerDialog::loadLogContent(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_logTextEdit->setPlainText(QString::fromUtf8("无法打开日志文件: %1").arg(filePath));
        return;
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    // 显示日志内容
    m_logTextEdit->setPlainText(content);

    // 滚动到底部（最新的日志）
    QTextCursor cursor = m_logTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logTextEdit->setTextCursor(cursor);
}

void LogViewerDialog::onLogFileChanged(int index)
{
    if (index >= 0 && index < m_logFilePaths.size()) {
        loadLogContent(m_logFilePaths[index]);
    }
}

void LogViewerDialog::onRefreshClicked()
{
    int currentIndex = m_logFileComboBox->currentIndex();
    loadLogFiles();

    // 尝试恢复之前选择的文件
    if (currentIndex >= 0 && currentIndex < m_logFileComboBox->count()) {
        m_logFileComboBox->setCurrentIndex(currentIndex);
    }
}

void LogViewerDialog::onOpenFolderClicked()
{
    Logger* logger = Application::instance().logger();
    if (!logger) {
        return;
    }

    QString logFilePath = logger->currentLogFilePath();
    if (logFilePath.isEmpty()) {
        return;
    }

    QFileInfo fileInfo(logFilePath);
    QString logDir = fileInfo.absolutePath();

    // 打开日志文件夹
    QDesktopServices::openUrl(QUrl::fromLocalFile(logDir));
}

void LogViewerDialog::onCopyClicked()
{
    QString content = m_logTextEdit->toPlainText();
    if (content.isEmpty()) {
        return;
    }

    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(content);

    QMessageBox::information(this,
        QString::fromUtf8("复制成功"),
        QString::fromUtf8("日志内容已复制到剪贴板"));
}
