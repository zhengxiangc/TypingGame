/* -------------------------------------------------------------------------
//  文件名      :  gamestatedata.h
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Mutable Save Apples session state (apples, score, level machine).
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#ifndef __CLASSEXAM_GAMESTATEDATA_H__
#define __CLASSEXAM_GAMESTATEDATA_H__

#include "appleentity.h"

#include <QChar>
#include <QList>

struct KCTGameStateData
{
    enum GameState {
        IdleGameState,
        PlayingGameState,
        PausedGameState,
        EndGameState
    };

    QList<KCTAppleEntity> apples;
    QList<QChar> usedLetters;
    int score = 0;
    int failCount = 0;
    int level = 1;
    int speed = 2;
    int targetScore = 5;
    bool gameOver = false;
    bool levelComplete = false;
    GameState state = IdleGameState;
};

#endif // __CLASSEXAM_GAMESTATEDATA_H__
