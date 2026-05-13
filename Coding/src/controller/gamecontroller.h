/* -------------------------------------------------------------------------
//  文件名      :  gamecontroller.h
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Declares KCTGameController (Save Apples game logic, no UI).
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#ifndef __CLASSEXAM_GAMECONTROLLER_H__
#define __CLASSEXAM_GAMECONTROLLER_H__

#include "../model/gameconfig.h"
#include "../model/gamestatedata.h"

class KCTGameController
{
public:
    KCTGameController();

    void startGame(const KCTGameConfig& config);
    void ensureMinimumApples(int count, const KCTGameConfig& config, int gameWidth, int appleSize);
    void setState(KCTGameStateData::GameState newState);
    KCTGameStateData::GameState state() const;

    void tick(const KCTGameConfig& config, int gameWidth, int appleSize, int failLine);
    bool handleLetterInput(QChar key, const KCTGameConfig& config, int gameWidth, int appleSize);
    void prepareNextLevel(const KCTGameConfig& config, int gameWidth, int appleSize);

    const KCTGameStateData& data() const;
    KCTGameStateData& data();

private:
    void tryCreateApple(const KCTGameConfig& config, int gameWidth, int appleSize);
    void removeAppleAt(int index);
    void checkLevelComplete(const KCTGameConfig& config);

    KCTGameStateData m_data;

    KCTGameController(const KCTGameController&) = delete;
    KCTGameController& operator=(const KCTGameController&) = delete;
};

#endif // __CLASSEXAM_GAMECONTROLLER_H__
