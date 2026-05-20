/* -------------------------------------------------------------------------
//  文件名      :  spacewargame.h
//  创建者      :  陈正翔
//  创建时间    :  2026-05-11
//  功能描述    :  Declares KCTSpaceWarGame widget (Space War mini-game UI).
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

#include "config/gameviewport.h"
#include "controller/spacewarcontroller.h"
#include "model/spacewarconfig.h"
#include "spacewarassets.h"

class QCloseEvent;
class QKeyEvent;
class QPaintEvent;
class QPushButton;
class QResizeEvent;
class QShowEvent;
class QTimer;
class KCTAsyncLogger;
class KCTDeepSeekWordClient;
class KCTEventTracker;
class KCTSpaceWarAudioService;

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
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void updateGame();
    void onExitClicked();
    void onSettingsClicked();

private:
    static constexpr int s_designWidth = 800;
    static constexpr int s_designHeight = 600;
    static constexpr int s_designEnemySize = 46;
    static constexpr int s_designPlayerWidth = 52;
    static constexpr int s_designPlayerHeight = 38;
    static constexpr int s_designPlayerAnchorOffset = 80;

    void applyViewport();
    void regenerateStars();
    void relayoutControllerForViewport(int oldGameWidth, int oldGameHeight);
    void layoutTopButtons();

    int enemySize() const;
    int playerWidth() const;
    int playerHeight() const;
    int anchorPlayerY() const;
    int gameWidth() const;
    int gameHeight() const;

    void updateUI();
    void loadSettings();
    void drawHud(QPainter& painter) const;
    void drawEnemies(QPainter& painter) const;
    void detectGameplayAudioEvents();
    void saveSettings();
    void logLine(const QString& message);
    void trackEvent(const QString& eventName);
    void maybeRequestRewardWordBatch();

    QTimer* m_timer;
    QPushButton* m_exitButton;
    QPushButton* m_settingsButton;
    QPushButton* m_pauseButton;

    KCTGameViewport m_viewport;
    KCTSpaceWarController m_controller;
    KCTSpaceWarConfig m_config;
    QVector<QPoint> m_stars;
    KCTSpaceWarAssets m_assets;

    std::unique_ptr<KCTAsyncLogger> m_logger;
    std::unique_ptr<KCTEventTracker> m_tracker;
    std::unique_ptr<KCTSpaceWarAudioService> m_audio;
    std::unique_ptr<KCTDeepSeekWordClient> m_deepSeekWordClient;

    int m_rewardCooldownMs = 0;
    bool m_rewardRequestPending = false;
    int m_rewardDomainIndex = 0;
    QStringList m_rewardWordQueue;

    bool m_initialSizeApplied = false;
    int m_lastGameWidth = 0;
    int m_lastGameHeight = 0;

    int m_prevEnemyCount = 0;
    int m_prevExplosionCount = 0;
    int m_prevUpgradeTier = 0;

    KCTSpaceWarGame(const KCTSpaceWarGame&) = delete;
    KCTSpaceWarGame& operator=(const KCTSpaceWarGame&) = delete;
};

#endif // __CLASSEXAM_SPACEWARGAME_H__
