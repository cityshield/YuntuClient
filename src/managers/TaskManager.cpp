/**
 * @file TaskManager.cpp
 * @brief 任务管理器实现
 */

#include "TaskManager.h"
#include "../core/Logger.h"
#include "../core/Application.h"
#include "../ui/components/ToastManager.h"
#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <algorithm>

TaskManager::TaskManager(QObject *parent)
    : QObject(parent)
    , m_wsClient(nullptr)
    , m_fileUploader(nullptr)
    , m_progressPollTimer(nullptr)
    , m_isInitialized(false)
{
    // 创建文件上传器
    m_fileUploader = new FileUploader(this);

    // 创建 WebSocket 客户端
    m_wsClient = new WebSocketClient(this);

    // 创建进度轮询定时器（作为降级方案）
    m_progressPollTimer = new QTimer(this);
    m_progressPollTimer->setInterval(5000);  // 每 5 秒轮询一次
    connect(m_progressPollTimer, &QTimer::timeout, this, &TaskManager::pollActiveTasksProgress);
}

TaskManager::~TaskManager()
{
    cleanup();
}

TaskManager& TaskManager::instance()
{
    static TaskManager instance;
    return instance;
}

void TaskManager::initialize()
{
    if (m_isInitialized) {
        return;
    }

    Application::instance().logger()->info("TaskManager", QString::fromUtf8("初始化任务管理器"));

    // 连接 WebSocket 信号
    connectWebSocketSignals();

    m_isInitialized = true;
}

void TaskManager::cleanup()
{
    Application::instance().logger()->info("TaskManager", QString::fromUtf8("清理任务管理器"));

    // 清理任务列表
    qDeleteAll(m_tasks);
    m_tasks.clear();
    m_taskMap.clear();

    m_isInitialized = false;
}

QList<Task*> TaskManager::getTasksByStatus(TaskStatus status) const
{
    QList<Task*> result;
    for (Task* task : m_tasks) {
        if (task->status() == status) {
            result.append(task);
        }
    }
    return result;
}

QList<Task*> TaskManager::getTasksByPriority(TaskPriority priority) const
{
    QList<Task*> result;
    for (Task* task : m_tasks) {
        if (task->priority() == priority) {
            result.append(task);
        }
    }
    return result;
}

Task* TaskManager::getTaskById(const QString& taskId) const
{
    return m_taskMap.value(taskId, nullptr);
}

int TaskManager::getTaskCountByStatus(TaskStatus status) const
{
    int count = 0;
    for (Task* task : m_tasks) {
        if (task->status() == status) {
            count++;
        }
    }
    return count;
}

void TaskManager::refreshTaskList()
{
    Application::instance().logger()->info("TaskManager", QString::fromUtf8("刷新任务列表"));

    ApiService::instance().getTasks(
        QString(),  // status filter
        0,          // skip
        100,        // limit
        [this](const QJsonObject& response) {
            // 清空当前任务列表
            qDeleteAll(m_tasks);
            m_tasks.clear();
            m_taskMap.clear();

            // 解析任务列表
            QJsonArray tasksArray = response["tasks"].toArray();
            for (const QJsonValue& value : tasksArray) {
                QJsonObject taskJson = value.toObject();
                Task* task = Task::fromJson(taskJson, this);
                addTask(task);
            }

            // 排序任务
            sortTasks();

            Application::instance().logger()->info("TaskManager", QString::fromUtf8("任务列表刷新成功，共 %1 个任务").arg(m_tasks.size()));
            emit taskListUpdated();
        },
        [this](int statusCode, const QString& error) {
            Application::instance().logger()->error("TaskManager", QString::fromUtf8("刷新任务列表失败: %1").arg(error));
        }
    );
}

void TaskManager::createTask(const QString& taskName, const QString& sceneFile, RenderConfig* config)
{
    qDebug() << "========== 创建新任务 ==========";
    qDebug() << "任务名称:" << taskName;
    qDebug() << "场景文件:" << sceneFile;
    Application::instance().logger()->info("TaskManager", QString::fromUtf8("创建新任务: %1").arg(taskName));

    // 创建任务对象
    Task* task = new Task(this);
    task->setTaskName(taskName);
    task->setSceneFile(sceneFile);
    task->setStatus(TaskStatus::Draft);
    task->setCreatedAt(QDateTime::currentDateTime());

    // 设置渲染配置
    if (config) {
        task->setRenderer(config->rendererString());
        task->setOutputFormat(config->imageFormatString());
    }

    // 添加到列表
    addTask(task);

    Application::instance().logger()->info("TaskManager", QString::fromUtf8("任务创建成功: %1").arg(taskName));
    emit taskCreated(task);
    emit taskListUpdated();
}

