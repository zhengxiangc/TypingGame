/* -------------------------------------------------------------------------
//  文件名      :  asynclogger.cpp
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Background jthread that drains log lines to a daily log file under logs/.
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#include "asynclogger.h"

#include <chrono>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>

KCTAsyncLogger::KCTAsyncLogger(QString baseDir)
    : m_baseDir(std::move(baseDir))
    , m_thread([this](std::stop_token st) { runLoop(st); })
{
}

KCTAsyncLogger::~KCTAsyncLogger()
{
    m_queue.shutdown();
}

void KCTAsyncLogger::log(const QString& message)
{
    m_queue.push(message);
}

void KCTAsyncLogger::runLoop(std::stop_token stopToken)
{
    const QString logDir = m_baseDir + QStringLiteral("/logs");
    QDir().mkpath(logDir);

    const QString fileName = QStringLiteral("typinggame_%1.log")
                                 .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd")));
    const QString path = logDir + QLatin1Char('/') + fileName;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }

    QTextStream stream(&file);

    while (true) {
        std::vector<QString> batch;
        m_queue.popAllWaitFor(batch, std::chrono::milliseconds(250));

        for (const QString& line : batch) {
            const QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
            stream << timestamp << QLatin1Char(' ') << line << Qt::endl;
        }
        if (!batch.empty()) {
            stream.flush();
            file.flush();
        }

        const bool idle = batch.empty() && m_queue.empty();
        if (idle && m_queue.isShutdown()) {
            break;
        }
        if (idle && stopToken.stop_requested()) {
            break;
        }
    }

    std::vector<QString> tail;
    m_queue.drainAll(tail);
    for (const QString& line : tail) {
        const QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
        stream << timestamp << QLatin1Char(' ') << line << Qt::endl;
    }
    stream.flush();
    file.flush();
}
