/**
 * @file CreateTaskDialog.cpp
 * @brief 新建任务对话框实现
 */

#include "CreateTaskDialog.h"
#include "../ThemeManager.h"
#include "../../services/MayaDetector.h"
#include "../../core/Logger.h"
#include "../../core/Application.h"
#include "../../network/ApiService.h"
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>

CreateTaskDialog::CreateTaskDialog(QWidget *parent)
    : QDialog(parent)
    , m_task(nullptr)
    , m_dialogPanel(nullptr)
    , m_ossUploader(nullptr)
    , m_titleLabel(nullptr)
    , m_closeButton(nullptr)
    , m_taskNameEdit(nullptr)
    , m_sceneFileEdit(nullptr)
    , m_browseButton(nullptr)
    , m_detectButton(nullptr)
    , m_startFrameSpinBox(nullptr)
    , m_endFrameSpinBox(nullptr)
    , m_frameStepSpinBox(nullptr)
    , m_widthSpinBox(nullptr)
    , m_heightSpinBox(nullptr)
    , m_rendererComboBox(nullptr)
    , m_outputFormatComboBox(nullptr)
    , m_outputPathEdit(nullptr)
    , m_priorityComboBox(nullptr)
    , m_createButton(nullptr)
    , m_cancelButton(nullptr)
    , m_detectStatusLabel(nullptr)
{
    // 设置对话框属性
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
    resize(700, 650);

    initUI();
    connectSignals();
}

CreateTaskDialog::~CreateTaskDialog()
{
}

void CreateTaskDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制半透明遮罩背景
    painter.fillRect(rect(), QColor(0, 0, 0, 100));
}

void CreateTaskDialog::onBrowseSceneClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        QString::fromUtf8("选择 Maya 场景文件"),
        QString(),
        QString::fromUtf8("Maya 场景文件 (*.ma *.mb);;所有文件 (*)"));

    if (!fileName.isEmpty()) {
        m_sceneFileEdit->setText(fileName);

        // 自动设置任务名称（如果未设置）
        if (m_taskNameEdit->text().isEmpty()) {
            QFileInfo fileInfo(fileName);
            m_taskNameEdit->setText(fileInfo.baseName());
        }
    }
}

void CreateTaskDialog::onDetectSceneClicked()
{
    QString sceneFile = m_sceneFileEdit->text().trimmed();

    if (sceneFile.isEmpty()) {
        m_detectStatusLabel->setText(QString::fromUtf8("❌ 请先选择场景文件"));
        m_detectStatusLabel->setStyleSheet("color: #D13438;");
        return;
    }

    if (!QFile::exists(sceneFile)) {
        m_detectStatusLabel->setText(QString::fromUtf8("❌ 场景文件不存在"));
        m_detectStatusLabel->setStyleSheet("color: #D13438;");
        return;
    }

    m_detectStatusLabel->setText(QString::fromUtf8("🔍 正在检测场景信息..."));
    m_detectStatusLabel->setStyleSheet("color: #0078D4;");
    m_detectButton->setEnabled(false);

    // 使用 MayaDetector 检测场景信息
    MayaDetector detector;

    // 检测 Maya 版本
    QString mayaVersion = detector.extractMayaVersionFromScene(sceneFile);
    if (!mayaVersion.isEmpty()) {
        Application::instance().logger()->info("CreateTaskDialog",
            QString::fromUtf8("检测到 Maya 版本: %1").arg(mayaVersion));
    }

    // 检测渲染器
    QString renderer = detector.extractRendererFromScene(sceneFile);
    if (!renderer.isEmpty()) {
        // 在下拉框中选择对应的渲染器
        int index = m_rendererComboBox->findText(renderer);
        if (index >= 0) {
            m_rendererComboBox->setCurrentIndex(index);
        }
        Application::instance().logger()->info("CreateTaskDialog",
            QString::fromUtf8("检测到渲染器: %1").arg(renderer));
    }

    // 检测缺失资源
    QStringList missingAssets = detector.detectMissingAssets(sceneFile);
    if (!missingAssets.isEmpty()) {
        m_detectStatusLabel->setText(QString::fromUtf8("⚠️ 检测到 %1 个缺失资源").arg(missingAssets.size()));
        m_detectStatusLabel->setStyleSheet("color: #FFB900;");

        Application::instance().logger()->warning("CreateTaskDialog",
            QString::fromUtf8("缺失资源: %1").arg(missingAssets.join(", ")));
    } else {
        m_detectStatusLabel->setText(QString::fromUtf8("✅ 场景检测完成，无缺失资源"));
        m_detectStatusLabel->setStyleSheet("color: #107C10;");
    }

    m_detectButton->setEnabled(true);
}