void TaskManager::submitTask(Task* task)
{
    if (!task) {
        Application::instance().logger()->error("TaskManager", QString::fromUtf8("提交任务失败: 任务对象为空"));
        emit taskSubmissionFailed("", QString::fromUtf8("任务对象为空"));
        return;
    }

    Application::instance().logger()->info("TaskManager", QString::fromUtf8("提交任务: %1").arg(task->taskName()));

    // 检查场景文件是否存在
    QString sceneFile = task->sceneFile();
    if (sceneFile.isEmpty()) {
        Application::instance().logger()->error("TaskManager", QString::fromUtf8("提交任务失败: 场景文件路径为空"));
        emit taskSubmissionFailed("", QString::fromUtf8("场景文件路径为空"));
        return;
    }

    QFile file(sceneFile);
    if (!file.exists()) {
        Application::instance().logger()->error("TaskManager", QString::fromUtf8("提交任务失败: 场景文件不存在: %1").arg(sceneFile));
        emit taskSubmissionFailed("", QString::fromUtf8("场景文件不存在: %1").arg(sceneFile));
        return;
    }

    // 生成本地临时 ID（用于跟踪上传进度）
    QString localTaskId = QString("local_%1").arg(QDateTime::currentMSecsSinceEpoch());

    // 添加到任务列表（如果还没有）
    if (!m_tasks.contains(task)) {
        addTask(task);
    }

    // 更新任务状态为上传中
    task->setStatus(TaskStatus::Uploading);
    task->setProgress(0);
    m_uploadingTasks[localTaskId] = task;

    Application::instance().logger()->info("TaskManager", QString::fromUtf8("开始上传场景文件: %1").arg(sceneFile));
    emit taskStatusUpdated(localTaskId, TaskStatus::Uploading);
    emit taskListUpdated();

    // 先断开之前可能存在的所有连接，避免重复连接
    disconnect(m_fileUploader, &FileUploader::progressChanged, this, nullptr);
    disconnect(m_fileUploader, &FileUploader::uploadFinished, this, nullptr);
    disconnect(m_fileUploader, &FileUploader::uploadError, this, nullptr);

    // 连接上传器信号 - 使用 Qt::UniqueConnection 避免重复连接
    connect(m_fileUploader, &FileUploader::progressChanged, this,
        [this, localTaskId, task](int progress, qint64 uploadedBytes, qint64 totalBytes) {
            if (!m_uploadingTasks.contains(localTaskId)) {
                return; // 任务已被取消或完成
            }
            task->setProgress(progress);
            emit fileUploadProgress(localTaskId, progress, uploadedBytes, totalBytes);
            emit taskProgressUpdated(localTaskId, progress);
        },
        Qt::UniqueConnection
    );

    connect(m_fileUploader, &FileUploader::uploadFinished, this,
        [this, localTaskId, task, sceneFile](bool success) {
            if (!m_uploadingTasks.contains(localTaskId)) {
                return; // 任务已被取消或完成
            }

            if (!success) {
                Application::instance().logger()->error("TaskManager", QString::fromUtf8("文件上传失败"));
                task->setStatus(TaskStatus::Failed);
                task->setErrorMessage(QString::fromUtf8("文件上传失败"));
                m_uploadingTasks.remove(localTaskId);
                emit fileUploadFailed(localTaskId, QString::fromUtf8("文件上传失败"));
                emit taskSubmissionFailed(localTaskId, QString::fromUtf8("文件上传失败"));
                emit taskListUpdated();
                return;
            }

            Application::instance().logger()->info("TaskManager", QString::fromUtf8("文件上传成功，开始创建任务"));

            // 文件上传成功，调用后端 API 创建任务
            QJsonObject taskJson = task->toJson();
            taskJson["sceneFileUrl"] = sceneFile;  // 实际应该是 OSS URL，这里简化处理

            ApiService::instance().createTask(
                taskJson,
                [this, localTaskId, task](const QJsonObject& response) {
                    if (!m_uploadingTasks.contains(localTaskId)) {
                        return; // 任务已被取消
                    }

                    // 更新任务 ID
                    QString taskId = response["taskId"].toString();
                    task->setTaskId(taskId);
                    task->setStatus(TaskStatus::Pending);
                    task->setProgress(0);

                    // 更新 map
                    m_taskMap[taskId] = task;
                    m_uploadingTasks.remove(localTaskId);

                    Application::instance().logger()->info("TaskManager", QString::fromUtf8("任务提交成功: %1").arg(taskId));
                    emit taskSubmitted(taskId);
                    emit taskStatusUpdated(taskId, TaskStatus::Pending);
                    emit taskListUpdated();
                },
                [this, localTaskId, task](int statusCode, const QString& error) {
                    if (!m_uploadingTasks.contains(localTaskId)) {
                        return; // 任务已被取消
                    }

                    Application::instance().logger()->error("TaskManager", QString::fromUtf8("任务提交失败: %1").arg(error));
                    task->setStatus(TaskStatus::Failed);
                    task->setErrorMessage(error);
                    m_uploadingTasks.remove(localTaskId);
                    emit taskSubmissionFailed(localTaskId, error);
                    emit taskListUpdated();
                }
            );
        },
        Qt::UniqueConnection
    );

    connect(m_fileUploader, &FileUploader::uploadError, this,
        [this, localTaskId, task](const QString& error) {
            if (!m_uploadingTasks.contains(localTaskId)) {
                return; // 任务已被取消或完成
            }

            Application::instance().logger()->error("TaskManager", QString::fromUtf8("文件上传错误: %1").arg(error));
            task->setStatus(TaskStatus::Failed);
            task->setErrorMessage(error);
            m_uploadingTasks.remove(localTaskId);
            emit fileUploadFailed(localTaskId, error);
            emit taskSubmissionFailed(localTaskId, error);
            emit taskListUpdated();
        },
        Qt::UniqueConnection
    );

    // 开始上传文件
    m_fileUploader->startUpload(sceneFile, localTaskId);
}

