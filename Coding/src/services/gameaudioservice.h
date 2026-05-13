/* -------------------------------------------------------------------------
//  文件名      :  gameaudioservice.h
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Declares KCTGameAudioService; private nested AudioWorker lives in the .cpp file.
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#ifndef __CLASSEXAM_GAMEAUDIOSERVICE_H__
#define __CLASSEXAM_GAMEAUDIOSERVICE_H__

#include "sfxid.h"

#include <QString>

class QThread;

class KCTGameAudioService
{
public:
    explicit KCTGameAudioService(QString applicationDir);
    ~KCTGameAudioService();

    void playSfx(KCTSfxId id);
    void startBgmIfAvailable();
    void pauseBgm();
    void resumeBgm();
    void stopBgm();

private:
    class AudioWorker;

    QString m_appDir;
    QThread* m_thread = nullptr;
    AudioWorker* m_worker = nullptr;

    KCTGameAudioService(const KCTGameAudioService&) = delete;
    KCTGameAudioService& operator=(const KCTGameAudioService&) = delete;
};

#endif // __CLASSEXAM_GAMEAUDIOSERVICE_H__
