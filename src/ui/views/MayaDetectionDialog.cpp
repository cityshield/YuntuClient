/**
 * @file MayaDetectionDialog.cpp
 * @brief Maya 环境检测对话框实现
 */

#include "MayaDetectionDialog.h"
#include "../ThemeManager.h"
#include "../../core/Application.h"
#include "../../core/Logger.h"
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDateTime>
#include <QFile>
#include <QDir>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QScrollBar>
#include <QApplication>
#include <QTimer>
#include <QSet>
#include <QStandardPaths>

MayaDetectionDialog::MayaDetectionDialog(QWidget *parent)
    : QDialog(parent)
    , m_detector(nullptr)
    , m_titleLabel(nullptr)
    , m_statusLabel(nullptr)
    , m_progressBar(nullptr)
    , m_resultText(nullptr)
    , m_startButton(nullptr)
    , m_refreshButton(nullptr)
    , m_exportButton(nullptr)
    , m_closeButton(nullptr)
    , m_mainLayout(nullptr)
    , m_isDetecting(false)
{
    initUI();
    connectSignals();

    // 创建检测器
    m_detector = new MayaDetector(this);

    // 设置窗口属性
    setWindowTitle(QString::fromUtf8("Maya 环境检测"));
    setMinimumSize(900, 700);
    resize(1000, 750);

    // 应用主题
    ThemeManager &theme = ThemeManager::instance();
    QString dialogStyle = QString(
        "QDialog {"
        "    background-color: %1;"
        "    border-radius: 8px;"
        "}"
    ).arg(theme.getBackgroundColor().name());
    setStyleSheet(dialogStyle);

    // 自动开始检测
    QTimer::singleShot(300, this, &MayaDetectionDialog::onStartDetection);
}

MayaDetectionDialog::~MayaDetectionDialog()
{
}

void MayaDetectionDialog::onStartDetection()
{
    if (m_isDetecting) {
        return;
    }

    m_isDetecting = true;
    m_detectedMayaVersions.clear();

    // 清空结果区域
    m_resultText->clear();
    m_resultText->append(QString::fromUtf8("🔍 开始检测 Maya 环境...\n"));
    m_resultText->append(QString::fromUtf8("检测时间: %1\n")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));

    // 显示日志文件位置
    QString logPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    m_resultText->append(QString::fromUtf8("\n📝 详细日志位置: %1\n").arg(logPath));
    m_resultText->append(QString::fromUtf8("   （如果检测有问题，请查看日志文件中的 qDebug 输出）\n"));

    m_resultText->append(addSeparator());

    // 禁用按钮
    m_startButton->setEnabled(false);
    m_refreshButton->setEnabled(false);
    m_exportButton->setEnabled(false);

    // 重置进度条
    m_progressBar->setValue(0);
    m_progressBar->setVisible(true);
    m_statusLabel->setText(QString::fromUtf8("正在检测..."));

    // 记录日志
    Application::instance().logger()->info("MayaDetectionDialog",
        QString::fromUtf8("开始 Maya 环境检测"));

    // 在后台线程执行检测（使用 QtConcurrent）
    QTimer::singleShot(100, this, [this]() {
        m_detectedMayaVersions = m_detector->detectAllMayaVersions();
        onDetectFinished();
    });
}

void MayaDetectionDialog::onRefreshClicked()
{
    onStartDetection();
}

void MayaDetectionDialog::onExportResults()
{
    if (m_detectedMayaVersions.isEmpty()) {
        QMessageBox::information(this, QString::fromUtf8("提示"),
            QString::fromUtf8("没有检测结果可导出，请先执行检测"));
        return;
    }

    // 选择保存路径
    QString defaultFileName = QString("Maya_Detection_Report_%1.txt")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

    QString filePath = QFileDialog::getSaveFileName(
        this,
        QString::fromUtf8("导出检测报告"),
        QDir::homePath() + "/" + defaultFileName,
        QString::fromUtf8("文本文件 (*.txt);;所有文件 (*.*)")
    );

    if (filePath.isEmpty()) {
        return;
    }

    // 写入文件
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QString::fromUtf8("错误"),
            QString::fromUtf8("无法创建文件: %1").arg(filePath));
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // 导出完整报告（包含所有插件）
    QString fullReport = generateFullReport(m_detectedMayaVersions);
    out << fullReport;
    file.close();

    QMessageBox::information(this, QString::fromUtf8("成功"),
        QString::fromUtf8("检测报告已导出到:\n%1").arg(filePath));

    Application::instance().logger()->info("MayaDetectionDialog",
        QString::fromUtf8("导出检测报告: %1").arg(filePath));
}

