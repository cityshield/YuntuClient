#include "Logger.h"
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QStringConverter>
#include <QSysInfo>
#include <QCoreApplication>

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
    m_logPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
    QDir().mkpath(m_logPath);

    QString logFileName = QDateTime::currentDateTime().toString("yyyy-MM-dd") + ".log";
    QString logFilePath = m_logPath + "/" + logFileName;

    m_logFile = new QFile(logFilePath);
    if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_stream = new QTextStream(m_logFile);
        m_stream->setEncoding(QStringConverter::Utf8);

        // 记录启动分隔符
        writeToFile("=".repeated(80));
        writeToFile(QString("Application Started: %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")));
        writeToFile("=".repeated(80));
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

void Logger::logCrash(const QString &crashMessage)
{
    QMutexLocker locker(&m_mutex);

    if (m_stream) {
        *m_stream << "\n";
        *m_stream << "!" << QString("!").repeated(78) << "!\n";
        *m_stream << "! CRASH DETECTED\n";
        *m_stream << "!" << QString("!").repeated(78) << "!\n";
        *m_stream << "Time: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
        *m_stream << "Message: " << crashMessage << "\n";
        *m_stream << "\nStack Trace:\n";
        *m_stream << getStackTrace() << "\n";
        *m_stream << "!" << QString("!").repeated(78) << "!\n";
        m_stream->flush();
    }

    // 同时输出到控制台
    qCritical().noquote() << "CRASH:" << crashMessage;
}

QStringList Logger::getAllLogFiles() const
{
    QDir logDir(m_logPath);
    if (!logDir.exists()) {
        return QStringList();
    }

    QStringList filters;
    filters << "*.log";

    QFileInfoList fileList = logDir.entryInfoList(filters, QDir::Files, QDir::Time);
    QStringList paths;

    for (const QFileInfo &fileInfo : fileList) {
        paths.append(fileInfo.absoluteFilePath());
    }

    return paths;
}

void Logger::logSystemInfo()
{
    QMutexLocker locker(&m_mutex);

    if (m_stream) {
        *m_stream << "\n";
        *m_stream << "System Information:\n";
        *m_stream << "-" << QString("-").repeated(78) << "\n";
        *m_stream << "Application: " << QCoreApplication::applicationName() << "\n";
        *m_stream << "Version: " << QCoreApplication::applicationVersion() << "\n";
        *m_stream << "OS: " << QSysInfo::prettyProductName() << "\n";
        *m_stream << "Kernel: " << QSysInfo::kernelType() << " " << QSysInfo::kernelVersion() << "\n";
        *m_stream << "CPU Architecture: " << QSysInfo::currentCpuArchitecture() << "\n";
        *m_stream << "Build ABI: " << QSysInfo::buildAbi() << "\n";
        *m_stream << "-" << QString("-").repeated(78) << "\n";
        m_stream->flush();
    }
}

QString Logger::getStackTrace() const
{
    // 简化的堆栈跟踪（Qt 不提供跨平台的堆栈跟踪）
    // 在 Windows 上需要使用 DbgHelp 库，macOS 使用 backtrace
    // 这里返回基本信息
    return QString("Stack trace not available on this platform\n") +
           QString("Last known location: ") + __FILE__ + ":" + QString::number(__LINE__);
}
