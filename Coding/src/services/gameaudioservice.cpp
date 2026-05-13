/* -------------------------------------------------------------------------
//  文件名      :  gameaudioservice.cpp
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Owns worker thread for SFX/BGM; bridges UI thread calls to audio objects.
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#include "gameaudioservice.h"

#include <QMetaObject>
#include <QThread>

#include "bgmplayer.h"
#include "sfxplayer.h"

// Private nested QObject; declared in the header as private, defined only in this translation unit.
class KCTGameAudioService::AudioWorker : public QObject
{
    Q_OBJECT

public:
    explicit AudioWorker(QString appDir, QObject* parent = nullptr)
        : QObject(parent)
        , m_appDir(std::move(appDir))
    {
    }

public slots:
    void startup();
    void shutdown();
    void playSfx(int id);
    void startBgm();
    void pauseBgm();
    void resumeBgm();
    void stopBgm();

private:
    QString m_appDir;
    KCTSfxPlayer* m_sfx = nullptr;
    KCTBgmPlayer* m_bgm = nullptr;

    AudioWorker(const AudioWorker&) = delete;
    AudioWorker& operator=(const AudioWorker&) = delete;
};

void KCTGameAudioService::AudioWorker::startup()
{
    const QString sounds = m_appDir + QStringLiteral("/assets/sounds");
    const QString music = m_appDir + QStringLiteral("/assets/music");

    m_sfx = new KCTSfxPlayer(this);
    m_sfx->setSoundsDir(sounds);

    m_bgm = new KCTBgmPlayer(this);
    m_bgm->setMusicDir(music);
}

void KCTGameAudioService::AudioWorker::playSfx(int id)
{
    if (m_sfx) {
        m_sfx->play(id);
    }
}

void KCTGameAudioService::AudioWorker::startBgm()
{
    if (m_bgm) {
        m_bgm->playLoop();
    }
}

void KCTGameAudioService::AudioWorker::pauseBgm()
{
    if (m_bgm) {
        m_bgm->pause();
    }
}

void KCTGameAudioService::AudioWorker::resumeBgm()
{
    if (m_bgm) {
        m_bgm->playLoop();
    }
}

void KCTGameAudioService::AudioWorker::stopBgm()
{
    if (m_bgm) {
        m_bgm->stop();
    }
}

void KCTGameAudioService::AudioWorker::shutdown()
{
    if (m_bgm) {
        m_bgm->stop();
    }
    delete m_sfx;
    m_sfx = nullptr;
    delete m_bgm;
    m_bgm = nullptr;
}

KCTGameAudioService::KCTGameAudioService(QString applicationDir)
    : m_appDir(std::move(applicationDir))
{
    m_thread = new QThread();
    m_worker = new AudioWorker(m_appDir);
    m_worker->moveToThread(m_thread);
    QObject::connect(m_thread, &QThread::started, m_worker, &AudioWorker::startup);
    m_thread->start();
}

KCTGameAudioService::~KCTGameAudioService()
{
    if (m_worker && m_thread) {
        QMetaObject::invokeMethod(m_worker, "shutdown", Qt::BlockingQueuedConnection);
        m_thread->quit();
        m_thread->wait(5000);
        delete m_worker;
        m_worker = nullptr;
    }
    delete m_thread;
    m_thread = nullptr;
}

void KCTGameAudioService::playSfx(KCTSfxId id)
{
    if (!m_worker) {
        return;
    }
    QMetaObject::invokeMethod(
        m_worker,
        "playSfx",
        Qt::QueuedConnection,
        Q_ARG(int, static_cast<int>(id)));
}

void KCTGameAudioService::startBgmIfAvailable()
{
    if (!m_worker) {
        return;
    }
    QMetaObject::invokeMethod(m_worker, "startBgm", Qt::QueuedConnection);
}

void KCTGameAudioService::pauseBgm()
{
    if (!m_worker) {
        return;
    }
    QMetaObject::invokeMethod(m_worker, "pauseBgm", Qt::QueuedConnection);
}

void KCTGameAudioService::resumeBgm()
{
    if (!m_worker) {
        return;
    }
    QMetaObject::invokeMethod(m_worker, "resumeBgm", Qt::QueuedConnection);
}

void KCTGameAudioService::stopBgm()
{
    if (!m_worker) {
        return;
    }
    QMetaObject::invokeMethod(m_worker, "stopBgm", Qt::QueuedConnection);
}

#include "gameaudioservice.moc"
