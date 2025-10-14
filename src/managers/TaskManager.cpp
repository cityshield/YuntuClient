/**
 * @file TaskManager.cpp
 * @brief 任务管理器实现
 */

#include "TaskManager.h"
#include "../core/Logger.h"
#include "../core/Application.h"
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
    , m_isInitialized(false)
{
    // 创建文件上传器
    m_fileUploader = new FileUploader(this);
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

    // 从本地加载任务列表
    loadTasksFromLocal();

    // 连接 WebSocket 信号（如果已连接）
    // WebSocket 客户端由外部管理，这里只是连接信号
    // 实际使用时需要在适当的时候设置 WebSocket 客户端

    m_isInitialized = true;
}

void TaskManager::cleanup()
{
    Application::instance().logger()->info("TaskManager", QString::fromUtf8("清理任务管理器"));

    // 保存任务到本地
    saveTasksToLocal();

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

    connect(m_wsClient, &WebSocketClient::taskProgressUpdated,
            this, &TaskManager::handleTaskProgressUpdate);

    // 可以添加更多 WebSocket 信号连接
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
        task->setProgress(progress);
        emit taskProgressUpdated(taskId, progress);
    }
}

void TaskManager::sortTasks()
{
    // 按创建时间降序排序（最新的在前面）
    std::sort(m_tasks.begin(), m_tasks.end(), [](Task* a, Task* b) {
        return a->createdAt() > b->createdAt();
    });
}
