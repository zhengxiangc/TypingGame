/* -------------------------------------------------------------------------
//  文件名      :  asynclogger.h
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Declares KCTAsyncLogger (async file logging facade).
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#ifndef __CLASSEXAM_ASYNCLOGGER_H__
#define __CLASSEXAM_ASYNCLOGGER_H__

#include "threadsafequeue.h"

#include <QString>
#include <stop_token>
#include <thread>

class KCTAsyncLogger
{
public:
    explicit KCTAsyncLogger(QString baseDir);
    ~KCTAsyncLogger();

    void log(const QString& message);

private:
    void runLoop(std::stop_token stopToken);

    QString m_baseDir;
    KCTThreadSafeQueue<QString> m_queue;
    std::jthread m_thread;

    KCTAsyncLogger(const KCTAsyncLogger&) = delete;
    KCTAsyncLogger& operator=(const KCTAsyncLogger&) = delete;
};

#endif // __CLASSEXAM_ASYNCLOGGER_H__