void TaskManager::startTask(const QString& taskId)
{
    Application::instance().logger()->info("TaskManager", QString::fromUtf8("开始任务: %1").arg(taskId));

    ApiService::instance().resumeTask(
        taskId,
        [this, taskId](const QJsonObject& response) {
            // 更新本地任务状态
            Task* task = getTaskById(taskId);
            if (task) {
                task->setStatus(TaskStatus::Rendering);
            }

            Application::instance().logger()->info("TaskManager", QString::fromUtf8("任务开始成功: %1").arg(taskId));
            emit taskOperationSuccess(taskId, "start");
            emit taskStatusUpdated(taskId, TaskStatus::Rendering);
        },
        [this, taskId](int statusCode, const QString& error) {
            Application::instance().logger()->error("TaskManager", QString::fromUtf8("开始任务失败: %1").arg(error));
            emit taskOperationFailed(taskId, "start", error);
        }
    );
}

void TaskManager::pauseTask(const QString& taskId)
{
    Application::instance().logger()->info("TaskManager", QString::fromUtf8("暂停任务: %1").arg(taskId));

    ApiService::instance().pauseTask(
        taskId,
        [this, taskId](const QJsonObject& response) {
            // 更新本地任务状态
            Task* task = getTaskById(taskId);
            if (task) {
                task->setStatus(TaskStatus::Paused);
            }

            Application::instance().logger()->info("TaskManager", QString::fromUtf8("任务暂停成功: %1").arg(taskId));
            emit taskOperationSuccess(taskId, "pause");
            emit taskStatusUpdated(taskId, TaskStatus::Paused);
        },
        [this, taskId](int statusCode, const QString& error) {
            Application::instance().logger()->error("TaskManager", QString::fromUtf8("暂停任务失败: %1").arg(error));
            emit taskOperationFailed(taskId, "pause", error);
        }
    );
}

