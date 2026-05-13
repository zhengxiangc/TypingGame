/* -------------------------------------------------------------------------
//  文件名      :  eventtracker.cpp
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Background jthread that appends JSON Lines telemetry to telemetry/events.jsonl.
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#include "eventtracker.h"

#include <chrono>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>

KCTEventTracker::KCTEventTracker(QString baseDir)
    : m_baseDir(std::move(baseDir))
    , m_thread([this](std::stop_token st) { runLoop(st); })
{
}

KCTEventTracker::~KCTEventTracker()
{
    m_queue.shutdown();
}

void KCTEventTracker::trackJsonLine(const QString& jsonLine)
{
    m_queue.push(jsonLine);
}

void KCTEventTracker::runLoop(std::stop_token stopToken)
{
    const QString dir = m_baseDir + QStringLiteral("/telemetry");
    QDir().mkpath(dir);

    const QString path = dir + QStringLiteral("/events.jsonl");
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }

    QTextStream stream(&file);

    while (true) {
        std::vector<QString> batch;
        m_queue.popAllWaitFor(batch, std::chrono::milliseconds(250));

        for (QString& line : batch) {
            line = line.trimmed();
            if (line.isEmpty()) {
                continue;
            }
            stream << line << Qt::endl;
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
    for (QString& line : tail) {
        line = line.trimmed();
        if (!line.isEmpty()) {
            stream << line << Qt::endl;
        }
    }
    stream.flush();
    file.flush();
}
