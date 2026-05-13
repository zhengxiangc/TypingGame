/* -------------------------------------------------------------------------
//  文件名      :  sfxplayer.cpp
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Loads WAV SFX from disk and plays them on the worker thread via QSoundEffect.
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#include "sfxplayer.h"

#include <QFile>
#include <QUrl>

KCTSfxPlayer::KCTSfxPlayer(QObject* parent)
    : QObject(parent)
    , m_hit(this)
    , m_miss(this)
    , m_level(this)
{
    m_hit.setVolume(0.9f);
    m_miss.setVolume(0.9f);
    m_level.setVolume(0.85f);
}

void KCTSfxPlayer::setSoundsDir(const QString& dir)
{
    m_dir = dir;
    bind(&m_hit, QStringLiteral("hit.wav"));
    bind(&m_miss, QStringLiteral("miss.wav"));
    bind(&m_level, QStringLiteral("level.wav"));
}

void KCTSfxPlayer::bind(QSoundEffect* effect, const QString& fileName)
{
    const QString path = m_dir + QLatin1Char('/') + fileName;
    if (QFile::exists(path)) {
        effect->setSource(QUrl::fromLocalFile(path));
        effect->setLoopCount(1);
    } else {
        effect->setSource(QUrl());
    }
}

void KCTSfxPlayer::play(int id)
{
    const KCTSfxId sfx = static_cast<KCTSfxId>(id);
    QSoundEffect* pEffect = nullptr;
    switch (sfx) {
    case KCTSfxId::HitSfxId:
        pEffect = &m_hit;
        break;
    case KCTSfxId::MissSfxId:
        pEffect = &m_miss;
        break;
    case KCTSfxId::LevelCompleteSfxId:
        pEffect = &m_level;
        break;
    default:
        pEffect = nullptr;
        break;
    }
    if (!pEffect || pEffect->source().isEmpty()) {
        return;
    }
    pEffect->stop();
    pEffect->play();
}