void TaskManager::resumeTask(const QString& taskId)
{
    Application::instance().logger()->info("TaskManager", QString::fromUtf8("恢复任务: %1").arg(taskId));

    ApiService::instance().resumeTask(
        taskId,
        [this, taskId](const QJsonObject& response) {
            // 更新本地任务状态
            Task* task = getTaskById(taskId);
            if (task) {
                task->setStatus(TaskStatus::Queued);
            }

            Application::instance().logger()->info("TaskManager", QString::fromUtf8("任务恢复成功: %1").arg(taskId));
            emit taskOperationSuccess(taskId, "resume");
            emit taskStatusUpdated(taskId, TaskStatus::Queued);
        },
        [this, taskId](int statusCode, const QString& error) {
            Application::instance().logger()->error("TaskManager", QString::fromUtf8("恢复任务失败: %1").arg(error));
            emit taskOperationFailed(taskId, "resume", error);
        }
    );
}

void TaskManager::cancelTask(const QString& taskId)
{
    Application::instance().logger()->info("TaskManager", QString::fromUtf8("取消任务: %1").arg(taskId));

    ApiService::instance().cancelTask(
        taskId,
        [this, taskId](const QJsonObject& response) {
            // 更新本地任务状态
            Task* task = getTaskById(taskId);
            if (task) {
                task->setStatus(TaskStatus::Cancelled);
            }

            Application::instance().logger()->info("TaskManager", QString::fromUtf8("任务取消成功: %1").arg(taskId));
            emit taskOperationSuccess(taskId, "cancel");
            emit taskStatusUpdated(taskId, TaskStatus::Cancelled);
        },
        [this, taskId](int statusCode, const QString& error) {
            Application::instance().logger()->error("TaskManager", QString::fromUtf8("取消任务失败: %1").arg(error));
            emit taskOperationFailed(taskId, "cancel", error);
        }
    );
}

void TaskManager::deleteTask(const QString& taskId)
{
    Application::instance().logger()->info("TaskManager", QString::fromUtf8("删除任务: %1").arg(taskId));

    ApiService::instance().deleteTask(
        taskId,
        false,  // deleteCloudData
        [this, taskId](const QJsonObject& response) {
            // 从本地列表删除
            removeTask(taskId);

            Application::instance().logger()->info("TaskManager", QString::fromUtf8("任务删除成功: %1").arg(taskId));
            emit taskOperationSuccess(taskId, "delete");
            emit taskRemoved(taskId);
            emit taskListUpdated();
        },
        [this, taskId](int statusCode, const QString& error) {
            Application::instance().logger()->error("TaskManager", QString::fromUtf8("删除任务失败: %1").arg(error));
            emit taskOperationFailed(taskId, "delete", error);
        }
    );
}

void TaskManager::fetchTaskDetails(const QString& taskId)
{
    Application::instance().logger()->info("TaskManager", QString::fromUtf8("获取任务详情: %1").arg(taskId));

    ApiService::instance().getTask(
        taskId,
        [this, taskId](const QJsonObject& response) {
            // 更新任务信息
            updateTask(taskId, response);

            Task* task = getTaskById(taskId);
            Application::instance().logger()->info("TaskManager", QString::fromUtf8("任务详情获取成功: %1").arg(taskId));
            emit taskDetailsFetched(task);
        },
        [this, taskId](int statusCode, const QString& error) {
            Application::instance().logger()->error("TaskManager", QString::fromUtf8("获取任务详情失败: %1").arg(error));
        }
    );
}

void TaskManager::downloadTaskResults(const QString& taskId, const QString& savePath)
{
    Application::instance().logger()->info("TaskManager", QString::fromUtf8("下载任务结果: %1 -> %2").arg(taskId, savePath));

    // 这里需要实现文件下载逻辑
    // 可以使用 HttpClient 的下载功能
    // 暂时留空，等待具体实现
}

void TaskManager::clearAllTasks()
{
    Application::instance().logger()->info("TaskManager", QString::fromUtf8("清空所有任务"));

    qDeleteAll(m_tasks);
    m_tasks.clear();
    m_taskMap.clear();

    emit taskListUpdated();
}

