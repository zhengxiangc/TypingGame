/* -------------------------------------------------------------------------
//  文件名      :  bgmplayer.h
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Declares KCTBgmPlayer (QMediaPlayer + QAudioOutput loop helper).
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#ifndef __CLASSEXAM_BGMPLAYER_H__
#define __CLASSEXAM_BGMPLAYER_H__

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QObject>

class KCTBgmPlayer : public QObject
{
    Q_OBJECT

public:
    explicit KCTBgmPlayer(QObject* parent = nullptr);

    void setMusicDir(const QString& dir);

public slots:
    void playLoop();
    void pause();
    void stop();

private:
    QString m_dir;
    QAudioOutput m_output;
    QMediaPlayer m_player;

    KCTBgmPlayer(const KCTBgmPlayer&) = delete;
    KCTBgmPlayer& operator=(const KCTBgmPlayer&) = delete;
};

#endif // __CLASSEXAM_BGMPLAYER_H__