void CreateTaskDialog::onCreateClicked()
{
    if (!validateInput()) {
        return;
    }

    // 禁用创建按钮，防止重复点击
    m_createButton->setEnabled(false);
    m_createButton->setText(QString::fromUtf8("创建中..."));

    createTask();
}

void CreateTaskDialog::onCancelClicked()
{
    reject();
}

void CreateTaskDialog::initUI()
{
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 对话框面板
    m_dialogPanel = new QWidget(this);
    m_dialogPanel->setFixedSize(700, 650);

    // 面板布局
    QVBoxLayout *panelLayout = new QVBoxLayout(m_dialogPanel);
    panelLayout->setContentsMargins(0, 0, 0, 0);
    panelLayout->setSpacing(0);

    // 标题栏
    QWidget *titleBar = new QWidget(m_dialogPanel);
    titleBar->setFixedHeight(50);
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(20, 0, 12, 0);

    m_titleLabel = new QLabel(QString::fromUtf8("新建渲染任务"), titleBar);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);

    m_closeButton = new FluentButton(QString::fromUtf8("×"), titleBar);
    m_closeButton->setFixedSize(36, 36);

    titleLayout->addWidget(m_titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(m_closeButton);

    // 表单内容
    QWidget *contentWidget = new QWidget(m_dialogPanel);
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(20, 20, 20, 20);
    contentLayout->setSpacing(16);

    // 使用网格布局
    QGridLayout *formLayout = new QGridLayout();
    formLayout->setHorizontalSpacing(12);
    formLayout->setVerticalSpacing(12);

    int row = 0;

    // 任务名称
    QLabel *nameLabel = new QLabel(QString::fromUtf8("任务名称:"), contentWidget);
    m_taskNameEdit = new FluentLineEdit(QString::fromUtf8("请输入任务名称"), contentWidget);
    formLayout->addWidget(nameLabel, row, 0);
    formLayout->addWidget(m_taskNameEdit, row, 1, 1, 2);
    row++;

    // 场景文件
    QLabel *sceneLabel = new QLabel(QString::fromUtf8("场景文件:"), contentWidget);
    m_sceneFileEdit = new FluentLineEdit(QString::fromUtf8("请选择 Maya 场景文件"), contentWidget);
    m_browseButton = new FluentButton(QString::fromUtf8("浏览..."), contentWidget);
    m_browseButton->setFixedWidth(80);

    QHBoxLayout *sceneLayout = new QHBoxLayout();
    sceneLayout->addWidget(m_sceneFileEdit);
    sceneLayout->addWidget(m_browseButton);

    formLayout->addWidget(sceneLabel, row, 0);
    formLayout->addLayout(sceneLayout, row, 1, 1, 2);
    row++;

    // 检测按钮和状态
    m_detectButton = new FluentButton(QString::fromUtf8("检测场景信息"), contentWidget);
    m_detectButton->setFixedWidth(120);
    m_detectStatusLabel = new QLabel(contentWidget);
    m_detectStatusLabel->setWordWrap(true);

    QHBoxLayout *detectLayout = new QHBoxLayout();
    detectLayout->addWidget(m_detectButton);
    detectLayout->addWidget(m_detectStatusLabel);
    detectLayout->addStretch();

    formLayout->addWidget(new QLabel(), row, 0);
    formLayout->addLayout(detectLayout, row, 1, 1, 2);
    row++;

    // 分隔线
    QFrame *line1 = new QFrame(contentWidget);
    line1->setFrameShape(QFrame::HLine);
    line1->setFrameShadow(QFrame::Sunken);
    formLayout->addWidget(line1, row, 0, 1, 3);
    row++;

    // 帧范围
    QLabel *framesLabel = new QLabel(QString::fromUtf8("帧范围:"), contentWidget);
    m_startFrameSpinBox = new QSpinBox(contentWidget);
    m_startFrameSpinBox->setRange(1, 99999);
    m_startFrameSpinBox->setValue(1);

    m_endFrameSpinBox = new QSpinBox(contentWidget);
    m_endFrameSpinBox->setRange(1, 99999);
    m_endFrameSpinBox->setValue(100);

    m_frameStepSpinBox = new QSpinBox(contentWidget);
    m_frameStepSpinBox->setRange(1, 100);
    m_frameStepSpinBox->setValue(1);

    QHBoxLayout *framesLayout = new QHBoxLayout();
    framesLayout->addWidget(new QLabel(QString::fromUtf8("起始:"), contentWidget));
    framesLayout->addWidget(m_startFrameSpinBox);
    framesLayout->addWidget(new QLabel(QString::fromUtf8("结束:"), contentWidget));
    framesLayout->addWidget(m_endFrameSpinBox);
    framesLayout->addWidget(new QLabel(QString::fromUtf8("步长:"), contentWidget));
    framesLayout->addWidget(m_frameStepSpinBox);
    framesLayout->addStretch();

    formLayout->addWidget(framesLabel, row, 0);
    formLayout->addLayout(framesLayout, row, 1, 1, 2);
    row++;

    // 分辨率
    QLabel *resolutionLabel = new QLabel(QString::fromUtf8("分辨率:"), contentWidget);
    m_widthSpinBox = new QSpinBox(contentWidget);
    m_widthSpinBox->setRange(1, 16384);
    m_widthSpinBox->setValue(1920);

    m_heightSpinBox = new QSpinBox(contentWidget);
    m_heightSpinBox->setRange(1, 16384);
    m_heightSpinBox->setValue(1080);

    QHBoxLayout *resolutionLayout = new QHBoxLayout();
    resolutionLayout->addWidget(m_widthSpinBox);
    resolutionLayout->addWidget(new QLabel(QString::fromUtf8("×"), contentWidget));
    resolutionLayout->addWidget(m_heightSpinBox);
    resolutionLayout->addStretch();

    formLayout->addWidget(resolutionLabel, row, 0);
    formLayout->addLayout(resolutionLayout, row, 1, 1, 2);
    row++;

    // 渲染器
    QLabel *rendererLabel = new QLabel(QString::fromUtf8("渲染器:"), contentWidget);
    m_rendererComboBox = new QComboBox(contentWidget);
    m_rendererComboBox->addItems({
        QString::fromUtf8("Arnold"),
        QString::fromUtf8("V-Ray"),
        QString::fromUtf8("Redshift"),
        QString::fromUtf8("Maya Software")
    });
    formLayout->addWidget(rendererLabel, row, 0);
    formLayout->addWidget(m_rendererComboBox, row, 1, 1, 2);
    row++;

    // 输出格式
    QLabel *formatLabel = new QLabel(QString::fromUtf8("输出格式:"), contentWidget);
    m_outputFormatComboBox = new QComboBox(contentWidget);
    m_outputFormatComboBox->addItems({
        QString::fromUtf8("PNG"),
        QString::fromUtf8("JPEG"),
        QString::fromUtf8("EXR"),
        QString::fromUtf8("TIFF")
    });
    formLayout->addWidget(formatLabel, row, 0);
    formLayout->addWidget(m_outputFormatComboBox, row, 1, 1, 2);
    row++;

    // 输出路径
    QLabel *outputLabel = new QLabel(QString::fromUtf8("输出路径:"), contentWidget);
    m_outputPathEdit = new FluentLineEdit(QString::fromUtf8("默认输出路径"), contentWidget);
    formLayout->addWidget(outputLabel, row, 0);
    formLayout->addWidget(m_outputPathEdit, row, 1, 1, 2);
    row++;

    // 优先级
    QLabel *priorityLabel = new QLabel(QString::fromUtf8("优先级:"), contentWidget);
    m_priorityComboBox = new QComboBox(contentWidget);
    m_priorityComboBox->addItems({
        QString::fromUtf8("低"),
        QString::fromUtf8("普通"),
        QString::fromUtf8("高"),
        QString::fromUtf8("紧急")
    });
    m_priorityComboBox->setCurrentIndex(1);  // 默认普通
    formLayout->addWidget(priorityLabel, row, 0);
    formLayout->addWidget(m_priorityComboBox, row, 1, 1, 2);
    row++;

    contentLayout->addLayout(formLayout);
    contentLayout->addStretch();

    // 底部按钮栏
    QWidget *buttonBar = new QWidget(m_dialogPanel);
    buttonBar->setFixedHeight(70);
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonBar);
    buttonLayout->setContentsMargins(20, 16, 20, 16);
    buttonLayout->setSpacing(12);

    m_cancelButton = new FluentButton(QString::fromUtf8("取消"), buttonBar);
    m_cancelButton->setMinimumWidth(100);

    m_createButton = new FluentButton(QString::fromUtf8("创建任务"), buttonBar);
    m_createButton->setIsPrimary(true);
    m_createButton->setMinimumWidth(100);

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_createButton);

    // 添加到面板布局
    panelLayout->addWidget(titleBar);
    panelLayout->addWidget(contentWidget, 1);
    panelLayout->addWidget(buttonBar);

    // 将面板居中
    mainLayout->addStretch();
    mainLayout->addWidget(m_dialogPanel, 0, Qt::AlignCenter);
    mainLayout->addStretch();

    // 设置面板样式
    ThemeManager &theme = ThemeManager::instance();
    QString panelStyle = QString("background-color: %1; border-radius: 12px;")
        .arg(theme.getSurfaceColor().name());
    m_dialogPanel->setStyleSheet(panelStyle);

    // 应用阴影效果
    ThemeManager::instance().applyShadowEffect(m_dialogPanel, 40, 0, 10);
}

