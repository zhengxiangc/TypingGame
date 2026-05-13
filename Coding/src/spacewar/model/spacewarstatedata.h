/* -------------------------------------------------------------------------
//  文件名      :  spacewarstatedata.h
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Mutable Space War session state including flying reward word progress.
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#ifndef __CLASSEXAM_SPACEWARSTATEDATA_H__
#define __CLASSEXAM_SPACEWARSTATEDATA_H__

#include "spacewarentities.h"

#include <QChar>
#include <QList>
#include <QString>
#include <QtGlobal>

struct KCTFlyingRewardWord
{
    QString text;
    double x = 0;
    double y = 0;
    double velocityXPixelsPerSec = 115.0;
    bool active = false;
};

struct KCTSpaceWarStateData
{
    enum GameState {
        IdleGameState,
        PlayingGameState,
        PausedGameState,
        EndGameState
    };

    QList<KCTEnemyEntity> enemies;
    QList<QChar> usedLetters;
    QList<KCTBulletEntity> bullets;
    QList<KCTExplosionEntity> explosions;

    double playerX = 400;
    double playerVelX = 2.8;

    int score = 0;
    int health = 18;
    int upgradeTier = 0;
    qint64 playingElapsedMs = 0;

    GameState state = IdleGameState;
    bool gameOver = false;

    int nextEnemyId = 1;

    KCTFlyingRewardWord flyingRewardWord;
    int rewardTypingIndex = 0;
};

#endif // __CLASSEXAM_SPACEWARSTATEDATA_H__
