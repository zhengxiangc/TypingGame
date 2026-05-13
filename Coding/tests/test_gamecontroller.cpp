/* -------------------------------------------------------------------------
//  文件名      :  test_gamecontroller.cpp
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Qt Test cases for KCTGameController (Save Apples logic).
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#include "controller/gamecontroller.h"
#include "model/gameconfig.h"

#include <QtTest/QtTest>

class KCTGameControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void startGame_initializesState()
    {
        KCTGameController controller;
        KCTGameConfig cfg;
        controller.startGame(cfg);

        QCOMPARE(controller.data().score, 0);
        QCOMPARE(controller.data().failCount, 0);
        QCOMPARE(controller.data().level, 1);
        QCOMPARE(controller.data().speed, cfg.baseSpeed);
        QCOMPARE(controller.data().targetScore, cfg.baseTarget);
        QVERIFY(!controller.data().gameOver);
        QVERIFY(!controller.data().levelComplete);
        QCOMPARE(controller.state(), KCTGameStateData::PlayingGameState);
        QVERIFY(controller.data().apples.isEmpty());
        QVERIFY(controller.data().usedLetters.isEmpty());
    }

    void setState_blocksWrongTransitionAfterGameOver()
    {
        KCTGameController controller;
        KCTGameConfig cfg;
        controller.startGame(cfg);
        controller.setState(KCTGameStateData::EndGameState);
        QVERIFY(controller.data().gameOver);
        controller.setState(KCTGameStateData::PlayingGameState);
        QCOMPARE(controller.state(), KCTGameStateData::EndGameState);
    }

    void setState_blocksNonPlayingWhileLevelComplete()
    {
        KCTGameController controller;
        KCTGameConfig cfg;
        cfg.baseTarget = 1;
        controller.startGame(cfg);
        controller.data().apples.clear();
        controller.data().usedLetters.clear();
        KCTAppleEntity apple;
        apple.letter = QLatin1Char('A');
        apple.x = 0;
        apple.y = 0;
        apple.visible = true;
        controller.data().apples.append(apple);
        controller.data().usedLetters.append(apple.letter);
        QVERIFY(controller.handleLetterInput(QLatin1Char('A'), cfg, 800, 50));
        QVERIFY(controller.data().levelComplete);
        QCOMPARE(controller.state(), KCTGameStateData::PlayingGameState);
        controller.setState(KCTGameStateData::PausedGameState);
        QCOMPARE(controller.state(), KCTGameStateData::PlayingGameState);
    }

    void handleLetterInput_miss_returnsFalse()
    {
        KCTGameController controller;
        KCTGameConfig cfg;
        controller.startGame(cfg);
        KCTAppleEntity apple;
        apple.letter = QLatin1Char('B');
        apple.x = 10;
        apple.y = 10;
        apple.visible = true;
        controller.data().apples.append(apple);
        controller.data().usedLetters.append(apple.letter);
        QVERIFY(!controller.handleLetterInput(QLatin1Char('Z'), cfg, 800, 50));
        QCOMPARE(controller.data().apples.size(), 1);
    }

    void tick_movesApple_andFailLine()
    {
        KCTGameController controller;
        KCTGameConfig cfg;
        cfg.spawnRate = 0;
        controller.startGame(cfg);
        KCTAppleEntity apple;
        apple.letter = QLatin1Char('C');
        apple.x = 0;
        apple.y = 510;
        apple.visible = true;
        controller.data().apples.append(apple);
        controller.data().usedLetters.append(apple.letter);
        const int failLine = 560;
        const int appleSize = 50;
        controller.tick(cfg, 800, appleSize, failLine);
        QCOMPARE(controller.data().failCount, 1);
        QVERIFY(controller.data().apples.isEmpty());
    }

    void prepareNextLevel_requiresLevelCompleteFlag()
    {
        KCTGameController controller;
        KCTGameConfig cfg;
        cfg.spawnRate = 0;
        controller.startGame(cfg);
        controller.prepareNextLevel(cfg, 800, 50);
        QVERIFY(controller.data().apples.isEmpty());

        controller.data().levelComplete = true;
        controller.prepareNextLevel(cfg, 800, 50);
        QVERIFY(!controller.data().levelComplete);
        QCOMPARE(controller.state(), KCTGameStateData::PlayingGameState);
    }

    void checkLevelComplete_advancesLevelAndClearsApples()
    {
        KCTGameController controller;
        KCTGameConfig cfg;
        cfg.baseTarget = 2;
        cfg.spawnRate = 0;
        controller.startGame(cfg);
        KCTAppleEntity apple;
        apple.letter = QLatin1Char('D');
        apple.x = 0;
        apple.y = 0;
        apple.visible = true;
        controller.data().apples.append(apple);
        controller.data().usedLetters.append(apple.letter);
        QVERIFY(controller.handleLetterInput(QLatin1Char('D'), cfg, 800, 50));
        QVERIFY(!controller.data().levelComplete);
        controller.data().apples.clear();
        controller.data().usedLetters.clear();
        KCTAppleEntity appleB;
        appleB.letter = QLatin1Char('E');
        appleB.x = 0;
        appleB.y = 0;
        appleB.visible = true;
        controller.data().apples.append(appleB);
        controller.data().usedLetters.append(appleB.letter);
        QVERIFY(controller.handleLetterInput(QLatin1Char('E'), cfg, 800, 50));
        QVERIFY(controller.data().levelComplete);
        QCOMPARE(controller.data().level, 2);
        QVERIFY(controller.data().apples.isEmpty());
    }

    void ensureMinimumApples_respectsMaxApples()
    {
        KCTGameController controller;
        KCTGameConfig cfg;
        cfg.maxApples = 1;
        cfg.spawnRate = 0;
        controller.startGame(cfg);
        controller.ensureMinimumApples(5, cfg, 800, 50);
        QCOMPARE(controller.data().apples.size(), 1);
    }
};

QTEST_GUILESS_MAIN(KCTGameControllerTest)

#include "test_gamecontroller.moc"