bool CreateTaskDialog::validateInput()
{
    // 验证任务名称
    QString taskName = m_taskNameEdit->text().trimmed();
    if (taskName.isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("验证失败"),
            QString::fromUtf8("请输入任务名称"));
        m_taskNameEdit->setFocus();
        return false;
    }

    // 验证场景文件
    QString sceneFile = m_sceneFileEdit->text().trimmed();
    if (sceneFile.isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("验证失败"),
            QString::fromUtf8("请选择场景文件"));
        m_sceneFileEdit->setFocus();
        return false;
    }

    if (!QFile::exists(sceneFile)) {
        QMessageBox::warning(this, QString::fromUtf8("验证失败"),
            QString::fromUtf8("场景文件不存在"));
        m_sceneFileEdit->setFocus();
        return false;
    }

    // 验证帧范围
    int startFrame = m_startFrameSpinBox->value();
    int endFrame = m_endFrameSpinBox->value();
    if (startFrame > endFrame) {
        QMessageBox::warning(this, QString::fromUtf8("验证失败"),
            QString::fromUtf8("起始帧不能大于结束帧"));
        m_startFrameSpinBox->setFocus();
        return false;
    }

    return true;
}

void CreateTaskDialog::createTask()
{
    // 创建任务对象
    m_task = new Task(this);

    // 设置基本信息
    m_task->setTaskName(m_taskNameEdit->text().trimmed());
    m_task->setSceneFile(m_sceneFileEdit->text().trimmed());

    // 设置渲染参数
    m_task->setStartFrame(m_startFrameSpinBox->value());
    m_task->setEndFrame(m_endFrameSpinBox->value());
    m_task->setFrameStep(m_frameStepSpinBox->value());
    m_task->setWidth(m_widthSpinBox->value());
    m_task->setHeight(m_heightSpinBox->value());

    // 设置渲染器和输出
    m_task->setRenderer(m_rendererComboBox->currentText());
    m_task->setOutputFormat(m_outputFormatComboBox->currentText());
    m_task->setOutputPath(m_outputPathEdit->text().trimmed());

    // 设置优先级
    TaskPriority priority = static_cast<TaskPriority>(m_priorityComboBox->currentIndex());
    m_task->setPriority(priority);

    // 设置状态和时间
    m_task->setStatus(TaskStatus::Draft);
    m_task->setCreatedAt(QDateTime::currentDateTime());

    Application::instance().logger()->info("CreateTaskDialog",
        QString::fromUtf8("创建任务: %1").arg(m_task->taskName()));

    // 调用 API 创建任务
    QJsonObject taskData;
    taskData["task_name"] = m_task->taskName();
    taskData["scene_file"] = QFileInfo(m_task->sceneFile()).fileName();  // 只传文件名
    taskData["start_frame"] = m_task->startFrame();
    taskData["end_frame"] = m_task->endFrame();
    taskData["frame_step"] = m_task->frameStep();
    taskData["width"] = m_task->width();
    taskData["height"] = m_task->height();
    taskData["renderer"] = m_task->renderer();
    taskData["output_format"] = m_task->outputFormat();
    taskData["priority"] = static_cast<int>(m_task->priority());

    ApiService::instance().createTask(taskData,
        [this](const QJsonObject& response) {
            // 任务创建成功，获取 taskId
            QString taskId = response["id"].toString();
            m_task->setId(taskId);

            Application::instance().logger()->info("CreateTaskDialog",
                QString::fromUtf8("任务创建成功，ID: %1").arg(taskId));

            // 开始上传场景文件
            startFileUpload(taskId);
        },
        [this](int code, const QString& error) {
            // 任务创建失败
            Application::instance().logger()->error("CreateTaskDialog",
                QString::fromUtf8("任务创建失败: %1").arg(error));

            QMessageBox::warning(this, QString::fromUtf8("创建失败"),
                QString::fromUtf8("任务创建失败: %1").arg(error));

            m_createButton->setEnabled(true);
            m_createButton->setText(QString::fromUtf8("创建任务"));
        });
}

