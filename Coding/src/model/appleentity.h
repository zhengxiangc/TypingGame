/* -------------------------------------------------------------------------
//  文件名      :  appleentity.h
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Falling apple entity for the Save Apples model.
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#ifndef __CLASSEXAM_APPLEENTITY_H__
#define __CLASSEXAM_APPLEENTITY_H__

#include <QChar>

struct KCTAppleEntity
{
    QChar letter = QChar();
    int x = 0;
    int y = 0;
    bool visible = false;
};

#endif // __CLASSEXAM_APPLEENTITY_H__
