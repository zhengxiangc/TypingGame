/* -------------------------------------------------------------------------
//  文件名      :  resourceconfig.cpp
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Static helpers for UI stylesheets and user-visible status strings.
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#include "resourceconfig.h"

#include <QCoreApplication>

QString KCTResourceConfig::gameBackgroundStyle()
{
    return "background-color: rgb(30, 30, 40);";
}

QString KCTResourceConfig::exitButtonStyle()
{
    return "background-color: #ff4444; color: white; font-size: 12px; font-weight: bold; border-radius: 5px;";
}

QString KCTResourceConfig::settingsButtonStyle()
{
    return "background-color: #2196F3; color: white; font-size: 12px; font-weight: bold; border-radius: 5px;";
}

QString KCTResourceConfig::pauseButtonStyle()
{
    return "background-color: #ffaa00; color: white; font-size: 12px; font-weight: bold; border-radius: 5px;";
}

QString KCTResourceConfig::appleStyle()
{
    return "background-color: red; border-radius: 25px; font-size: 24px; font-weight: bold; color: white;";
}

QString KCTResourceConfig::levelMessageStyle()
{
    return "background-color: rgba(0, 0, 0, 180); color: green; font-size: 24px; font-weight: bold; border-radius: 10px; padding: 20px;";
}

QString KCTResourceConfig::dialogStyle()
{
    return "background-color: rgb(50, 50, 60); color: white;";
}

QString KCTResourceConfig::titleIdle()
{
    return QCoreApplication::translate("KCTResourceConfig", "Save Apples - Idle - Press ENTER to Start");
}

QString KCTResourceConfig::titlePaused()
{
    return QCoreApplication::translate("KCTResourceConfig", "Save Apples - PAUSED - Press SPACE to Resume");
}

QString KCTResourceConfig::titleEnd(int level)
{
    return QCoreApplication::translate("KCTResourceConfig", "Save Apples - GAME OVER - Final Level: %1").arg(level);
}

QString KCTResourceConfig::titlePlaying(int level, int target, int success, int fail, int accuracy, int speed)
{
    return QCoreApplication::translate("KCTResourceConfig",
                                       "Save Apples - Level %1  Need: %2  Success: %3  Fail: %4  Accuracy: %5%  Speed: %6")
        .arg(level)
        .arg(target)
        .arg(success)
        .arg(fail)
        .arg(accuracy)
        .arg(speed);
}

QString KCTResourceConfig::levelCompleteMessage()
{
    return QCoreApplication::translate("KCTResourceConfig", "Level Complete! Entering Next Level...");
}

QString KCTResourceConfig::failLineText()
{
    return QCoreApplication::translate("KCTResourceConfig", "Fail Line (70%)");
}

QString KCTResourceConfig::pressEnterText()
{
    return QCoreApplication::translate("KCTResourceConfig", "Press ENTER to Start");
}

QString KCTResourceConfig::pausedHintText()
{
    return QCoreApplication::translate("KCTResourceConfig", "PAUSED - Press SPACE to Resume");
}