void TaskManager::saveTasksToLocal()
{
    QJsonArray tasksArray;
    for (Task* task : m_tasks) {
        tasksArray.append(task->toJson());
    }

    QJsonObject root;
    root["tasks"] = tasksArray;
    root["lastUpdate"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonDocument doc(root);
    QByteArray data = doc.toJson();

    // 保存到文件
    QString dataPath = QDir::homePath() + "/AppData/Roaming/YunTu";
    QDir().mkpath(dataPath);

    QFile file(dataPath + "/tasks.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(data);
        file.close();
        Application::instance().logger()->debug("TaskManager", QString::fromUtf8("任务列表已保存到本地"));
    } else {
        Application::instance().logger()->error("TaskManager", QString::fromUtf8("保存任务列表失败"));
    }
}

void TaskManager::loadTasksFromLocal()
{
    QString dataPath = QDir::homePath() + "/AppData/Roaming/YunTu/tasks.json";
    QFile file(dataPath);

    if (!file.exists()) {
        Application::instance().logger()->debug("TaskManager", QString::fromUtf8("本地任务文件不存在"));
        return;
    }

    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject root = doc.object();
        QJsonArray tasksArray = root["tasks"].toArray();

        for (const QJsonValue& value : tasksArray) {
            QJsonObject taskJson = value.toObject();
            Task* task = Task::fromJson(taskJson, this);
            addTask(task);
        }

        sortTasks();

        Application::instance().logger()->info("TaskManager", QString::fromUtf8("从本地加载 %1 个任务").arg(m_tasks.size()));
    } else {
        Application::instance().logger()->error("TaskManager", QString::fromUtf8("加载本地任务列表失败"));
    }
}

void TaskManager::addTask(Task* task)
{
    if (!task) {
        return;
    }

    m_tasks.append(task);

    if (!task->taskId().isEmpty()) {
        m_taskMap[task->taskId()] = task;
    }

    emit taskAdded(task);
    emit taskListUpdated();
}

void TaskManager::removeTask(const QString& taskId)
{
    Task* task = m_taskMap.value(taskId, nullptr);
    if (task) {
        m_tasks.removeOne(task);
        m_taskMap.remove(taskId);
        delete task;
    }
}

void TaskManager::updateTask(const QString& taskId, const QJsonObject& taskData)
{
    Task* task = getTaskById(taskId);
    if (!task) {
        // 任务不存在，创建新任务
        task = Task::fromJson(taskData, this);
        addTask(task);
    } else {
        // 更新现有任务
        task->setTaskName(taskData["taskName"].toString());
        task->setStatus(static_cast<TaskStatus>(taskData["status"].toInt()));
        task->setProgress(taskData["progress"].toInt());
        task->setPriority(static_cast<TaskPriority>(taskData["priority"].toInt()));
        // ... 更新其他字段
    }

    emit taskListUpdated();
}

void TaskManager::connectWebSocketSignals()
{
    if (!m_wsClient) {
        return;
    }

    Application::instance().logger()->info("TaskManager", QString::fromUtf8("连接 WebSocket 信号"));

    // 连接任务进度更新信号
    connect(m_wsClient, &WebSocketClient::taskProgressUpdated,
            this, &TaskManager::handleTaskProgressUpdate);

    // 连接任务状态变化信号
    connect(m_wsClient, &WebSocketClient::taskStatusChanged,
            this, [this](const QString& taskId, const QString& status) {
        // 将字符串状态转换为枚举
        TaskStatus taskStatus = TaskStatus::Pending;  // 默认值
        if (status == "pending") taskStatus = TaskStatus::Pending;
        else if (status == "queued") taskStatus = TaskStatus::Queued;
        else if (status == "rendering") taskStatus = TaskStatus::Rendering;
        else if (status == "paused") taskStatus = TaskStatus::Paused;
        else if (status == "completed") taskStatus = TaskStatus::Completed;
        else if (status == "failed") taskStatus = TaskStatus::Failed;
        else if (status == "cancelled") taskStatus = TaskStatus::Cancelled;

        handleTaskStatusUpdate(taskId, static_cast<int>(taskStatus));
    });

    // 连接 WebSocket 连接状态信号
    connect(m_wsClient, &WebSocketClient::connected, this, [this]() {
        Application::instance().logger()->info("TaskManager", QString::fromUtf8("WebSocket 已连接，停止进度轮询"));
        // WebSocket 连接成功，停止轮询
        if (m_progressPollTimer) {
            m_progressPollTimer->stop();
        }
    });

    connect(m_wsClient, &WebSocketClient::disconnected, this, [this]() {
        Application::instance().logger()->warning("TaskManager", QString::fromUtf8("WebSocket 已断开，启动进度轮询降级"));
        // WebSocket 断开，启动轮询作为降级方案
        if (m_progressPollTimer) {
            m_progressPollTimer->start();
        }
    });
}

