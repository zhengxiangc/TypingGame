/* -------------------------------------------------------------------------
//  文件名      :  gamecontroller.cpp
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Save Apples rules: spawning apples, scoring, level transitions, input handling.
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#include "gamecontroller.h"

#include <QRandomGenerator>

KCTGameController::KCTGameController() = default;

void KCTGameController::startGame(const KCTGameConfig& config)
{
    m_data.score = 0;
    m_data.failCount = 0;
    m_data.level = 1;
    m_data.speed = config.baseSpeed;
    m_data.targetScore = config.baseTarget;
    m_data.gameOver = false;
    m_data.levelComplete = false;
    m_data.state = KCTGameStateData::PlayingGameState;
    m_data.apples.clear();
    m_data.usedLetters.clear();
}

void KCTGameController::ensureMinimumApples(int count, const KCTGameConfig& config, int gameWidth, int appleSize)
{
    if (m_data.state != KCTGameStateData::PlayingGameState) {
        return;
    }
    while (m_data.apples.size() < count && m_data.apples.size() < config.maxApples) {
        const int nBefore = m_data.apples.size();
        tryCreateApple(config, gameWidth, appleSize);
        if (m_data.apples.size() == nBefore) {
            break;
        }
    }
}

void KCTGameController::setState(KCTGameStateData::GameState newState)
{
    if (m_data.gameOver && newState != KCTGameStateData::EndGameState) {
        return;
    }
    if (m_data.levelComplete && newState != KCTGameStateData::PlayingGameState) {
        return;
    }
    m_data.state = newState;
    if (newState == KCTGameStateData::EndGameState) {
        m_data.gameOver = true;
    }
}

KCTGameStateData::GameState KCTGameController::state() const
{
    return m_data.state;
}

void KCTGameController::tick(const KCTGameConfig& config, int gameWidth, int appleSize, int failLine)
{
    if (m_data.state != KCTGameStateData::PlayingGameState || m_data.levelComplete) {
        return;
    }

    for (int i = 0; i < m_data.apples.size(); ++i) {
        m_data.apples[i].y += m_data.speed;
        if (m_data.apples[i].y + appleSize >= failLine) {
            m_data.failCount++;
            removeAppleAt(i);
            --i;
        }
    }

    if (QRandomGenerator::global()->bounded(0, 100) < config.spawnRate) {
        tryCreateApple(config, gameWidth, appleSize);
    }
}

bool KCTGameController::handleLetterInput(QChar key, const KCTGameConfig& config, int gameWidth, int appleSize)
{
    if (m_data.state != KCTGameStateData::PlayingGameState || m_data.levelComplete) {
        return false;
    }

    for (int i = 0; i < m_data.apples.size(); ++i) {
        if (m_data.apples[i].letter == key) {
            removeAppleAt(i);
            m_data.score++;
            checkLevelComplete(config);
            if (!m_data.levelComplete && m_data.apples.size() < 2) {
                tryCreateApple(config, gameWidth, appleSize);
            }
            return true;
        }
    }
    return false;
}

void KCTGameController::prepareNextLevel(const KCTGameConfig& config, int gameWidth, int appleSize)
{
    if (!m_data.levelComplete) {
        return;
    }
    m_data.levelComplete = false;
    m_data.state = KCTGameStateData::PlayingGameState;
    ensureMinimumApples(2, config, gameWidth, appleSize);
}

const KCTGameStateData& KCTGameController::data() const
{
    return m_data;
}

KCTGameStateData& KCTGameController::data()
{
    return m_data;
}

void KCTGameController::tryCreateApple(const KCTGameConfig& config, int gameWidth, int appleSize)
{
    if (m_data.state != KCTGameStateData::PlayingGameState) {
        return;
    }
    if (m_data.apples.size() >= config.maxApples) {
        return;
    }

    QChar letter;
    bool letterAvailable = false;
    for (int nAttempt = 0; nAttempt < 50; ++nAttempt) {
        letter = QChar('A' + QRandomGenerator::global()->bounded(0, 26));
        if (!m_data.usedLetters.contains(letter)) {
            letterAvailable = true;
            break;
        }
    }
    if (!letterAvailable) {
        return;
    }

    KCTAppleEntity apple;
    apple.letter = letter;
    apple.x = QRandomGenerator::global()->bounded(0, gameWidth - appleSize);
    apple.y = -appleSize;
    apple.visible = true;
    m_data.apples.append(apple);
    m_data.usedLetters.append(letter);
}

void KCTGameController::removeAppleAt(int index)
{
    if (index < 0 || index >= m_data.apples.size()) {
        return;
    }
    m_data.usedLetters.removeOne(m_data.apples[index].letter);
    m_data.apples.removeAt(index);
}

void KCTGameController::checkLevelComplete(const KCTGameConfig& config)
{
    const int nTarget = config.baseTarget > 0 ? config.baseTarget : m_data.targetScore;
    if (m_data.score >= nTarget && !m_data.levelComplete && m_data.state == KCTGameStateData::PlayingGameState) {
        m_data.levelComplete = true;
        m_data.level++;
        m_data.speed += 1;
        m_data.targetScore = nTarget + (m_data.level - 1) * 2;
        m_data.score = 0;
        m_data.apples.clear();
        m_data.usedLetters.clear();
    }
}
