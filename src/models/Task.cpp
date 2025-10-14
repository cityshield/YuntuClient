/**
 * @file Task.cpp
 * @brief 渲染任务模型实现
 */

#include "Task.h"
#include <QtMath>

Task::Task(QObject *parent)
    : QObject(parent)
    , m_status(TaskStatus::Draft)
    , m_priority(TaskPriority::Normal)
    , m_progress(0)
    , m_startFrame(1)
    , m_endFrame(1)
    , m_frameStep(1)
    , m_width(1920)
    , m_height(1080)
    , m_outputFormat("png")
    , m_estimatedCost(0.0)
    , m_actualCost(0.0)
{
}

Task::~Task()
{
}

void Task::setTaskId(const QString &taskId)
{
    if (m_taskId != taskId) {
        m_taskId = taskId;
        emit taskIdChanged();
        emit taskDataChanged();
    }
}

void Task::setTaskName(const QString &taskName)
{
    if (m_taskName != taskName) {
        m_taskName = taskName;
        emit taskNameChanged();
        emit taskDataChanged();
    }
}

void Task::setSceneFile(const QString &sceneFile)
{
    if (m_sceneFile != sceneFile) {
        m_sceneFile = sceneFile;
        emit sceneFileChanged();
        emit taskDataChanged();
    }
}

void Task::setMayaVersion(const QString &version)
{
    if (m_mayaVersion != version) {
        m_mayaVersion = version;
        emit taskDataChanged();
    }
}

void Task::setRenderer(const QString &renderer)
{
    if (m_renderer != renderer) {
        m_renderer = renderer;
        emit taskDataChanged();
    }
}

void Task::setStatus(TaskStatus status)
{
    if (m_status != status) {
        m_status = status;
        emit statusChanged();
        emit taskDataChanged();

        // 自动设置时间
        if (status == TaskStatus::Rendering && !m_startedAt.isValid()) {
            setStartedAt(QDateTime::currentDateTime());
        } else if ((status == TaskStatus::Completed || status == TaskStatus::Failed || status == TaskStatus::Cancelled)
                   && !m_completedAt.isValid()) {
            setCompletedAt(QDateTime::currentDateTime());
        }
    }
}

void Task::setPriority(TaskPriority priority)
{
    if (m_priority != priority) {
        m_priority = priority;
        emit priorityChanged();
        emit taskDataChanged();
    }
}

void Task::setProgress(int progress)
{
    progress = qBound(0, progress, 100);
    if (m_progress != progress) {
        m_progress = progress;
        emit progressChanged();
        emit taskDataChanged();
    }
}

void Task::setStartFrame(int frame)
{
    if (m_startFrame != frame) {
        m_startFrame = frame;
        emit taskDataChanged();
    }
}

void Task::setEndFrame(int frame)
{
    if (m_endFrame != frame) {
        m_endFrame = frame;
        emit taskDataChanged();
    }
}

void Task::setFrameStep(int step)
{
    if (m_frameStep != step && step > 0) {
        m_frameStep = step;
        emit taskDataChanged();
    }
}

void Task::setWidth(int width)
{
    if (m_width != width) {
        m_width = width;
        emit taskDataChanged();
    }
}

void Task::setHeight(int height)
{
    if (m_height != height) {
        m_height = height;
        emit taskDataChanged();
    }
}

void Task::setOutputPath(const QString &path)
{
    if (m_outputPath != path) {
        m_outputPath = path;
        emit taskDataChanged();
    }
}

void Task::setOutputFormat(const QString &format)
{
    if (m_outputFormat != format) {
        m_outputFormat = format;
        emit taskDataChanged();
    }
}

void Task::setCreatedAt(const QDateTime &time)
{
    m_createdAt = time;
}

void Task::setStartedAt(const QDateTime &time)
{
    m_startedAt = time;
}

void Task::setCompletedAt(const QDateTime &time)
{
    m_completedAt = time;
}

void Task::setEstimatedCost(double cost)
{
    if (qAbs(m_estimatedCost - cost) > 0.01) {
        m_estimatedCost = cost;
        emit taskDataChanged();
    }
}

void Task::setActualCost(double cost)
{
    if (qAbs(m_actualCost - cost) > 0.01) {
        m_actualCost = cost;
        emit taskDataChanged();
    }
}

void Task::setErrorMessage(const QString &message)
{
    if (m_errorMessage != message) {
        m_errorMessage = message;
        emit taskDataChanged();
    }
}

void Task::addRenderLog(const QString &log)
{
    m_renderLogs.append(log);
    emit renderLogAdded(log);
}

void Task::clearRenderLogs()
{
    m_renderLogs.clear();
}

QJsonObject Task::toJson() const
{
    QJsonObject json;
    json["taskId"] = m_taskId;
    json["taskName"] = m_taskName;
    json["sceneFile"] = m_sceneFile;
    json["mayaVersion"] = m_mayaVersion;
    json["renderer"] = m_renderer;
    json["status"] = static_cast<int>(m_status);
    json["priority"] = static_cast<int>(m_priority);
    json["progress"] = m_progress;
    json["startFrame"] = m_startFrame;
    json["endFrame"] = m_endFrame;
    json["frameStep"] = m_frameStep;
    json["width"] = m_width;
    json["height"] = m_height;
    json["outputPath"] = m_outputPath;
    json["outputFormat"] = m_outputFormat;
    json["createdAt"] = m_createdAt.toString(Qt::ISODate);
    json["startedAt"] = m_startedAt.toString(Qt::ISODate);
    json["completedAt"] = m_completedAt.toString(Qt::ISODate);
    json["estimatedCost"] = m_estimatedCost;
    json["actualCost"] = m_actualCost;
    json["errorMessage"] = m_errorMessage;
    return json;
}