void TaskManager::handleTaskStatusUpdate(const QString& taskId, int status)
{
    Task* task = getTaskById(taskId);
    if (task) {
        task->setStatus(static_cast<TaskStatus>(status));
        emit taskStatusUpdated(taskId, static_cast<TaskStatus>(status));
    }
}

void TaskManager::handleTaskProgressUpdate(const QString& taskId, int progress)
{
    Task* task = getTaskById(taskId);
    if (task) {
        Application::instance().logger()->debug("TaskManager",
            QString::fromUtf8("任务进度更新: %1 -> %2%").arg(taskId).arg(progress));
        task->setProgress(progress);
        emit taskProgressUpdated(taskId, progress);
    }
}

void TaskManager::connectWebSocket(const QString& url, const QString& userId)
{
    if (!m_wsClient) {
        Application::instance().logger()->error("TaskManager",
            QString::fromUtf8("WebSocket 客户端未初始化"));
        return;
    }

    Application::instance().logger()->info("TaskManager",
        QString::fromUtf8("连接到 WebSocket: %1, 用户: %2").arg(url).arg(userId));

    m_wsClient->connectToServer(url, userId);
}

void TaskManager::pollActiveTasksProgress()
{
    // 仅在 WebSocket 未连接时才轮询
    if (m_wsClient && m_wsClient->isConnected()) {
        return;
    }

    Application::instance().logger()->debug("TaskManager",
        QString::fromUtf8("轮询活跃任务进度"));

    // 获取所有活跃状态的任务（正在渲染或排队中）
    QList<Task*> activeTasks;
    for (Task* task : m_tasks) {
        if (task->status() == TaskStatus::Rendering ||
            task->status() == TaskStatus::Queued) {
            activeTasks.append(task);
        }
    }

    if (activeTasks.isEmpty()) {
        return;
    }

    // 对每个活跃任务请求最新状态
    for (Task* task : activeTasks) {
        fetchTaskDetails(task->taskId());
    }
}

void TaskManager::sortTasks()
{
    // 按创建时间降序排序（最新的在前面）
    std::sort(m_tasks.begin(), m_tasks.end(), [](Task* a, Task* b) {
        return a->createdAt() > b->createdAt();
    });
}

