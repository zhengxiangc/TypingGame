/* -------------------------------------------------------------------------
//  文件名      :  gameconfig.h
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Tunable parameters for Save Apples difficulty and pacing.
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#ifndef __CLASSEXAM_GAMECONFIG_H__
#define __CLASSEXAM_GAMECONFIG_H__

struct KCTGameConfig
{
    int maxApples = 6;
    int spawnRate = 5;
    int baseSpeed = 2;
    int baseTarget = 5;
};

#endif // __CLASSEXAM_GAMECONFIG_H__