void MayaDetectionDialog::onDetectProgress(int progress, const QString &message)
{
    m_progressBar->setValue(progress);
    m_statusLabel->setText(message);
    m_resultText->append(QString::fromUtf8("[%1%] %2").arg(progress).arg(message));

    // 自动滚动到底部
    QScrollBar *scrollBar = m_resultText->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());

    // 处理事件，确保界面刷新
    QApplication::processEvents();
}

void MayaDetectionDialog::onDetectFinished()
{
    m_isDetecting = false;

    // 恢复按钮
    m_startButton->setEnabled(true);
    m_refreshButton->setEnabled(true);
    m_exportButton->setEnabled(!m_detectedMayaVersions.isEmpty());

    m_progressBar->setValue(100);
    m_statusLabel->setText(QString::fromUtf8("检测完成"));

    // 显示结果
    displayResults(m_detectedMayaVersions);

    Application::instance().logger()->info("MayaDetectionDialog",
        QString::fromUtf8("Maya 检测完成，找到 %1 个版本").arg(m_detectedMayaVersions.size()));
}

void MayaDetectionDialog::initUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(20, 20, 20, 20);
    m_mainLayout->setSpacing(15);

    // 标题
    m_titleLabel = new QLabel(QString::fromUtf8("🔍 Maya 环境检测工具"), this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setAlignment(Qt::AlignCenter);

    // 状态标签
    m_statusLabel = new QLabel(QString::fromUtf8("准备就绪"), this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet("color: #0078D4; font-weight: bold;");

    // 进度条
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setVisible(false);
    m_progressBar->setMinimumHeight(25);

    // 结果文本框
    m_resultText = new QTextEdit(this);
    m_resultText->setReadOnly(true);
    m_resultText->setFont(QFont("Consolas", 10));
    m_resultText->setPlaceholderText(QString::fromUtf8("检测结果将显示在这里..."));

    ThemeManager &theme = ThemeManager::instance();
    QString textEditStyle = QString(
        "QTextEdit {"
        "    background-color: %1;"
        "    border: 1px solid %2;"
        "    border-radius: 6px;"
        "    padding: 10px;"
        "    color: %3;"
        "}"
    ).arg(theme.getSurfaceColor().name())
     .arg(theme.getBorderColor().name())
     .arg(theme.getTextColor().name());
    m_resultText->setStyleSheet(textEditStyle);

    // 按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    m_startButton = new FluentButton(QString::fromUtf8("开始检测"), this);
    m_startButton->setIsPrimary(true);
    m_startButton->setMinimumWidth(120);

    m_refreshButton = new FluentButton(QString::fromUtf8("刷新"), this);
    m_refreshButton->setMinimumWidth(120);

    m_exportButton = new FluentButton(QString::fromUtf8("导出报告"), this);
    m_exportButton->setMinimumWidth(120);
    m_exportButton->setEnabled(false);

    m_closeButton = new FluentButton(QString::fromUtf8("关闭"), this);
    m_closeButton->setMinimumWidth(120);

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_refreshButton);
    buttonLayout->addWidget(m_exportButton);
    buttonLayout->addWidget(m_closeButton);
    buttonLayout->addStretch();

    // 添加到主布局
    m_mainLayout->addWidget(m_titleLabel);
    m_mainLayout->addWidget(m_statusLabel);
    m_mainLayout->addWidget(m_progressBar);
    m_mainLayout->addWidget(m_resultText, 1);
    m_mainLayout->addLayout(buttonLayout);
}

void MayaDetectionDialog::connectSignals()
{
    // 按钮信号
    connect(m_startButton, &FluentButton::clicked,
            this, &MayaDetectionDialog::onStartDetection);

    connect(m_refreshButton, &FluentButton::clicked,
            this, &MayaDetectionDialog::onRefreshClicked);

    connect(m_exportButton, &FluentButton::clicked,
            this, &MayaDetectionDialog::onExportResults);

    connect(m_closeButton, &FluentButton::clicked,
            this, &QDialog::accept);

    // 检测器信号
    connect(m_detector, &MayaDetector::detectProgress,
            this, &MayaDetectionDialog::onDetectProgress);

    connect(m_detector, &MayaDetector::detectFinished,
            this, &MayaDetectionDialog::onDetectFinished);
}

