/* -------------------------------------------------------------------------
//  文件名      :  spacewarcontroller.cpp
//  创建者      :  陈正翔
//  创建时间    :  2026-05-11
//  功能描述    :  Space War simulation: enemies, bullets, explosions, reward word flight.
// -------------------------------------------------------------------------*/

#include "spacewarcontroller.h"

#include <algorithm>
#include <cmath>

#include <QRandomGenerator>
#include <QString>

namespace {
constexpr double gs_mathPi = 3.14159265358979323846;
constexpr double gs_bulletMaxSpeed = 9.0;
constexpr double gs_bulletSteer = 3.2;
constexpr double gs_bulletDamping = 0.88;
constexpr int gs_explosionFrameMs = 45;
constexpr int gs_explosionFrames = 8;
} // namespace (file-local tuning constants)

KCTSpaceWarController::KCTSpaceWarController() = default;

void KCTSpaceWarController::startGame(const KCTSpaceWarConfig& config)
{
    Q_UNUSED(config);
    m_data.enemies.clear();
    m_data.usedLetters.clear();
    m_data.bullets.clear();
    m_data.explosions.clear();
    m_data.playerX = 375;
    m_data.playerVelX = std::abs(m_data.playerVelX) > 0.1 ? m_data.playerVelX : 2.8;
    m_data.score = 0;
    m_data.health = 18;
    m_data.upgradeTier = 0;
    m_data.playingElapsedMs = 0;
    m_data.gameOver = false;
    m_data.state = KCTSpaceWarStateData::PlayingGameState;
    m_data.nextEnemyId = 1;
    m_data.flyingRewardWord = {};
    m_data.rewardTypingIndex = 0;
}

void KCTSpaceWarController::setState(KCTSpaceWarStateData::GameState newState)
{
    if (m_data.gameOver && newState != KCTSpaceWarStateData::EndGameState) {
        return;
    }
    m_data.state = newState;
    if (newState == KCTSpaceWarStateData::EndGameState) {
        m_data.gameOver = true;
    }
}

KCTSpaceWarStateData::GameState KCTSpaceWarController::state() const
{
    return m_data.state;
}

const KCTSpaceWarStateData& KCTSpaceWarController::data() const
{
    return m_data;
}

KCTSpaceWarStateData& KCTSpaceWarController::data()
{
    return m_data;
}

double KCTSpaceWarController::enemyFallSpeed(const KCTSpaceWarConfig& config) const
{
    const double base = 0.55 + static_cast<double>(config.enemySpeedLevel - 1) * 0.38;
    const double tier = 1.0 + static_cast<double>(m_data.upgradeTier) * 0.14;
    return base * tier;
}

void KCTSpaceWarController::applyDifficultyUpgrade(const KCTSpaceWarConfig& config)
{
    Q_UNUSED(config);
    m_data.upgradeTier++;
}

void KCTSpaceWarController::tick(const KCTSpaceWarConfig& config, int deltaMs, int gameWidth, int gameHeight,
                                 int enemySize, int playerW, int playerH, int playerY)
{
    if (m_data.state != KCTSpaceWarStateData::PlayingGameState || m_data.gameOver) {
        return;
    }

    m_data.playingElapsedMs += deltaMs;
    const int intervalMs = qMax(3, config.upgradeIntervalSec) * 1000;
    const int tier = static_cast<int>(m_data.playingElapsedMs / intervalMs);
    while (m_data.upgradeTier < tier) {
        applyDifficultyUpgrade(config);
    }

    // Player auto-move (horizontal bounce)
    m_data.playerX += m_data.playerVelX;
    if (m_data.playerX <= 0) {
        m_data.playerX = 0;
        m_data.playerVelX = std::abs(m_data.playerVelX);
    } else if (m_data.playerX + playerW >= gameWidth) {
        m_data.playerX = gameWidth - playerW;
        m_data.playerVelX = -std::abs(m_data.playerVelX);
    }

    updateEnemies(config, gameWidth, gameHeight, enemySize, playerW, playerH, playerY);
    updateBullets(config, enemySize, playerW, playerH, playerY);
    updateExplosions(deltaMs);
    updateFlyingRewardWord(deltaMs, gameWidth);

    const int spawnChance = qMin(40, config.spawnRatePercent + m_data.upgradeTier);
    if (QRandomGenerator::global()->bounded(0, 100) < spawnChance) {
        trySpawnEnemy(config, gameWidth, enemySize);
    }
}