void CreateTaskDialog::startFileUpload(const QString& taskId)
{
    QString sceneFile = m_task->sceneFile();
    QFileInfo fileInfo(sceneFile);
    QString fileName = fileInfo.fileName();

    m_createButton->setText(QString::fromUtf8("获取上传凭证..."));

    // 获取 STS 上传凭证
    ApiService::instance().getUploadCredentials(taskId, fileName,
        [this, taskId, fileName, sceneFile](const QJsonObject& response) {
            // 解析 STS 凭证
            QString accessKeyId = response["accessKeyId"].toString();
            QString accessKeySecret = response["accessKeySecret"].toString();
            QString securityToken = response["securityToken"].toString();
            QString endpoint = response["endpoint"].toString();
            QString bucketName = response["bucketName"].toString();
            QString objectKey = response["objectKey"].toString();
            QString expiration = response["expiration"].toString();

            Application::instance().logger()->info("CreateTaskDialog",
                QString::fromUtf8("获取上传凭证成功，开始上传文件: %1").arg(fileName));

#ifdef ENABLE_OSS_SDK
            // 使用 OSS SDK 上传
            if (!m_ossUploader) {
                m_ossUploader = new OSSUploader(this);

                // 连接上传信号
                connect(m_ossUploader, &OSSUploader::progressChanged,
                        this, &CreateTaskDialog::onUploadProgress);
                connect(m_ossUploader, &OSSUploader::uploadFinished,
                        this, &CreateTaskDialog::onUploadFinished);
                connect(m_ossUploader, &OSSUploader::uploadError,
                        this, &CreateTaskDialog::onUploadError);
            }

            // 准备 STS 凭证
            OSSUploader::STSCredentials credentials;
            credentials.accessKeyId = accessKeyId;
            credentials.accessKeySecret = accessKeySecret;
            credentials.securityToken = securityToken;
            credentials.endpoint = endpoint;
            credentials.bucketName = bucketName;
            credentials.objectKey = objectKey;
            credentials.expiration = expiration;

            // 配置上传参数
            OSSUploader::UploadConfig config;
            config.partSize = 10 * 1024 * 1024;  // 10MB
            config.threadNum = 3;
            config.maxRetries = 5;
            config.enableCheckpoint = true;

            m_createButton->setText(QString::fromUtf8("上传中 0%"));

            // 开始上传
            m_ossUploader->startUpload(sceneFile, taskId, credentials, config);
#else
            // OSS SDK 不可用，提示用户
            Application::instance().logger()->warning("CreateTaskDialog",
                QString::fromUtf8("OSS SDK 未启用，无法直接上传到 OSS"));

            QMessageBox::warning(this, QString::fromUtf8("上传失败"),
                QString::fromUtf8("OSS SDK 未启用，请重新编译客户端并启用 OSS SDK 支持"));

            m_createButton->setEnabled(true);
            m_createButton->setText(QString::fromUtf8("创建任务"));
#endif
        },
        [this](int code, const QString& error) {
            // 获取凭证失败
            Application::instance().logger()->error("CreateTaskDialog",
                QString::fromUtf8("获取上传凭证失败: %1").arg(error));

            QMessageBox::warning(this, QString::fromUtf8("上传失败"),
                QString::fromUtf8("获取上传凭证失败: %1").arg(error));

            m_createButton->setEnabled(true);
            m_createButton->setText(QString::fromUtf8("创建任务"));
        });
}