void MayaDetectionDialog::displayResults(const QVector<MayaSoftwareInfo> &mayaVersions)
{
    // 显示摘要
    m_resultText->append(addSeparator(QString::fromUtf8("检测摘要")));
    m_resultText->append(generateSummary(mayaVersions));
    m_resultText->append("\n");

    if (mayaVersions.isEmpty()) {
        m_resultText->append(addSeparator());
        m_resultText->append(QString::fromUtf8("❌ 未检测到 Maya 安装\n"));
        m_resultText->append(QString::fromUtf8("可能的原因:\n"));
        m_resultText->append(QString::fromUtf8("  1. 系统中未安装 Maya\n"));
        m_resultText->append(QString::fromUtf8("  2. Maya 安装在非标准路径\n"));
        m_resultText->append(QString::fromUtf8("  3. 注册表信息缺失\n"));
        m_resultText->append(addSeparator());
        return;
    }

    // 显示详细信息
    for (int i = 0; i < mayaVersions.size(); ++i) {
        m_resultText->append(addSeparator(QString::fromUtf8("Maya 版本 #%1").arg(i + 1)));
        m_resultText->append(formatMayaInfo(mayaVersions[i], i));
        m_resultText->append("\n");
    }

    // 显示统计信息
    m_resultText->append(addSeparator(QString::fromUtf8("统计信息")));

    // 统计渲染器
    QSet<QString> allRenderers;
    for (const auto &info : mayaVersions) {
        for (const QString &renderer : info.renderers) {
            allRenderers.insert(renderer);
        }
    }

    m_resultText->append(QString::fromUtf8("📊 检测到的渲染器类型: %1 种\n").arg(allRenderers.size()));
    if (!allRenderers.isEmpty()) {
        for (const QString &renderer : allRenderers) {
            m_resultText->append(QString::fromUtf8("   • %1\n").arg(renderer));
        }
    }

    // 统计插件总数
    int totalPlugins = 0;
    for (const auto &info : mayaVersions) {
        totalPlugins += info.plugins.size();
    }
    m_resultText->append(QString::fromUtf8("\n📦 检测到的插件总数: %1 个\n").arg(totalPlugins));

    m_resultText->append(addSeparator());
    m_resultText->append(QString::fromUtf8("✅ 检测完成！\n"));
    m_resultText->append(QString::fromUtf8("提示: 您可以点击「导出报告」按钮保存详细信息\n"));

    // 滚动到顶部
    m_resultText->moveCursor(QTextCursor::Start);
}

QString MayaDetectionDialog::formatMayaInfo(const MayaSoftwareInfo &info, int index, bool fullPluginList) const
{
    QString result;

    // 基本信息
    result += QString::fromUtf8("📦 软件名称: %1\n").arg(info.name);
    result += QString::fromUtf8("🔢 版本号: %1\n").arg(info.version);
    result += QString::fromUtf8("📝 完整版本: %1\n").arg(info.fullVersion.isEmpty() ? info.version : info.fullVersion);
    result += QString::fromUtf8("✅ 有效性: %1\n").arg(info.isValid ? QString::fromUtf8("是") : QString::fromUtf8("否"));
    result += "\n";

    // 路径信息
    result += QString::fromUtf8("📁 安装路径:\n");
    result += QString::fromUtf8("   %1\n\n").arg(info.installPath);

    result += QString::fromUtf8("⚙️  可执行文件:\n");
    result += QString::fromUtf8("   %1\n").arg(info.executablePath);
    result += QString::fromUtf8("   存在: %1\n\n").arg(QFile::exists(info.executablePath) ? QString::fromUtf8("是") : QString::fromUtf8("否"));

    // 渲染器信息
    result += QString::fromUtf8("🎨 渲染器 (%1 个):\n").arg(info.renderers.size());
    if (info.renderers.isEmpty()) {
        result += QString::fromUtf8("   (未检测到第三方渲染器)\n\n");
    } else {
        for (const QString &renderer : info.renderers) {
            result += QString::fromUtf8("   ✓ %1\n").arg(renderer);
        }
        result += "\n";
    }

    // 插件信息
    result += QString::fromUtf8("🔌 插件 (%1 个):\n").arg(info.plugins.size());
    if (info.plugins.isEmpty()) {
        result += QString::fromUtf8("   (未检测到插件)\n");
    } else {
        if (fullPluginList) {
            // 导出时显示所有插件
            for (int i = 0; i < info.plugins.size(); ++i) {
                result += QString::fromUtf8("   • %1\n").arg(info.plugins[i]);
            }
        } else {
            // 界面显示时只显示前20个插件
            int displayCount = qMin(20, info.plugins.size());
            for (int i = 0; i < displayCount; ++i) {
                result += QString::fromUtf8("   • %1\n").arg(info.plugins[i]);
            }
            if (info.plugins.size() > 20) {
                result += QString::fromUtf8("   ... 还有 %1 个插件（完整列表请导出报告查看）\n").arg(info.plugins.size() - 20);
            }
        }
    }

    return result;
}