void KCTSpaceWarController::updateEnemies(const KCTSpaceWarConfig& config, int gameWidth, int gameHeight,
                                          int enemySize, int playerW, int playerH, int playerY)
{
    const double fall = enemyFallSpeed(config);
    const double swayAmp = 38.0 + static_cast<double>(m_data.upgradeTier % 3) * 4.0;

    for (int i = 0; i < m_data.enemies.size(); ++i) {
        KCTEnemyEntity& enemy = m_data.enemies[i];
        enemy.phase += 0.12;
        enemy.y += fall;
        enemy.x = enemy.anchorX + swayAmp * std::sin(enemy.phase * 0.11);

        if (enemy.x < 0) {
            enemy.x = 0;
        } else if (enemy.x + enemySize > gameWidth) {
            enemy.x = gameWidth - enemySize;
        }

        const double ecx = enemy.x + enemySize * 0.5;
        const double ecy = enemy.y + enemySize * 0.5;
        const double pcx = m_data.playerX + playerW * 0.5;
        const double pcy = playerY + playerH * 0.5;
        const double dx = ecx - pcx;
        const double dy = ecy - pcy;
        const double dist = std::sqrt(dx * dx + dy * dy);
        if (dist < (enemySize + (std::max)(playerW, playerH)) * 0.42) {
            if (m_data.health > 0) {
                m_data.health--;
            }
            removeEnemyAt(i);
            --i;
            if (m_data.health <= 0) {
                setState(KCTSpaceWarStateData::EndGameState);
            }
            continue;
        }

        if (enemy.y + enemySize >= gameHeight) {
            if (m_data.health > 0) {
                m_data.health--;
            }
            removeEnemyAt(i);
            --i;
            if (m_data.health <= 0) {
                setState(KCTSpaceWarStateData::EndGameState);
            }
        }
    }
}

int KCTSpaceWarController::findEnemyIndexById(int id) const
{
    for (int i = 0; i < m_data.enemies.size(); ++i) {
        if (m_data.enemies[i].id == id) {
            return i;
        }
    }
    return -1;
}

void KCTSpaceWarController::removeEnemyAt(int index)
{
    if (index < 0 || index >= m_data.enemies.size()) {
        return;
    }
    m_data.usedLetters.removeOne(m_data.enemies[index].letter);
    m_data.enemies.removeAt(index);
}

void KCTSpaceWarController::trySpawnEnemy(const KCTSpaceWarConfig& config, int gameWidth, int enemySize)
{
    if (m_data.enemies.size() >= config.maxEnemies) {
        return;
    }

    QChar letter;
    bool letterAvailable = false;
    for (int nAttempt = 0; nAttempt < 60; ++nAttempt) {
        letter = QChar('A' + QRandomGenerator::global()->bounded(0, 26));
        if (!m_data.usedLetters.contains(letter)) {
            letterAvailable = true;
            break;
        }
    }
    if (!letterAvailable) {
        return;
    }

    KCTEnemyEntity enemy;
    enemy.id = m_data.nextEnemyId++;
    enemy.letter = letter;
    enemy.anchorX = QRandomGenerator::global()->bounded(0, qMax(1, gameWidth - enemySize));
    enemy.x = enemy.anchorX;
    enemy.y = -enemySize;
    enemy.phase = QRandomGenerator::global()->generateDouble() * 2.0 * gs_mathPi;
    m_data.enemies.append(enemy);
    m_data.usedLetters.append(letter);
}

bool KCTSpaceWarController::handleLetterInput(QChar key, const KCTSpaceWarConfig& config, int gameWidth,
                                              int enemySize, int playerW, int playerH, int playerY)
{
    Q_UNUSED(config);
    Q_UNUSED(gameWidth);
    if (m_data.state != KCTSpaceWarStateData::PlayingGameState || m_data.gameOver) {
        return false;
    }

    for (int i = 0; i < m_data.enemies.size(); ++i) {
        if (m_data.enemies[i].letter != key) {
            continue;
        }

        const KCTEnemyEntity& target = m_data.enemies[i];
        KCTBulletEntity bullet;
        bullet.x = m_data.playerX + playerW * 0.5;
        bullet.y = playerY + playerH * 0.5;
        const double tcx = target.x + enemySize * 0.5;
        const double tcy = target.y + enemySize * 0.5;
        double dx = tcx - bullet.x;
        double dy = tcy - bullet.y;
        const double len = std::sqrt(dx * dx + dy * dy);
        if (len > 1e-6) {
            dx /= len;
            dy /= len;
        } else {
            dx = 0;
            dy = -1;
        }
        bullet.vx = dx * 4.0;
        bullet.vy = dy * 4.0;
        bullet.targetEnemyId = target.id;
        m_data.bullets.append(bullet);
        return true;
    }
    return false;
}