void CreateTaskDialog::onUploadProgress(int progress, qint64 uploadedBytes, qint64 totalBytes)
{
    m_createButton->setText(QString::fromUtf8("上传中 %1%").arg(progress));

    Application::instance().logger()->debug("CreateTaskDialog",
        QString::fromUtf8("上传进度: %1% (%2/%3 bytes)")
            .arg(progress)
            .arg(uploadedBytes)
            .arg(totalBytes));
}

void CreateTaskDialog::onUploadFinished(bool success)
{
    if (success) {
        Application::instance().logger()->info("CreateTaskDialog",
            QString::fromUtf8("文件上传成功"));

        m_createButton->setText(QString::fromUtf8("上传完成"));

        QMessageBox::information(this, QString::fromUtf8("创建成功"),
            QString::fromUtf8("任务创建并上传成功！"));

        accept();
    } else {
        Application::instance().logger()->error("CreateTaskDialog",
            QString::fromUtf8("文件上传失败"));

        QMessageBox::warning(this, QString::fromUtf8("上传失败"),
            QString::fromUtf8("文件上传失败，请重试"));

        m_createButton->setEnabled(true);
        m_createButton->setText(QString::fromUtf8("创建任务"));
    }
}

void CreateTaskDialog::onUploadError(const QString& error)
{
    Application::instance().logger()->error("CreateTaskDialog",
        QString::fromUtf8("上传错误: %1").arg(error));

    QMessageBox::warning(this, QString::fromUtf8("上传失败"),
        QString::fromUtf8("上传错误: %1").arg(error));

    m_createButton->setEnabled(true);
    m_createButton->setText(QString::fromUtf8("创建任务"));
}

void CreateTaskDialog::connectSignals()
{
    connect(m_closeButton, &FluentButton::clicked, this, &CreateTaskDialog::onCancelClicked);
    connect(m_browseButton, &FluentButton::clicked, this, &CreateTaskDialog::onBrowseSceneClicked);
    connect(m_detectButton, &FluentButton::clicked, this, &CreateTaskDialog::onDetectSceneClicked);
    connect(m_createButton, &FluentButton::clicked, this, &CreateTaskDialog::onCreateClicked);
    connect(m_cancelButton, &FluentButton::clicked, this, &CreateTaskDialog::onCancelClicked);

    // 主题变更时更新面板背景
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [this](ThemeType theme) {
                Q_UNUSED(theme);
                ThemeManager &themeMgr = ThemeManager::instance();
                QString panelStyle = QString("background-color: %1; border-radius: 12px;")
                    .arg(themeMgr.getSurfaceColor().name());
                m_dialogPanel->setStyleSheet(panelStyle);
            });
}
