/**
 * @file Task.h
 * @brief 渲染任务模型
 */

#ifndef TASK_H
#define TASK_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QJsonObject>
#include <QStringList>

/**
 * @brief 任务状态枚举
 */
enum class TaskStatus {
    Draft = 0,          // 草稿（未提交）
    Uploading = 1,      // 上传中
    Pending = 2,        // 待审核
    Queued = 3,         // 队列中
    Rendering = 4,      // 渲染中
    Paused = 5,         // 已暂停
    Completed = 6,      // 已完成
    Failed = 7,         // 失败
    Cancelled = 8       // 已取消
};

/**
 * @brief 任务优先级
 */
enum class TaskPriority {
    Low = 0,            // 低优先级
    Normal = 1,         // 普通
    High = 2,           // 高优先级
    Urgent = 3          // 紧急
};

/**
 * @brief 渲染任务模型
 *
 * 存储渲染任务的所有信息，包括场景文件、渲染配置、进度等
 */
class Task : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString taskId READ taskId WRITE setTaskId NOTIFY taskIdChanged)
    Q_PROPERTY(QString taskName READ taskName WRITE setTaskName NOTIFY taskNameChanged)
    Q_PROPERTY(QString sceneFile READ sceneFile WRITE setSceneFile NOTIFY sceneFileChanged)
    Q_PROPERTY(TaskStatus status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(int progress READ progress WRITE setProgress NOTIFY progressChanged)
    Q_PROPERTY(TaskPriority priority READ priority WRITE setPriority NOTIFY priorityChanged)

public:
    explicit Task(QObject *parent = nullptr);
    ~Task();

    // Getters
    QString taskId() const { return m_taskId; }
    QString taskName() const { return m_taskName; }
    QString sceneFile() const { return m_sceneFile; }
    QString mayaVersion() const { return m_mayaVersion; }
    QString renderer() const { return m_renderer; }
    TaskStatus status() const { return m_status; }
    TaskPriority priority() const { return m_priority; }
    int progress() const { return m_progress; }
    int startFrame() const { return m_startFrame; }
    int endFrame() const { return m_endFrame; }
    int frameStep() const { return m_frameStep; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    QString outputPath() const { return m_outputPath; }
    QString outputFormat() const { return m_outputFormat; }
    QDateTime createdAt() const { return m_createdAt; }
    QDateTime startedAt() const { return m_startedAt; }
    QDateTime completedAt() const { return m_completedAt; }
    double estimatedCost() const { return m_estimatedCost; }
    double actualCost() const { return m_actualCost; }
    QString errorMessage() const { return m_errorMessage; }
    QStringList renderLogs() const { return m_renderLogs; }

    // 上传相关 Getters
    int uploadProgress() const { return m_uploadProgress; }
    qint64 uploadedBytes() const { return m_uploadedBytes; }
    qint64 totalBytes() const { return m_totalBytes; }
    qint64 uploadSpeed() const { return m_uploadSpeed; }
    bool isUploading() const { return m_isUploading; }
    bool uploadPaused() const { return m_uploadPaused; }
    QString uploadError() const { return m_uploadError; }

    // Setters
    void setTaskId(const QString &taskId);
    void setTaskName(const QString &taskName);
    void setSceneFile(const QString &sceneFile);
    void setMayaVersion(const QString &version);
    void setRenderer(const QString &renderer);
    void setStatus(TaskStatus status);
    void setPriority(TaskPriority priority);
    void setProgress(int progress);
    void setStartFrame(int frame);
    void setEndFrame(int frame);
    void setFrameStep(int step);
    void setWidth(int width);
    void setHeight(int height);
    void setOutputPath(const QString &path);
    void setOutputFormat(const QString &format);
    void setCreatedAt(const QDateTime &time);
    void setStartedAt(const QDateTime &time);
    void setCompletedAt(const QDateTime &time);
    void setEstimatedCost(double cost);
    void setActualCost(double cost);
    void setErrorMessage(const QString &message);
    void addRenderLog(const QString &log);
    void clearRenderLogs();

    // 上传相关 Setters
    void setUploadProgress(int progress);
    void setUploadedBytes(qint64 bytes);
    void setTotalBytes(qint64 bytes);
    void setUploadSpeed(qint64 bytesPerSecond);
    void setIsUploading(bool uploading);
    void setUploadPaused(bool paused);
    void setUploadError(const QString &error);

    // 序列化/反序列化
    QJsonObject toJson() const;
    static Task* fromJson(const QJsonObject &json, QObject *parent = nullptr);

    // 工具方法
    QString statusString() const;
    QString priorityString() const;
    bool canStart() const;
    bool canPause() const;
    bool canResume() const;
    bool canCancel() const;
    int totalFrames() const;
    QString durationString() const;
    void clear();

signals:
    void taskIdChanged();
    void taskNameChanged();
    void sceneFileChanged();
    void statusChanged();
    void progressChanged();
    void priorityChanged();
    void taskDataChanged();
    void renderLogAdded(const QString &log);

    // 上传相关信号
    void uploadProgressChanged(int progress, qint64 uploaded, qint64 total);
    void uploadSpeedChanged(qint64 bytesPerSecond);
    void uploadStatusChanged(bool isUploading, bool isPaused);
    void uploadErrorOccurred(const QString &error);

private:
    QString m_taskId;
    QString m_taskName;
    QString m_sceneFile;
    QString m_mayaVersion;
    QString m_renderer;
    TaskStatus m_status;
    TaskPriority m_priority;
    int m_progress;

    // 渲染参数
    int m_startFrame;
    int m_endFrame;
    int m_frameStep;
    int m_width;
    int m_height;
    QString m_outputPath;
    QString m_outputFormat;

    // 时间信息
    QDateTime m_createdAt;
    QDateTime m_startedAt;
    QDateTime m_completedAt;

    // 费用信息
    double m_estimatedCost;
    double m_actualCost;

    // 错误和日志
    QString m_errorMessage;
    QStringList m_renderLogs;

    // 上传相关
    int m_uploadProgress = 0;           // 上传进度 0-100
    qint64 m_uploadedBytes = 0;         // 已上传字节数
    qint64 m_totalBytes = 0;            // 总字节数
    qint64 m_uploadSpeed = 0;           // 上传速度 (bytes/s)
    bool m_isUploading = false;         // 是否正在上传
    bool m_uploadPaused = false;        // 上传是否暂停
    QString m_uploadError;              // 上传错误信息
};

#endif // TASK_H
