/* -------------------------------------------------------------------------
//  文件名      :  resourceconfig.h
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Declares KCTResourceConfig (centralized styles and copy strings).
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#ifndef __CLASSEXAM_RESOURCECONFIG_H__
#define __CLASSEXAM_RESOURCECONFIG_H__

#include <QString>

class KCTResourceConfig
{
public:
    static QString gameBackgroundStyle();
    static QString exitButtonStyle(int fontSizePx = 12);
    static QString settingsButtonStyle(int fontSizePx = 12);
    static QString pauseButtonStyle(int fontSizePx = 12);
    static QString spaceWarEnemyLabelStyle(int fontSizePx, int borderRadiusPx);
    static QString appleStyle();
    static QString levelMessageStyle();
    static QString dialogStyle();

    static QString titleIdle();
    static QString titlePaused();
    static QString titleEnd(int level);
    static QString titlePlaying(int level, int target, int success, int fail, int accuracy, int speed);
    static QString levelCompleteMessage();
    static QString failLineText();
    static QString pressEnterText();
    static QString pausedHintText();

private:
    KCTResourceConfig() = delete;
    KCTResourceConfig(const KCTResourceConfig&) = delete;
    KCTResourceConfig& operator=(const KCTResourceConfig&) = delete;
};

#endif // __CLASSEXAM_RESOURCECONFIG_H__
