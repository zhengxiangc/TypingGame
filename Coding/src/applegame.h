/* -------------------------------------------------------------------------
//  文件名      :  applegame.h
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Declares KCTAppleGame widget for the Save Apples mini-game.
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#ifndef __CLASSEXAM_APPLEGAME_H__
#define __CLASSEXAM_APPLEGAME_H__

#include <QList>
#include <QString>
#include <QWidget>
#include <memory>

#include "controller/gamecontroller.h"
#include "model/gameconfig.h"

class QCloseEvent;
class QKeyEvent;
class QLabel;
class QPaintEvent;
class QPushButton;
class QTimer;
class KCTAsyncLogger;
class KCTEventTracker;
class KCTGameAudioService;

class KCTAppleGame : public QWidget
{
    Q_OBJECT

public:
    using GameState = KCTGameStateData::GameState;

    explicit KCTAppleGame(QWidget *parent = nullptr);
    ~KCTAppleGame() override;

    void setGameState(GameState newState);
    GameState gameState() const;
    void startGame();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void updateGame();
    void onExitClicked();
    void onSettingsClicked();
    void applySettings();

private:
    static constexpr int s_gameWidth = 800;
    static constexpr int s_gameHeight = 600;
    static constexpr int s_failLine = 420;
    static constexpr int s_appleSize = 50;

    void updateUI();
    void showLevelCompleteMessage();
    void syncAppleLabels();
    void clearAppleLabels();
    void createInitialApples();
    void loadSettings();
    void saveSettings();
    void logLine(const QString& message);
    void trackEvent(const QString& eventName);

    QTimer* m_timer;
    QLabel* m_messageLabel;
    QPushButton* m_exitButton;
    QPushButton* m_settingsButton;
    QPushButton* m_pauseButton;

    KCTGameController m_controller;
    KCTGameConfig m_config;
    QList<QLabel*> m_appleLabels;

    std::unique_ptr<KCTAsyncLogger> m_logger;
    std::unique_ptr<KCTEventTracker> m_tracker;
    std::unique_ptr<KCTGameAudioService> m_audio;

    KCTAppleGame(const KCTAppleGame&) = delete;
    KCTAppleGame& operator=(const KCTAppleGame&) = delete;
};

#endif // __CLASSEXAM_APPLEGAME_H__
