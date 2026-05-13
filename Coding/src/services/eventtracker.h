/* -------------------------------------------------------------------------
//  文件名      :  eventtracker.h
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Declares KCTEventTracker (async JSONL event writer).
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#ifndef __CLASSEXAM_EVENTTRACKER_H__
#define __CLASSEXAM_EVENTTRACKER_H__

#include "threadsafequeue.h"

#include <QString>
#include <stop_token>
#include <thread>

class KCTEventTracker
{
public:
    explicit KCTEventTracker(QString baseDir);
    ~KCTEventTracker();

    void trackJsonLine(const QString& jsonLine);

private:
    void runLoop(std::stop_token stopToken);

    QString m_baseDir;
    KCTThreadSafeQueue<QString> m_queue;
    std::jthread m_thread;

    KCTEventTracker(const KCTEventTracker&) = delete;
    KCTEventTracker& operator=(const KCTEventTracker&) = delete;
};

#endif // __CLASSEXAM_EVENTTRACKER_H__
