#pragma once

#include <QObject>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>

/**
 * @brief 日志管理类
 */
class Logger : public QObject
{
    Q_OBJECT

public:
    enum LogLevel {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3
    };

    explicit Logger(QObject *parent = nullptr);
    ~Logger();

    void initialize();

    void debug(const QString &category, const QString &message);
    void info(const QString &category, const QString &message);
    void warning(const QString &category, const QString &message);
    void error(const QString &category, const QString &message);

    void log(LogLevel level, const QString &category, const QString &message);

private:
    void writeToFile(const QString &message);
    QString levelToString(LogLevel level) const;

    QFile *m_logFile;
    QTextStream *m_stream;
    QMutex m_mutex;
    LogLevel m_minLevel;
};