void KCTSpaceWarController::updateBullets(const KCTSpaceWarConfig& config, int enemySize, int playerW, int playerH,
                                          int playerY)
{
    Q_UNUSED(playerW);
    Q_UNUSED(playerH);
    Q_UNUSED(playerY);

    for (int i = 0; i < m_data.bullets.size(); ++i) {
        KCTBulletEntity& bullet = m_data.bullets[i];
        const int ti = findEnemyIndexById(bullet.targetEnemyId);
        if (ti < 0) {
            m_data.bullets.removeAt(i);
            --i;
            continue;
        }

        const KCTEnemyEntity& target = m_data.enemies[ti];
        const double tcx = target.x + enemySize * 0.5;
        const double tcy = target.y + enemySize * 0.5;
        double dx = tcx - bullet.x;
        double dy = tcy - bullet.y;
        const double len = std::sqrt(dx * dx + dy * dy);
        if (len > 1e-6) {
            dx /= len;
            dy /= len;
        }

        bullet.vx = bullet.vx * gs_bulletDamping + dx * gs_bulletSteer;
        bullet.vy = bullet.vy * gs_bulletDamping + dy * gs_bulletSteer;
        const double speed = std::sqrt(bullet.vx * bullet.vx + bullet.vy * bullet.vy);
        if (speed > gs_bulletMaxSpeed && speed > 1e-6) {
            const double s = gs_bulletMaxSpeed / speed;
            bullet.vx *= s;
            bullet.vy *= s;
        }

        bullet.x += bullet.vx;
        bullet.y += bullet.vy;

        const double hdx = bullet.x - tcx;
        const double hdy = bullet.y - tcy;
        const double hitR = enemySize * 0.45 + 6.0;
        if (hdx * hdx + hdy * hdy <= hitR * hitR) {
            KCTExplosionEntity explosion;
            explosion.x = tcx;
            explosion.y = tcy;
            explosion.frame = 0;
            explosion.accumMs = 0;
            m_data.explosions.append(explosion);

            const int killedId = m_data.enemies[ti].id;
            m_data.score += config.bonusMode ? 2 : 1;
            removeEnemyAt(ti);

            for (int j = m_data.bullets.size() - 1; j >= 0; --j) {
                if (m_data.bullets[j].targetEnemyId == killedId) {
                    m_data.bullets.removeAt(j);
                    if (j < i) {
                        --i;
                    }
                }
            }
            --i;
        }
    }
}

void KCTSpaceWarController::updateExplosions(int deltaMs)
{
    for (int i = 0; i < m_data.explosions.size(); ++i) {
        KCTExplosionEntity& explosion = m_data.explosions[i];
        explosion.accumMs += deltaMs;
        while (explosion.accumMs >= gs_explosionFrameMs && explosion.frame < gs_explosionFrames) {
            explosion.accumMs -= gs_explosionFrameMs;
            explosion.frame++;
        }
        if (explosion.frame >= gs_explosionFrames) {
            m_data.explosions.removeAt(i);
            --i;
        }
    }
}

QString KCTSpaceWarController::sanitizeRewardWord(const QString& raw)
{
    QString out;
    out.reserve(raw.size());
    for (QChar ch : raw) {
        if (ch.isLetter()) {
            out.append(ch.toUpper());
        }
    }
    return out;
}

bool KCTSpaceWarController::hasActiveFlyingRewardWord() const
{
    return m_data.flyingRewardWord.active && !m_data.flyingRewardWord.text.isEmpty();
}

void KCTSpaceWarController::spawnFlyingRewardWord(const QString& wordUppercase, int gameWidth, int gameHeight)
{
    Q_UNUSED(gameWidth);
    const QString w = sanitizeRewardWord(wordUppercase);
    if (w.size() < 3) {
        return;
    }
    m_data.flyingRewardWord.text = w;
    m_data.flyingRewardWord.velocityXPixelsPerSec = 115.0;
    m_data.flyingRewardWord.x = -static_cast<double>(w.size()) * 15.0 - 48.0;
    const int yMin = static_cast<int>(static_cast<double>(gameHeight) * 0.16);
    const int yMax = static_cast<int>(static_cast<double>(gameHeight) * 0.42);
    m_data.flyingRewardWord.y = static_cast<double>(QRandomGenerator::global()->bounded(yMin, qMax(yMin + 1, yMax)));
    m_data.flyingRewardWord.active = true;
    m_data.rewardTypingIndex = 0;
}

void KCTSpaceWarController::updateFlyingRewardWord(int deltaMs, int gameWidth)
{
    if (!m_data.flyingRewardWord.active) {
        return;
    }
    const double dt = static_cast<double>(deltaMs) / 1000.0;
    m_data.flyingRewardWord.x += m_data.flyingRewardWord.velocityXPixelsPerSec * dt;
    const double estimatedWidth = m_data.flyingRewardWord.text.size() * 17.0 + 40.0;
    if (m_data.flyingRewardWord.x > static_cast<double>(gameWidth) + estimatedWidth) {
        m_data.flyingRewardWord.active = false;
        m_data.flyingRewardWord.text.clear();
        m_data.rewardTypingIndex = 0;
    }
}

KCTSpaceWarController::RewardKeyResult KCTSpaceWarController::handleFlyingRewardWordKey(QChar keyUppercase)
{
    if (!hasActiveFlyingRewardWord()) {
        return RewardKeyResult::NoActiveWord;
    }
    const QString& w = m_data.flyingRewardWord.text;
    if (m_data.rewardTypingIndex >= w.size()) {
        return RewardKeyResult::NoActiveWord;
    }

    if (w.at(m_data.rewardTypingIndex) == keyUppercase) {
        m_data.rewardTypingIndex++;
        if (m_data.rewardTypingIndex >= w.size()) {
            m_data.health = maxHealthValue;
            m_data.flyingRewardWord.active = false;
            m_data.flyingRewardWord.text.clear();
            m_data.rewardTypingIndex = 0;
            return RewardKeyResult::CompletedHeal;
        }
        return RewardKeyResult::ProgressConsumed;
    }
    m_data.rewardTypingIndex = 0;
    return RewardKeyResult::NoActiveWord;
}