Task* Task::fromJson(const QJsonObject &json, QObject *parent)
{
    Task *task = new Task(parent);

    task->setTaskId(json["taskId"].toString());
    task->setTaskName(json["taskName"].toString());
    task->setSceneFile(json["sceneFile"].toString());
    task->setMayaVersion(json["mayaVersion"].toString());
    task->setRenderer(json["renderer"].toString());
    task->setStatus(static_cast<TaskStatus>(json["status"].toInt()));
    task->setPriority(static_cast<TaskPriority>(json["priority"].toInt()));
    task->setProgress(json["progress"].toInt());
    task->setStartFrame(json["startFrame"].toInt());
    task->setEndFrame(json["endFrame"].toInt());
    task->setFrameStep(json["frameStep"].toInt());
    task->setWidth(json["width"].toInt());
    task->setHeight(json["height"].toInt());
    task->setOutputPath(json["outputPath"].toString());
    task->setOutputFormat(json["outputFormat"].toString());

    QString createdAtStr = json["createdAt"].toString();
    if (!createdAtStr.isEmpty()) {
        task->setCreatedAt(QDateTime::fromString(createdAtStr, Qt::ISODate));
    }

    QString startedAtStr = json["startedAt"].toString();
    if (!startedAtStr.isEmpty()) {
        task->setStartedAt(QDateTime::fromString(startedAtStr, Qt::ISODate));
    }

    QString completedAtStr = json["completedAt"].toString();
    if (!completedAtStr.isEmpty()) {
        task->setCompletedAt(QDateTime::fromString(completedAtStr, Qt::ISODate));
    }

    task->setEstimatedCost(json["estimatedCost"].toDouble());
    task->setActualCost(json["actualCost"].toDouble());
    task->setErrorMessage(json["errorMessage"].toString());

    return task;
}

QString Task::statusString() const
{
    switch (m_status) {
        case TaskStatus::Draft:
            return QString::fromUtf8("草稿");
        case TaskStatus::Uploading:
            return QString::fromUtf8("上传中");
        case TaskStatus::Pending:
            return QString::fromUtf8("待审核");
        case TaskStatus::Queued:
            return QString::fromUtf8("队列中");
        case TaskStatus::Rendering:
            return QString::fromUtf8("渲染中");
        case TaskStatus::Paused:
            return QString::fromUtf8("已暂停");
        case TaskStatus::Completed:
            return QString::fromUtf8("已完成");
        case TaskStatus::Failed:
            return QString::fromUtf8("失败");
        case TaskStatus::Cancelled:
            return QString::fromUtf8("已取消");
        default:
            return QString::fromUtf8("未知");
    }
}

QString Task::priorityString() const
{
    switch (m_priority) {
        case TaskPriority::Low:
            return QString::fromUtf8("低");
        case TaskPriority::Normal:
            return QString::fromUtf8("普通");
        case TaskPriority::High:
            return QString::fromUtf8("高");
        case TaskPriority::Urgent:
            return QString::fromUtf8("紧急");
        default:
            return QString::fromUtf8("未知");
    }
}

bool Task::canStart() const
{
    return m_status == TaskStatus::Draft || m_status == TaskStatus::Pending;
}

bool Task::canPause() const
{
    return m_status == TaskStatus::Rendering || m_status == TaskStatus::Queued;
}

bool Task::canResume() const
{
    return m_status == TaskStatus::Paused;
}

bool Task::canCancel() const
{
    return m_status != TaskStatus::Completed &&
           m_status != TaskStatus::Failed &&
           m_status != TaskStatus::Cancelled;
}

int Task::totalFrames() const
{
    if (m_endFrame >= m_startFrame && m_frameStep > 0) {
        return (m_endFrame - m_startFrame) / m_frameStep + 1;
    }
    return 0;
}

QString Task::durationString() const
{
    if (!m_startedAt.isValid()) {
        return QString::fromUtf8("未开始");
    }

    QDateTime endTime = m_completedAt.isValid() ? m_completedAt : QDateTime::currentDateTime();
    qint64 seconds = m_startedAt.secsTo(endTime);

    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;

    if (hours > 0) {
        return QString("%1小时%2分钟").arg(hours).arg(minutes);
    } else if (minutes > 0) {
        return QString("%1分钟%2秒").arg(minutes).arg(secs);
    } else {
        return QString("%1秒").arg(secs);
    }
}

void Task::clear()
{
    setTaskId(QString());
    setTaskName(QString());
    setSceneFile(QString());
    setMayaVersion(QString());
    setRenderer(QString());
    setStatus(TaskStatus::Draft);
    setPriority(TaskPriority::Normal);
    setProgress(0);
    setStartFrame(1);
    setEndFrame(1);
    setFrameStep(1);
    setWidth(1920);
    setHeight(1080);
    setOutputPath(QString());
    setOutputFormat("png");
    setEstimatedCost(0.0);
    setActualCost(0.0);
    setErrorMessage(QString());
    clearRenderLogs();
    m_createdAt = QDateTime();
    m_startedAt = QDateTime();
    m_completedAt = QDateTime();
}
