/* -------------------------------------------------------------------------
//  文件名      :  bgmplayer.cpp
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Loops background music from assets when a file is present.
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#include "bgmplayer.h"

#include <QFile>
#include <QUrl>

KCTBgmPlayer::KCTBgmPlayer(QObject* parent)
    : QObject(parent)
    , m_output(this)
    , m_player(this)
{
    m_player.setAudioOutput(&m_output);
    m_output.setVolume(0.35f);
}

void KCTBgmPlayer::setMusicDir(const QString& dir)
{
    m_dir = dir;
    const QString path = m_dir + QStringLiteral("/bgm.mp3");
    if (QFile::exists(path)) {
        m_player.setSource(QUrl::fromLocalFile(path));
    } else {
        m_player.setSource(QUrl());
    }
}

void KCTBgmPlayer::playLoop()
{
    if (m_player.source().isEmpty()) {
        return;
    }
    if (m_player.playbackState() == QMediaPlayer::PlayingState) {
        return;
    }
    if (m_player.playbackState() == QMediaPlayer::PausedState) {
        m_player.play();
        return;
    }
    m_player.setLoops(QMediaPlayer::Infinite);
    m_player.play();
}

void KCTBgmPlayer::pause()
{
    m_player.pause();
}

void KCTBgmPlayer::stop()
{
    m_player.stop();
}