#ifdef ENABLE_OSS_SDK
// 上传管理方法实现
void TaskManager::startTaskUpload(Task* task,
                                  const QString& filePath,
                                  const OSSUploader::STSCredentials& credentials,
                                  const OSSUploader::UploadConfig& config)
{
    if (!task) {
        Application::instance().logger()->error("TaskManager",
            QString::fromUtf8("开始上传失败: task 为空"));
        return;
    }

    QString taskId = task->taskId();

    // 如果任务不在列表中，添加到列表
    // 注意：CreateTaskDialog 已经提前添加了任务，这里只是安全检查
    if (!m_tasks.contains(task)) {
        addTask(task);  // addTask 会发出 taskListUpdated 信号
        Application::instance().logger()->debug("TaskManager",
            QString::fromUtf8("任务 %1 已添加到任务列表").arg(taskId));
    }

    // 如果已经在上传，先停止
    if (m_uploaders.contains(taskId)) {
        Application::instance().logger()->warning("TaskManager",
            QString::fromUtf8("任务 %1 正在上传，先取消旧的上传").arg(taskId));
        cancelTaskUpload(taskId);
    }

    Application::instance().logger()->info("TaskManager",
        QString::fromUtf8("开始上传任务文件: %1, 文件: %2").arg(taskId).arg(filePath));

    // 创建上传器
    OSSUploader* uploader = new OSSUploader(this);
    m_uploaders[taskId] = uploader;

    // 更新任务状态（如果 CreateTaskDialog 已经设置，这里可能是重复设置）
    // 但为了安全起见，仍然设置一次
    task->setStatus(TaskStatus::Uploading);
    task->setIsUploading(true);
    task->setUploadPaused(false);
    // 不重置进度，保留 CreateTaskDialog 设置的值

    // 连接上传器信号到任务对象
    connect(uploader, &OSSUploader::progressChanged,
            task, [task](int progress, qint64 uploaded, qint64 total) {
        task->setUploadProgress(progress);
        task->setUploadedBytes(uploaded);
        task->setTotalBytes(total);
    });

    connect(uploader, &OSSUploader::speedChanged,
            task, [task](qint64 bytesPerSecond) {
        task->setUploadSpeed(bytesPerSecond);
    });

    connect(uploader, &OSSUploader::uploadFinished,
            this, [this, task, taskId](bool success) {
        Application::instance().logger()->info("TaskManager",
            QString::fromUtf8("任务 %1 上传完成，成功: %2").arg(taskId).arg(success));

        task->setIsUploading(false);

        if (success) {
            // 上传成功，更新状态为待审核
            task->setStatus(TaskStatus::Pending);
            task->setUploadProgress(100);

            Application::instance().logger()->info("TaskManager",
                QString::fromUtf8("任务 %1 文件上传成功").arg(taskId));

            // 显示成功提示
            ToastManager::instance().showSuccess(QString::fromUtf8("上传成功，等待渲染"));
        } else {
            // 上传失败，恢复为草稿状态
            task->setStatus(TaskStatus::Draft);

            Application::instance().logger()->error("TaskManager",
                QString::fromUtf8("任务 %1 文件上传失败").arg(taskId));

            // 显示失败提示
            ToastManager::instance().showError(QString::fromUtf8("上传失败"));
        }

        // 清理上传器
        OSSUploader* uploader = m_uploaders.take(taskId);
        if (uploader) {
            uploader->deleteLater();
        }

        emit taskListUpdated();
    });

    connect(uploader, &OSSUploader::uploadError,
            task, [task](const QString& error) {
        task->setUploadError(error);
        Application::instance().logger()->error("TaskManager",
            QString::fromUtf8("上传错误: %1").arg(error));
    });

    // 开始上传
    uploader->startUpload(filePath, taskId, credentials, config);
}

void TaskManager::pauseTaskUpload(const QString& taskId)
{
    OSSUploader* uploader = m_uploaders.value(taskId, nullptr);
    if (!uploader) {
        Application::instance().logger()->warning("TaskManager",
            QString::fromUtf8("暂停上传失败: 未找到任务 %1 的上传器").arg(taskId));
        return;
    }

    uploader->pause();

    Task* task = getTaskById(taskId);
    if (task) {
        task->setUploadPaused(true);
    }

    Application::instance().logger()->info("TaskManager",
        QString::fromUtf8("已暂停任务 %1 的上传").arg(taskId));
}

void TaskManager::resumeTaskUpload(const QString& taskId)
{
    OSSUploader* uploader = m_uploaders.value(taskId, nullptr);
    if (!uploader) {
        Application::instance().logger()->warning("TaskManager",
            QString::fromUtf8("恢复上传失败: 未找到任务 %1 的上传器").arg(taskId));
        return;
    }

    uploader->resume();

    Task* task = getTaskById(taskId);
    if (task) {
        task->setUploadPaused(false);
    }

    Application::instance().logger()->info("TaskManager",
        QString::fromUtf8("已恢复任务 %1 的上传").arg(taskId));
}

void TaskManager::cancelTaskUpload(const QString& taskId)
{
    OSSUploader* uploader = m_uploaders.take(taskId);
    if (!uploader) {
        Application::instance().logger()->warning("TaskManager",
            QString::fromUtf8("取消上传失败: 未找到任务 %1 的上传器").arg(taskId));
        return;
    }

    uploader->cancel();
    uploader->deleteLater();

    Task* task = getTaskById(taskId);
    if (task) {
        task->setIsUploading(false);
        task->setUploadPaused(false);
        task->setStatus(TaskStatus::Draft);  // 恢复为草稿状态
    }

    Application::instance().logger()->info("TaskManager",
        QString::fromUtf8("已取消任务 %1 的上传").arg(taskId));

    emit taskListUpdated();
}
#endif
