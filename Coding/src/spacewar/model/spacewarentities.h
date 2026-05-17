/* -------------------------------------------------------------------------
//  文件名      :  spacewarentities.h
//  创建者      :  陈正翔
//  创建时间    :  2026-05-11
//  功能描述    :  DTOs for enemies, bullets, and explosions in Space War.
// -------------------------------------------------------------------------*/

#ifndef __CLASSEXAM_SPACEWARENTITIES_H__
#define __CLASSEXAM_SPACEWARENTITIES_H__

#include <QChar>

struct KCTEnemyEntity
{
    int id = 0;
    QChar letter = QChar();
    double x = 0;
    double y = 0;
    double anchorX = 0;
    double phase = 0;
};

struct KCTBulletEntity
{
    double x = 0;
    double y = 0;
    double vx = 0;
    double vy = 0;
    int targetEnemyId = -1;
};

struct KCTExplosionEntity
{
    double x = 0;
    double y = 0;
    int frame = 0;
    int accumMs = 0;
};

#endif // __CLASSEXAM_SPACEWARENTITIES_H__
