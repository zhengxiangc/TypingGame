/* -------------------------------------------------------------------------
//  文件名      :  spacewaraudio.cpp
//  功能描述    :  Space War SFX/BGM on a worker thread (QSoundEffect / QMediaPlayer).
// -------------------------------------------------------------------------*/

#include "spacewaraudio.h"

#include <QFile>
#include <QMediaPlayer>
#include <QMetaObject>
#include <QAudioOutput>
#include <QSoundEffect>
#include <QThread>
#include <QUrl>

class KCTSpaceWarAudioService::AudioWorker : public QObject
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
    void bindSfx(QSoundEffect* effect, const QString& path);

    QString m_appDir;
    QSoundEffect m_blast;
    QSoundEffect m_planeOut;
    QSoundEffect m_shoot;
    QSoundEffect m_wordOut;
    QSoundEffect m_upgrade;
    QMediaPlayer m_bgmPlayer;
    QAudioOutput m_bgmOutput;

    AudioWorker(const AudioWorker&) = delete;
    AudioWorker& operator=(const AudioWorker&) = delete;
};

void KCTSpaceWarAudioService::AudioWorker::bindSfx(QSoundEffect* effect, const QString& path)
{
    if (QFile::exists(path)) {
        effect->setSource(QUrl::fromLocalFile(path));
        effect->setLoopCount(1);
    } else {
        effect->setSource(QUrl());
    }
}

void KCTSpaceWarAudioService::AudioWorker::startup()
{
    const QString soundsDir = m_appDir + QStringLiteral("/assets/sounds/spacewar");
    const QString musicDir = m_appDir + QStringLiteral("/assets/music/spacewar");

    m_blast.setVolume(0.9f);
    m_planeOut.setVolume(0.85f);
    m_shoot.setVolume(0.9f);
    m_wordOut.setVolume(0.85f);
    m_upgrade.setVolume(0.9f);

    bindSfx(&m_blast, soundsDir + QStringLiteral("/SPACE_BLAST.wav"));
    bindSfx(&m_planeOut, soundsDir + QStringLiteral("/SPACE_PLANEOUT.wav"));
    bindSfx(&m_shoot, soundsDir + QStringLiteral("/SPACE_SHOOT.wav"));
    bindSfx(&m_wordOut, soundsDir + QStringLiteral("/SPACE_WORDOUT.wav"));
    bindSfx(&m_upgrade, soundsDir + QStringLiteral("/UPGRADE.wav"));

    m_bgmPlayer.setAudioOutput(&m_bgmOutput);
    m_bgmOutput.setVolume(0.35f);
    const QString bgmPath = musicDir + QStringLiteral("/SPACE_BG.wav");
    if (QFile::exists(bgmPath)) {
        m_bgmPlayer.setSource(QUrl::fromLocalFile(bgmPath));
    } else {
        m_bgmPlayer.setSource(QUrl());
    }
}

void KCTSpaceWarAudioService::AudioWorker::playSfx(int id)
{
    QSoundEffect* effect = nullptr;
    switch (static_cast<KCTSpaceWarSfxId>(id)) {
    case KCTSpaceWarSfxId::BlastSfxId:
        effect = &m_blast;
        break;
    case KCTSpaceWarSfxId::PlaneOutSfxId:
        effect = &m_planeOut;
        break;
    case KCTSpaceWarSfxId::ShootSfxId:
        effect = &m_shoot;
        break;
    case KCTSpaceWarSfxId::WordOutSfxId:
        effect = &m_wordOut;
        break;
    case KCTSpaceWarSfxId::UpgradeSfxId:
        effect = &m_upgrade;
        break;
    default:
        break;
    }
    if (!effect || effect->source().isEmpty()) {
        return;
    }
    effect->stop();
    effect->play();
}

void KCTSpaceWarAudioService::AudioWorker::startBgm()
{
    if (m_bgmPlayer.source().isEmpty()) {
        return;
    }
    if (m_bgmPlayer.playbackState() == QMediaPlayer::PlayingState) {
        return;
    }
    if (m_bgmPlayer.playbackState() == QMediaPlayer::PausedState) {
        m_bgmPlayer.play();
        return;
    }
    m_bgmPlayer.setLoops(QMediaPlayer::Infinite);
    m_bgmPlayer.play();
}

void KCTSpaceWarAudioService::AudioWorker::pauseBgm()
{
    m_bgmPlayer.pause();
}

void KCTSpaceWarAudioService::AudioWorker::resumeBgm()
{
    startBgm();
}

void KCTSpaceWarAudioService::AudioWorker::stopBgm()
{
    m_bgmPlayer.stop();
}

void KCTSpaceWarAudioService::AudioWorker::shutdown()
{
    m_bgmPlayer.stop();
    m_blast.setSource(QUrl());
    m_planeOut.setSource(QUrl());
    m_shoot.setSource(QUrl());
    m_wordOut.setSource(QUrl());
    m_upgrade.setSource(QUrl());
    m_bgmPlayer.setSource(QUrl());
}

KCTSpaceWarAudioService::KCTSpaceWarAudioService(QString applicationDir)
    : m_appDir(std::move(applicationDir))
{
    m_thread = new QThread();
    m_worker = new AudioWorker(m_appDir);
    m_worker->moveToThread(m_thread);
    QObject::connect(m_thread, &QThread::started, m_worker, &AudioWorker::startup);
    m_thread->start();
}

KCTSpaceWarAudioService::~KCTSpaceWarAudioService()
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

void KCTSpaceWarAudioService::playSfx(KCTSpaceWarSfxId id)
{
    if (!m_worker) {
        return;
    }
    QMetaObject::invokeMethod(m_worker, "playSfx", Qt::QueuedConnection, Q_ARG(int, static_cast<int>(id)));
}

void KCTSpaceWarAudioService::startBgmIfAvailable()
{
    if (!m_worker) {
        return;
    }
    QMetaObject::invokeMethod(m_worker, "startBgm", Qt::QueuedConnection);
}

void KCTSpaceWarAudioService::pauseBgm()
{
    if (!m_worker) {
        return;
    }
    QMetaObject::invokeMethod(m_worker, "pauseBgm", Qt::QueuedConnection);
}

void KCTSpaceWarAudioService::resumeBgm()
{
    if (!m_worker) {
        return;
    }
    QMetaObject::invokeMethod(m_worker, "resumeBgm", Qt::QueuedConnection);
}

void KCTSpaceWarAudioService::stopBgm()
{
    if (!m_worker) {
        return;
    }
    QMetaObject::invokeMethod(m_worker, "stopBgm", Qt::QueuedConnection);
}

#include "spacewaraudio.moc"
