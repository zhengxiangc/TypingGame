/* -------------------------------------------------------------------------
//  文件名      :  spacewarconfig.h
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  User-tunable Space War parameters (QSettings-backed in the view).
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#ifndef __CLASSEXAM_SPACEWARCONFIG_H__
#define __CLASSEXAM_SPACEWARCONFIG_H__

struct KCTSpaceWarConfig
{
    int maxEnemies = 5;
    int enemySpeedLevel = 5;
    int upgradeIntervalSec = 10;
    bool bonusMode = false;
    bool rewardMode = false;
    int spawnRatePercent = 6;
};

#endif // __CLASSEXAM_SPACEWARCONFIG_H__
