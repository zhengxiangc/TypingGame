/* -------------------------------------------------------------------------
//  文件名      :  spacewargame.h
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Declares KCTSpaceWarGame widget (Space War mini-game UI).
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#ifndef __CLASSEXAM_SPACEWARGAME_H__
#define __CLASSEXAM_SPACEWARGAME_H__

#include <QList>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QWidget>
#include <memory>

#include "controller/spacewarcontroller.h"
#include "model/spacewarconfig.h"

class QCloseEvent;
class QKeyEvent;
class QLabel;
class QPaintEvent;
class QPushButton;
class QTimer;
class KCTAsyncLogger;
class KCTDeepSeekWordClient;
class KCTEventTracker;
class KCTGameAudioService;

class KCTSpaceWarGame : public QWidget
{
    Q_OBJECT

public:
    using GameState = KCTSpaceWarStateData::GameState;

    explicit KCTSpaceWarGame(QWidget* parent = nullptr);
    ~KCTSpaceWarGame() override;

    void setGameState(GameState newState);
    GameState gameState() const;
    void startGame();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void updateGame();
    void onExitClicked();
    void onSettingsClicked();

private:
    static constexpr int s_gameWidth = 800;
    static constexpr int s_gameHeight = 600;
    static constexpr int s_enemySize = 46;
    static constexpr int s_playerWidth = 52;
    static constexpr int s_playerHeight = 38;

    static int anchorPlayerY() { return s_gameHeight - 80; }

    void updateUI();
    void syncEnemyLabels();
    void clearEnemyLabels();
    void loadSettings();
    void saveSettings();
    void logLine(const QString& message);
    void trackEvent(const QString& eventName);
    void maybeRequestRewardWordBatch();

    QTimer* m_timer;
    QPushButton* m_exitButton;
    QPushButton* m_settingsButton;
    QPushButton* m_pauseButton;

    KCTSpaceWarController m_controller;
    KCTSpaceWarConfig m_config;
    QList<QLabel*> m_enemyLabels;
    QVector<QPoint> m_stars;

    std::unique_ptr<KCTAsyncLogger> m_logger;
    std::unique_ptr<KCTEventTracker> m_tracker;
    std::unique_ptr<KCTGameAudioService> m_audio;
    std::unique_ptr<KCTDeepSeekWordClient> m_deepSeekWordClient;

    int m_rewardCooldownMs = 0;
    bool m_rewardRequestPending = false;
    int m_rewardDomainIndex = 0;
    QStringList m_rewardWordQueue;

    KCTSpaceWarGame(const KCTSpaceWarGame&) = delete;
    KCTSpaceWarGame& operator=(const KCTSpaceWarGame&) = delete;
};

#endif // __CLASSEXAM_SPACEWARGAME_H__
