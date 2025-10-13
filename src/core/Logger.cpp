#include "Logger.h"
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QStringConverter>

Logger::Logger(QObject *parent)
    : QObject(parent)
    , m_logFile(nullptr)
    , m_stream(nullptr)
    , m_minLevel(Debug)
{
}

Logger::~Logger()
{
    if (m_stream) {
        m_stream->flush();
        delete m_stream;
    }
    if (m_logFile) {
        m_logFile->close();
        delete m_logFile;
    }
}

void Logger::initialize()
{
    QString logPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    QDir().mkpath(logPath);

    QString logFileName = QDateTime::currentDateTime().toString("yyyy-MM-dd") + ".log";
    QString logFilePath = logPath + "/" + logFileName;

    m_logFile = new QFile(logFilePath);
    if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_stream = new QTextStream(m_logFile);
        m_stream->setEncoding(QStringConverter::Utf8);
    }
}

void Logger::debug(const QString &category, const QString &message)
{
    log(Debug, category, message);
}

void Logger::info(const QString &category, const QString &message)
{
    log(Info, category, message);
}

void Logger::warning(const QString &category, const QString &message)
{
    log(Warning, category, message);
}

void Logger::error(const QString &category, const QString &message)
{
    log(Error, category, message);
}

void Logger::log(LogLevel level, const QString &category, const QString &message)
{
    if (level < m_minLevel) {
        return;
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString levelStr = levelToString(level);
    QString logMessage = QString("[%1] [%2] [%3] %4")
                            .arg(timestamp)
                            .arg(levelStr)
                            .arg(category)
                            .arg(message);

    // 输出到控制台
    qDebug().noquote() << logMessage;

    // 写入文件
    writeToFile(logMessage);
}

void Logger::writeToFile(const QString &message)
{
    QMutexLocker locker(&m_mutex);

    if (m_stream) {
        *m_stream << message << "\n";
        m_stream->flush();
    }
}

QString Logger::levelToString(LogLevel level) const
{
    switch (level) {
        case Debug:   return "DEBUG";
        case Info:    return "INFO ";
        case Warning: return "WARN ";
        case Error:   return "ERROR";
        default:      return "UNKN ";
    }
}
