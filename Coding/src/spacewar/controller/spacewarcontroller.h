/* -------------------------------------------------------------------------
//  文件名      :  spacewarcontroller.h
//  创建者      :  陈正翔
//  创建时间    :  2026-05-11
//  功能描述    :  Declares KCTSpaceWarController (Space War rules, no UI).
// -------------------------------------------------------------------------*/

#ifndef __CLASSEXAM_SPACEWARCONTROLLER_H__
#define __CLASSEXAM_SPACEWARCONTROLLER_H__

#include "../model/spacewarconfig.h"
#include "../model/spacewarstatedata.h"

#include <QChar>

class KCTSpaceWarController
{
public:
    enum class RewardKeyResult {
        NoActiveWord,
        ProgressConsumed,
        CompletedHeal,
    };

    static constexpr int maxHealthValue = 18;

    KCTSpaceWarController();

    void startGame(const KCTSpaceWarConfig& config);
    void setState(KCTSpaceWarStateData::GameState newState);
    KCTSpaceWarStateData::GameState state() const;

    void tick(const KCTSpaceWarConfig& config, int deltaMs, int gameWidth, int gameHeight,
              int enemySize, int playerW, int playerH, int playerY);

    bool handleLetterInput(QChar key, const KCTSpaceWarConfig& config, int gameWidth, int enemySize,
                           int playerW, int playerH, int playerY);

    const KCTSpaceWarStateData& data() const;
    KCTSpaceWarStateData& data();

    bool hasActiveFlyingRewardWord() const;
    void spawnFlyingRewardWord(const QString& wordUppercase, int gameWidth, int gameHeight);
    void updateFlyingRewardWord(int deltaMs, int gameWidth);

    RewardKeyResult handleFlyingRewardWordKey(QChar keyUppercase);

private:
    void trySpawnEnemy(const KCTSpaceWarConfig& config, int gameWidth, int enemySize);
    void removeEnemyAt(int index);
    int findEnemyIndexById(int id) const;
    void applyDifficultyUpgrade(const KCTSpaceWarConfig& config);
    double enemyFallSpeed(const KCTSpaceWarConfig& config) const;
    void updateEnemies(const KCTSpaceWarConfig& config, int gameWidth, int gameHeight, int enemySize,
                       int playerW, int playerH, int playerY);
    void updateBullets(const KCTSpaceWarConfig& config, int enemySize, int playerW, int playerH, int playerY);
    void updateExplosions(int deltaMs);
    static QString sanitizeRewardWord(const QString& raw);

    KCTSpaceWarStateData m_data;

    KCTSpaceWarController(const KCTSpaceWarController&) = delete;
    KCTSpaceWarController& operator=(const KCTSpaceWarController&) = delete;
};

#endif // __CLASSEXAM_SPACEWARCONTROLLER_H__
