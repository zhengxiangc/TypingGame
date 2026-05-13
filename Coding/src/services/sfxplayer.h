/* -------------------------------------------------------------------------
//  文件名      :  sfxplayer.h
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Declares KCTSfxPlayer (Qt Multimedia sound effects).
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#ifndef __CLASSEXAM_SFXPLAYER_H__
#define __CLASSEXAM_SFXPLAYER_H__

#include "sfxid.h"

#include <QObject>
#include <QSoundEffect>

class KCTSfxPlayer : public QObject
{
    Q_OBJECT

public:
    explicit KCTSfxPlayer(QObject* parent = nullptr);

    void setSoundsDir(const QString& dir);

public slots:
    void play(int id);

private:
    void bind(QSoundEffect* effect, const QString& fileName);

    QString m_dir;
    QSoundEffect m_hit;
    QSoundEffect m_miss;
    QSoundEffect m_level;

    KCTSfxPlayer(const KCTSfxPlayer&) = delete;
    KCTSfxPlayer& operator=(const KCTSfxPlayer&) = delete;
};

#endif // __CLASSEXAM_SFXPLAYER_H__
