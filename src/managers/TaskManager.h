/**
 * @file TaskManager.h
 * @brief 任务管理器
 */

#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <QObject>
#include <QList>
#include <QMap>
#include "../models/Task.h"
#include "../models/RenderConfig.h"
#include "../network/ApiService.h"
#include "../network/WebSocketClient.h"
#include "../network/FileUploader.h"

/**
 * @brief 任务管理器
 *
 * 管理渲染任务列表、任务操作、实时状态更新等
 * 使用单例模式
 */
class TaskManager : public QObject
{
    Q_OBJECT

public:
    static TaskManager& instance();

    // 禁用拷贝构造和赋值
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;

    /**
     * @brief 初始化任务管理器
     */
    void initialize();

    /**
     * @brief 清理任务管理器
     */
    void cleanup();

    /**
     * @brief 获取所有任务列表
     */
    QList<Task*> getAllTasks() const { return m_tasks; }

    /**
     * @brief 根据状态筛选任务
     */
    QList<Task*> getTasksByStatus(TaskStatus status) const;

    /**
     * @brief 根据优先级筛选任务
     */
    QList<Task*> getTasksByPriority(TaskPriority priority) const;

    /**
     * @brief 根据 ID 获取任务
     */
    Task* getTaskById(const QString& taskId) const;

    /**
     * @brief 获取任务数量
     */
    int getTaskCount() const { return m_tasks.size(); }

    /**
     * @brief 获取指定状态的任务数量
     */
    int getTaskCountByStatus(TaskStatus status) const;

    /**
     * @brief 从服务器刷新任务列表
     */
    void refreshTaskList();

    /**
     * @brief 创建新任务
     * @param taskName 任务名称
     * @param sceneFile 场景文件路径
     * @param config 渲染配置
     */
    void createTask(const QString& taskName, const QString& sceneFile, RenderConfig* config);

    /**
     * @brief 提交任务到服务器
     * @param task 任务对象
     */
    void submitTask(Task* task);

    /**
     * @brief 开始任务
     * @param taskId 任务ID
     */
    void startTask(const QString& taskId);

    /**
     * @brief 暂停任务
     * @param taskId 任务ID
     */
    void pauseTask(const QString& taskId);

    /**
     * @brief 恢复任务
     * @param taskId 任务ID
     */
    void resumeTask(const QString& taskId);

    /**
     * @brief 取消任务
     * @param taskId 任务ID
     */
    void cancelTask(const QString& taskId);

    /**
     * @brief 删除任务
     * @param taskId 任务ID
     */
    void deleteTask(const QString& taskId);

    /**
     * @brief 获取任务详情
     * @param taskId 任务ID
     */
    void fetchTaskDetails(const QString& taskId);

    /**
     * @brief 下载任务结果
     * @param taskId 任务ID
     * @param savePath 保存路径
     */
    void downloadTaskResults(const QString& taskId, const QString& savePath);

    /**
     * @brief 清空所有任务（本地）
     */
    void clearAllTasks();

    /**
     * @brief 保存任务列表到本地
     */
    void saveTasksToLocal();

    /**
     * @brief 从本地加载任务列表
     */
    void loadTasksFromLocal();

signals:
    /**
     * @brief 任务列表更新信号
     */
    void taskListUpdated();

    /**
     * @brief 任务添加信号
     */
    void taskAdded(Task* task);

    /**
     * @brief 任务删除信号
     */
    void taskRemoved(const QString& taskId);

    /**
     * @brief 任务状态更新信号
     */
    void taskStatusUpdated(const QString& taskId, TaskStatus status);

    /**
     * @brief 任务进度更新信号
     */
    void taskProgressUpdated(const QString& taskId, int progress);

    /**
     * @brief 任务创建成功信号
     */
    void taskCreated(Task* task);

    /**
     * @brief 任务创建失败信号
     */
    void taskCreationFailed(const QString& error);

    /**
     * @brief 任务提交成功信号
     */
    void taskSubmitted(const QString& taskId);

    /**
     * @brief 任务提交失败信号
     */
    void taskSubmissionFailed(const QString& taskId, const QString& error);

    /**
     * @brief 任务操作成功信号
     */
    void taskOperationSuccess(const QString& taskId, const QString& operation);

    /**
     * @brief 任务操作失败信号
     */
    void taskOperationFailed(const QString& taskId, const QString& operation, const QString& error);

    /**
     * @brief 任务详情获取成功信号
     */
    void taskDetailsFetched(Task* task);

    /**
     * @brief 文件上传进度信号
     * @param taskId 任务ID（本地临时ID）
     * @param progress 上传进度 (0-100)
     * @param uploadedBytes 已上传字节数
     * @param totalBytes 总字节数
     */
    void fileUploadProgress(const QString& taskId, int progress, qint64 uploadedBytes, qint64 totalBytes);

    /**
     * @brief 文件上传失败信号
     * @param taskId 任务ID（本地临时ID）
     * @param error 错误信息
     */
    void fileUploadFailed(const QString& taskId, const QString& error);

private:
    explicit TaskManager(QObject *parent = nullptr);
    ~TaskManager();

    /**
     * @brief 添加任务到列表
     */
    void addTask(Task* task);

    /**
     * @brief 从列表中移除任务
     */
    void removeTask(const QString& taskId);

    /**
     * @brief 更新任务信息
     */
    void updateTask(const QString& taskId, const QJsonObject& taskData);

    /**
     * @brief 连接 WebSocket 信号
     */
    void connectWebSocketSignals();

    /**
     * @brief 处理任务状态更新（来自 WebSocket）
     */
    void handleTaskStatusUpdate(const QString& taskId, int status);

    /**
     * @brief 处理任务进度更新（来自 WebSocket）
     */
    void handleTaskProgressUpdate(const QString& taskId, int progress);

    /**
     * @brief 按创建时间排序任务
     */
    void sortTasks();

private:
    WebSocketClient* m_wsClient;
    FileUploader* m_fileUploader;

    QList<Task*> m_tasks;
    QMap<QString, Task*> m_taskMap;  // taskId -> Task* 快速查找
    QMap<QString, Task*> m_uploadingTasks;  // 正在上传的任务（本地临时ID -> Task*）

    bool m_isInitialized;
};

#endif // TASKMANAGER_H