QString MayaDetectionDialog::generateSummary(const QVector<MayaSoftwareInfo> &mayaVersions) const
{
    QString summary;

    summary += QString::fromUtf8("🎯 检测到 Maya 版本数: %1\n\n").arg(mayaVersions.size());

    if (!mayaVersions.isEmpty()) {
        summary += QString::fromUtf8("📋 版本列表:\n");
        for (int i = 0; i < mayaVersions.size(); ++i) {
            const auto &info = mayaVersions[i];
            QString status = info.isValid ? QString::fromUtf8("✅") : QString::fromUtf8("⚠️");
            summary += QString::fromUtf8("   %1 Maya %2 - %3 个渲染器, %4 个插件\n")
                .arg(status)
                .arg(info.version)
                .arg(info.renderers.size())
                .arg(info.plugins.size());
        }
    }

    return summary;
}

QString MayaDetectionDialog::addSeparator(const QString &title) const
{
    if (title.isEmpty()) {
        return QString::fromUtf8("═══════════════════════════════════════════════════════════════\n");
    } else {
        return QString::fromUtf8("\n═══════════════════════════════════════════════════════════════\n  %1\n═══════════════════════════════════════════════════════════════\n\n").arg(title);
    }
}

QString MayaDetectionDialog::generateFullReport(const QVector<MayaSoftwareInfo> &mayaVersions) const
{
    QString report;

    // 报告头部
    report += QString::fromUtf8("═══════════════════════════════════════════════════════════════\n");
    report += QString::fromUtf8("  盛世云图 - Maya 环境检测报告\n");
    report += QString::fromUtf8("═══════════════════════════════════════════════════════════════\n\n");
    report += QString::fromUtf8("检测时间: %1\n")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    report += QString::fromUtf8("报告版本: 1.0.0\n");
    report += QString::fromUtf8("检测平台: Windows\n");
    report += addSeparator();

    // 检测摘要
    report += addSeparator(QString::fromUtf8("检测摘要"));
    report += generateSummary(mayaVersions);
    report += "\n";

    if (mayaVersions.isEmpty()) {
        report += addSeparator();
        report += QString::fromUtf8("❌ 未检测到 Maya 安装\n\n");
        report += QString::fromUtf8("可能的原因:\n");
        report += QString::fromUtf8("  1. 系统中未安装 Maya\n");
        report += QString::fromUtf8("  2. Maya 安装在非标准路径\n");
        report += QString::fromUtf8("  3. 注册表信息缺失\n");
        report += addSeparator();
        return report;
    }

    // 详细信息（包含所有插件）
    for (int i = 0; i < mayaVersions.size(); ++i) {
        report += addSeparator(QString::fromUtf8("Maya 版本 #%1").arg(i + 1));
        report += formatMayaInfo(mayaVersions[i], i, true);  // true = 显示完整插件列表
        report += "\n";
    }

    // 统计信息
    report += addSeparator(QString::fromUtf8("统计信息"));

    // 统计渲染器
    QSet<QString> allRenderers;
    for (const auto &info : mayaVersions) {
        for (const QString &renderer : info.renderers) {
            allRenderers.insert(renderer);
        }
    }

    report += QString::fromUtf8("📊 检测到的渲染器类型: %1 种\n").arg(allRenderers.size());
    if (!allRenderers.isEmpty()) {
        for (const QString &renderer : allRenderers) {
            report += QString::fromUtf8("   • %1\n").arg(renderer);
        }
    }

    // 统计插件总数
    int totalPlugins = 0;
    for (const auto &info : mayaVersions) {
        totalPlugins += info.plugins.size();
    }
    report += QString::fromUtf8("\n📦 检测到的插件总数: %1 个\n").arg(totalPlugins);

    // 完整插件列表（按 Maya 版本分组）
    if (totalPlugins > 0) {
        report += addSeparator(QString::fromUtf8("完整插件列表"));
        for (int i = 0; i < mayaVersions.size(); ++i) {
            const auto &info = mayaVersions[i];
            if (!info.plugins.isEmpty()) {
                report += QString::fromUtf8("\nMaya %1 (%2 个插件):\n").arg(info.version).arg(info.plugins.size());
                for (int j = 0; j < info.plugins.size(); ++j) {
                    report += QString::fromUtf8("  %1. %2\n").arg(j + 1).arg(info.plugins[j]);
                }
            }
        }
    }

    // 报告尾部
    report += addSeparator();
    report += QString::fromUtf8("✅ 检测完成！\n\n");
    report += QString::fromUtf8("本报告由盛世云图客户端自动生成\n");
    report += QString::fromUtf8("技术支持: https://yuntu.com\n");
    report += addSeparator();

    return report;
}
