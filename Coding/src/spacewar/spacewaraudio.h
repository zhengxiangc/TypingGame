/* -------------------------------------------------------------------------
//  文件名      :  spacewaraudio.h
//  功能描述    :  Space War SFX/BGM loaded from assets/sounds/spacewar and assets/music/spacewar.
// -------------------------------------------------------------------------*/

#ifndef __CLASSEXAM_SPACEWARAUDIO_H__
#define __CLASSEXAM_SPACEWARAUDIO_H__

#include <QObject>
#include <QString>

#include <memory>

class QThread;

enum class KCTSpaceWarSfxId : int {
    BlastSfxId = 0,
    PlaneOutSfxId = 1,
    ShootSfxId = 2,
    WordOutSfxId = 3,
    UpgradeSfxId = 4,
};

class KCTSpaceWarAudioService
{
public:
    explicit KCTSpaceWarAudioService(QString applicationDir);
    ~KCTSpaceWarAudioService();

    void playSfx(KCTSpaceWarSfxId id);
    void startBgmIfAvailable();
    void pauseBgm();
    void resumeBgm();
    void stopBgm();

private:
    class AudioWorker;
    QString m_appDir;
    QThread* m_thread = nullptr;
    AudioWorker* m_worker = nullptr;

    KCTSpaceWarAudioService(const KCTSpaceWarAudioService&) = delete;
    KCTSpaceWarAudioService& operator=(const KCTSpaceWarAudioService&) = delete;
};

#endif // __CLASSEXAM_SPACEWARAUDIO_H__
