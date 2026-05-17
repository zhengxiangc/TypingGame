/* -------------------------------------------------------------------------
//  文件名      :  spacewargame.cpp
//  创建者      :  陈正翔
//  创建时间    :  2026-05-11
//  功能描述    :  Space War view: rendering, settings, reward-mode Deepseek integration.
// -------------------------------------------------------------------------*/

#include "spacewargame.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDateTime>
#include <QDialog>
#include <QFont>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSettings>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>

#include "config/resourceconfig.h"
#include "services/asynclogger.h"
#include "services/deepseekwordclient.h"
#include "services/eventtracker.h"
#include "services/gameaudioservice.h"
#include "services/sfxid.h"

namespace {
constexpr int gs_timerTickMilliseconds = 16;
constexpr int gs_rewardSpawnIntervalMs = 10000;
constexpr int gs_rewardBatchSize = 12;
constexpr int gs_rewardRefillThreshold = 3;

const QStringList& rewardDomainTopics()
{
    static const QStringList domains{
        QStringLiteral("astronomy and space"),
        QStringLiteral("ocean and marine life"),
        QStringLiteral("world geography"),
        QStringLiteral("sports and athletics"),
        QStringLiteral("music and instruments"),
        QStringLiteral("technology and computing"),
    };
    return domains;
}
} // namespace (file-local tuning constants and reward topic list)

KCTSpaceWarGame::KCTSpaceWarGame(QWidget* parent)
    : QWidget(parent)
    , m_timer(new QTimer(this))
{
    setFixedSize(s_gameWidth, s_gameHeight);
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet(KCTResourceConfig::gameBackgroundStyle());

    loadSettings();

    m_stars.resize(90);
    for (QPoint& starPosition : m_stars) {
        starPosition = QPoint(QRandomGenerator::global()->bounded(0, s_gameWidth),
                              QRandomGenerator::global()->bounded(0, s_gameHeight));
    }

    const QString appDir = QCoreApplication::applicationDirPath();
    m_logger = std::make_unique<KCTAsyncLogger>(appDir);
    m_tracker = std::make_unique<KCTEventTracker>(appDir);
    m_audio = std::make_unique<KCTGameAudioService>(appDir);

    m_deepSeekWordClient = std::make_unique<KCTDeepSeekWordClient>(this);
    connect(m_deepSeekWordClient.get(), &KCTDeepSeekWordClient::rewardDiagnosticLine, this,
            [this](const QString& line) { logLine(line); });
    connect(m_deepSeekWordClient.get(), &KCTDeepSeekWordClient::wordsBatchReady, this, [this](const QStringList& words) {
        m_rewardRequestPending = false;
        int added = 0;
        int skippedDup = 0;
        for (const QString& w : words) {
            if (w.isEmpty()) {
                continue;
            }
            if (m_rewardWordQueue.contains(w)) {
                ++skippedDup;
                continue;
            }
            m_rewardWordQueue.append(w);
            ++added;
        }
        const int snapshotN = qMin(16, m_rewardWordQueue.size());
        const QString snapshot = snapshotN > 0 ? m_rewardWordQueue.mid(0, snapshotN).join(QLatin1Char(','))
                                                 : QString();
        logLine(QStringLiteral("reward_cache batch_received in_batch=%1 added=%2 dup_skip=%3 queue_now=%4 snapshot=%5")
                    .arg(words.size())
                    .arg(added)
                    .arg(skippedDup)
                    .arg(m_rewardWordQueue.size())
                    .arg(snapshot.isEmpty() ? QStringLiteral("-") : snapshot));
        if (!m_config.rewardMode) {
            return;
        }
        if (m_controller.state() != KCTSpaceWarStateData::PlayingGameState || m_controller.data().gameOver) {
            return;
        }
        update();
    });

    m_exitButton = new QPushButton(tr("Exit"), this);
    m_exitButton->setGeometry(s_gameWidth - 88, 10, 72, 28);
    m_exitButton->setStyleSheet(KCTResourceConfig::exitButtonStyle());
    connect(m_exitButton, &QPushButton::clicked, this, &KCTSpaceWarGame::onExitClicked);

    m_settingsButton = new QPushButton(tr("Settings"), this);
    m_settingsButton->setGeometry(s_gameWidth - 178, 10, 80, 28);
    m_settingsButton->setStyleSheet(KCTResourceConfig::settingsButtonStyle());
    connect(m_settingsButton, &QPushButton::clicked, this, &KCTSpaceWarGame::onSettingsClicked);

    m_pauseButton = new QPushButton(tr("Pause"), this);
    m_pauseButton->setGeometry(s_gameWidth - 268, 10, 80, 28);
    m_pauseButton->setStyleSheet(KCTResourceConfig::pauseButtonStyle());
    connect(m_pauseButton, &QPushButton::clicked, this, [this]() {
        if (m_controller.state() == KCTSpaceWarStateData::PlayingGameState) {
            setGameState(KCTSpaceWarStateData::PausedGameState);
        } else if (m_controller.state() == KCTSpaceWarStateData::PausedGameState) {
            setGameState(KCTSpaceWarStateData::PlayingGameState);
        }
    });

    m_exitButton->setFocusPolicy(Qt::NoFocus);
    m_settingsButton->setFocusPolicy(Qt::NoFocus);
    m_pauseButton->setFocusPolicy(Qt::NoFocus);

    connect(m_timer, &QTimer::timeout, this, &KCTSpaceWarGame::updateGame);
}

KCTSpaceWarGame::~KCTSpaceWarGame()
{
    if (m_timer) {
        m_timer->stop();
    }
    clearEnemyLabels();
}

void KCTSpaceWarGame::setGameState(GameState newState)
{
    const GameState oldState = m_controller.state();
    m_controller.setState(newState);
    const GameState now = m_controller.state();

    if (m_audio) {
        if (oldState == KCTSpaceWarStateData::PlayingGameState && now == KCTSpaceWarStateData::PausedGameState) {
            m_audio->pauseBgm();
        } else if (oldState == KCTSpaceWarStateData::PausedGameState && now == KCTSpaceWarStateData::PlayingGameState) {
            m_audio->resumeBgm();
        } else if (now == KCTSpaceWarStateData::IdleGameState || now == KCTSpaceWarStateData::EndGameState) {
            m_audio->stopBgm();
        }
    }

    if ((oldState == KCTSpaceWarStateData::PlayingGameState && now == KCTSpaceWarStateData::PausedGameState)
        || (oldState == KCTSpaceWarStateData::PausedGameState && now == KCTSpaceWarStateData::PlayingGameState)) {
        trackEvent(QStringLiteral("pause_toggle"));
        logLine(QStringLiteral("pause_toggle"));
    }

    switch (now) {
    case KCTSpaceWarStateData::IdleGameState:
        m_timer->stop();
        setWindowTitle(tr("Space War — Press ENTER"));
        break;
    case KCTSpaceWarStateData::PlayingGameState:
        if (!m_timer->isActive()) {
            m_timer->start(gs_timerTickMilliseconds);
        }
        updateUI();
        break;
    case KCTSpaceWarStateData::PausedGameState:
        m_timer->stop();
        setWindowTitle(tr("Space War — PAUSED"));
        break;
    case KCTSpaceWarStateData::EndGameState:
        m_timer->stop();
        setWindowTitle(tr("Space War — GAME OVER  Score: %1").arg(m_controller.data().score));
        break;
    }
    update();
}

KCTSpaceWarGame::GameState KCTSpaceWarGame::gameState() const
{
    return m_controller.state();
}

void KCTSpaceWarGame::startGame()
{
    loadSettings();
    m_rewardCooldownMs = 0;
    m_rewardRequestPending = false;
    m_rewardDomainIndex = 0;
    m_rewardWordQueue.clear();
    m_controller.startGame(m_config);
    clearEnemyLabels();
    syncEnemyLabels();
    setGameState(KCTSpaceWarStateData::PlayingGameState);
    trackEvent(QStringLiteral("game_start"));
    logLine(QStringLiteral("game_start"));
    if (m_audio) {
        m_audio->startBgmIfAvailable();
    }
    if (m_config.rewardMode) {
        logLine(QStringLiteral("reward_cache game_start queue_cleared=1 threshold=%1 batch_size=%2")
                    .arg(gs_rewardRefillThreshold)
                    .arg(gs_rewardBatchSize));
        maybeRequestRewardWordBatch();
    }
    setFocus();
}

void KCTSpaceWarGame::syncEnemyLabels()
{
    const auto& enemies = m_controller.data().enemies;
    while (m_enemyLabels.size() > enemies.size()) {
        QLabel* pLabel = m_enemyLabels.takeLast();
        delete pLabel;
    }
    while (m_enemyLabels.size() < enemies.size()) {
        QLabel* pLabel = new QLabel(this);
        pLabel->setFixedSize(s_enemySize, s_enemySize);
        pLabel->setAlignment(Qt::AlignCenter);
        pLabel->setStyleSheet(
            QStringLiteral("background-color: #37474f; border: 2px solid #90a4ae; border-radius: 8px; "
                           "color: #eceff1; font-size: 22px; font-weight: bold;"));
        pLabel->show();
        m_enemyLabels.append(pLabel);
    }

    for (int i = 0; i < enemies.size(); ++i) {
        m_enemyLabels[i]->setText(QString(enemies[i].letter));
        m_enemyLabels[i]->move(static_cast<int>(enemies[i].x), static_cast<int>(enemies[i].y));
    }
}

void KCTSpaceWarGame::clearEnemyLabels()
{
    for (QLabel* pLabel : m_enemyLabels) {
        delete pLabel;
    }
    m_enemyLabels.clear();
}

void KCTSpaceWarGame::updateGame()
{
    if (m_controller.state() != KCTSpaceWarStateData::PlayingGameState) {
        return;
    }

    const int nHealthBefore = m_controller.data().health;
    m_controller.tick(m_config, gs_timerTickMilliseconds, s_gameWidth, s_gameHeight, s_enemySize, s_playerWidth, s_playerHeight, anchorPlayerY());

    if (m_controller.data().health < nHealthBefore) {
        if (m_audio) {
            m_audio->playSfx(KCTSfxId::MissSfxId);
        }
        trackEvent(QStringLiteral("health_loss"));
        logLine(QStringLiteral("health_loss"));
    }

    if (m_controller.state() == KCTSpaceWarStateData::EndGameState) {
        if (m_audio) {
            m_audio->stopBgm();
        }
        trackEvent(QStringLiteral("game_over"));
        logLine(QStringLiteral("game_over"));
        setWindowTitle(tr("Space War — GAME OVER  Score: %1").arg(m_controller.data().score));
        update();
        return;
    }

    if (m_config.rewardMode) {
        if (!m_controller.hasActiveFlyingRewardWord()) {
            m_rewardCooldownMs += gs_timerTickMilliseconds;
            if (m_rewardCooldownMs >= gs_rewardSpawnIntervalMs && !m_rewardWordQueue.isEmpty()) {
                m_rewardCooldownMs = 0;
                const QString w = m_rewardWordQueue.takeFirst();
                m_controller.spawnFlyingRewardWord(w, s_gameWidth, s_gameHeight);
                logLine(QStringLiteral("reward_cache spawn word=%1 queue_after=%2 http_pending=%3")
                            .arg(w)
                            .arg(m_rewardWordQueue.size())
                            .arg(m_rewardRequestPending ? 1 : 0));
            }
        }
        maybeRequestRewardWordBatch();
    }

    syncEnemyLabels();
    updateUI();
    update();
}

void KCTSpaceWarGame::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Space:
        if (m_controller.state() == KCTSpaceWarStateData::PlayingGameState) {
            setGameState(KCTSpaceWarStateData::PausedGameState);
        } else if (m_controller.state() == KCTSpaceWarStateData::PausedGameState) {
            setGameState(KCTSpaceWarStateData::PlayingGameState);
        }
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (m_controller.state() == KCTSpaceWarStateData::IdleGameState
            || m_controller.state() == KCTSpaceWarStateData::EndGameState) {
            startGame();
        }
        return;
    case Qt::Key_Escape:
        onExitClicked();
        return;
    default:
        break;
    }

    // Qt::Key_* for non-letter keys are large values (e.g. Shift); never pass them to QChar(key).
    const int keyVal = event->key();
    if (keyVal < Qt::Key_A || keyVal > Qt::Key_Z) {
        QWidget::keyPressEvent(event);
        return;
    }
    const QChar key = QChar(static_cast<char16_t>(keyVal));

    if (m_config.rewardMode) {
        const auto rewardResult = m_controller.handleFlyingRewardWordKey(key);
        if (rewardResult == KCTSpaceWarController::RewardKeyResult::CompletedHeal) {
            if (m_audio) {
                m_audio->playSfx(KCTSfxId::LevelCompleteSfxId);
            }
            trackEvent(QStringLiteral("reward_word_complete"));
            logLine(QStringLiteral("reward_word_complete"));
            updateUI();
            update();
            return;
        }
        if (rewardResult == KCTSpaceWarController::RewardKeyResult::ProgressConsumed) {
            update();
            return;
        }
    }

    if (m_controller.handleLetterInput(key, m_config, s_gameWidth, s_enemySize, s_playerWidth, s_playerHeight, anchorPlayerY())) {
        if (m_audio) {
            m_audio->playSfx(KCTSfxId::HitSfxId);
        }
        trackEvent(QStringLiteral("key_hit_success"));
        logLine(QStringLiteral("key_hit_success"));
        syncEnemyLabels();
        updateUI();
        update();
    }
}

void KCTSpaceWarGame::updateUI()
{
    if (m_controller.state() != KCTSpaceWarStateData::PlayingGameState) {
        return;
    }
    const auto& d = m_controller.data();
    setWindowTitle(tr("Space War — Score: %1  HP: %2  Tier: %3")
                       .arg(d.score)
                       .arg(d.health)
                       .arg(d.upgradeTier));
}

void KCTSpaceWarGame::onExitClicked()
{
    if (m_timer) {
        m_timer->stop();
    }
    close();
}

void KCTSpaceWarGame::closeEvent(QCloseEvent* event)
{
    if (m_timer) {
        m_timer->stop();
    }
    trackEvent(QStringLiteral("game_exit"));
    logLine(QStringLiteral("game_exit"));
    if (m_audio) {
        m_audio->stopBgm();
    }
    clearEnemyLabels();
    event->accept();
}

void KCTSpaceWarGame::logLine(const QString& message)
{
    if (m_logger) {
        m_logger->log(message);
    }
}

void KCTSpaceWarGame::maybeRequestRewardWordBatch()
{
    if (!m_config.rewardMode || m_rewardRequestPending) {
        return;
    }
    if (m_rewardWordQueue.size() >= gs_rewardRefillThreshold) {
        return;
    }
    const int queueBefore = m_rewardWordQueue.size();
    m_rewardRequestPending = true;
    const QStringList& domains = rewardDomainTopics();
    const QString domain = domains.at(m_rewardDomainIndex % domains.size());
    m_rewardDomainIndex++;
    logLine(QStringLiteral("reward_cache request_batch queue_before=%1 threshold=%2 batch_size=%3 domain=%4")
                .arg(queueBefore)
                .arg(gs_rewardRefillThreshold)
                .arg(gs_rewardBatchSize)
                .arg(domain));
    m_deepSeekWordClient->requestWordsBatchAsync(4, 8, gs_rewardBatchSize, domain);
}

void KCTSpaceWarGame::trackEvent(const QString& eventName)
{
    if (!m_tracker) {
        return;
    }
    QJsonObject jsonObject;
    jsonObject[QStringLiteral("unix_ms")] = QDateTime::currentMSecsSinceEpoch();
    jsonObject[QStringLiteral("event")] = eventName;
    const auto& d = m_controller.data();
    jsonObject[QStringLiteral("score")] = d.score;
    jsonObject[QStringLiteral("health")] = d.health;
    jsonObject[QStringLiteral("upgrade_tier")] = d.upgradeTier;
    jsonObject[QStringLiteral("game_state")] = int(d.state);

    const QJsonDocument doc(jsonObject);
    m_tracker->trackJsonLine(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

void KCTSpaceWarGame::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(12, 18, 42));

    painter.setPen(QPen(QColor(220, 220, 255, 40), 1));
    for (const QPoint& starPosition : m_stars) {
        painter.drawPoint(starPosition);
    }

    const auto& data = m_controller.data();
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 235, 120));
    for (const KCTBulletEntity& bullet : data.bullets) {
        painter.drawEllipse(QPointF(bullet.x, bullet.y), 4.0, 4.0);
    }

    for (const KCTExplosionEntity& explosion : data.explosions) {
        const int nFrame = explosion.frame;
        const double radius = 6.0 + static_cast<double>(nFrame) * 8.0;
        const int nAlpha = qMax(30, 230 - nFrame * 26);
        painter.setBrush(QColor(255, 140 - nFrame * 12, 30, nAlpha));
        painter.drawEllipse(QPointF(explosion.x, explosion.y), radius, radius * 0.85);
    }

    const int nPlayerY = anchorPlayerY();
    const double playerX = data.playerX;
    QPolygonF ship;
    ship << QPointF(playerX + s_playerWidth * 0.5, nPlayerY)
         << QPointF(playerX + 4, nPlayerY + s_playerHeight)
         << QPointF(playerX + s_playerWidth - 4, nPlayerY + s_playerHeight);
    painter.setBrush(QColor(72, 200, 255));
    painter.setPen(QPen(QColor(180, 240, 255), 2));
    painter.drawPolygon(ship);

    if (m_controller.hasActiveFlyingRewardWord()) {
        const auto& rw = m_controller.data().flyingRewardWord;
        QFont rewardFont(QStringLiteral("Arial"), 22, QFont::Bold);
        painter.setFont(rewardFont);
        const int nTyped = m_controller.data().rewardTypingIndex;
        const QString& full = rw.text;
        double x = rw.x;
        for (int i = 0; i < full.size(); ++i) {
            const QString ch = full.mid(i, 1);
            if (i < nTyped) {
                painter.setPen(QPen(QColor(120, 255, 200), 2));
            } else {
                painter.setPen(QPen(QColor(255, 250, 220), 2));
            }
            painter.drawText(QPointF(x, rw.y), ch);
            x += painter.fontMetrics().horizontalAdvance(ch) + 2;
        }
    }

    if (m_controller.state() == KCTSpaceWarStateData::IdleGameState) {
        painter.setPen(QPen(Qt::white, 1));
        painter.setFont(QFont(QStringLiteral("Arial"), 20));
        painter.drawText(QRect(0, s_gameHeight / 2 - 40, s_gameWidth, 60), Qt::AlignCenter,
                         tr("Press ENTER to start"));
    } else if (m_controller.state() == KCTSpaceWarStateData::PausedGameState) {
        painter.setPen(QPen(QColor(255, 220, 120), 1));
        painter.setFont(QFont(QStringLiteral("Arial"), 20));
        painter.drawText(QRect(0, s_gameHeight / 2 - 40, s_gameWidth, 60), Qt::AlignCenter,
                         tr("PAUSED — SPACE to resume"));
    } else if (m_controller.state() == KCTSpaceWarStateData::EndGameState) {
        painter.setPen(QPen(QColor(255, 120, 120), 1));
        painter.setFont(QFont(QStringLiteral("Arial"), 22));
        painter.drawText(QRect(0, s_gameHeight / 2 - 50, s_gameWidth, 100), Qt::AlignCenter,
                         tr("GAME OVER\nPress ENTER to retry"));
    }
}

void KCTSpaceWarGame::onSettingsClicked()
{
    const GameState previousState = m_controller.state();
    if (m_controller.state() == KCTSpaceWarStateData::PlayingGameState) {
        setGameState(KCTSpaceWarStateData::PausedGameState);
    }

    QDialog* pDialog = new QDialog(this);
    pDialog->setWindowTitle(tr("Space War Settings"));
    pDialog->setFixedSize(360, 360);
    pDialog->setStyleSheet(KCTResourceConfig::dialogStyle());
    pDialog->setAttribute(Qt::WA_DeleteOnClose);
    pDialog->setModal(true);

    auto* pLayout = new QVBoxLayout(pDialog);

    auto* pMaxLbl = new QLabel(tr("Max enemies on screen: %1").arg(m_config.maxEnemies), pDialog);
    auto* pMaxSlider = new QSlider(Qt::Horizontal, pDialog);
    pMaxSlider->setRange(1, 10);
    pMaxSlider->setValue(m_config.maxEnemies);

    auto* pSpdLbl = new QLabel(tr("Enemy speed level: %1").arg(m_config.enemySpeedLevel), pDialog);
    auto* pSpdSlider = new QSlider(Qt::Horizontal, pDialog);
    pSpdSlider->setRange(1, 10);
    pSpdSlider->setValue(m_config.enemySpeedLevel);

    auto* pUpLbl = new QLabel(tr("Upgrade interval (sec): %1").arg(m_config.upgradeIntervalSec), pDialog);
    auto* pUpSlider = new QSlider(Qt::Horizontal, pDialog);
    pUpSlider->setRange(3, 45);
    pUpSlider->setValue(m_config.upgradeIntervalSec);

    auto* pBonus = new QCheckBox(tr("Bonus mode (double score)"), pDialog);
    pBonus->setChecked(m_config.bonusMode);

    auto* pReward = new QCheckBox(tr("Reward mode (flying words, heal on full match)"), pDialog);
    pReward->setChecked(m_config.rewardMode);

    pLayout->addWidget(pMaxLbl);
    pLayout->addWidget(pMaxSlider);
    pLayout->addWidget(pSpdLbl);
    pLayout->addWidget(pSpdSlider);
    pLayout->addWidget(pUpLbl);
    pLayout->addWidget(pUpSlider);
    pLayout->addWidget(pBonus);
    pLayout->addWidget(pReward);

    auto* pRow = new QHBoxLayout();
    auto* pSaveBtn = new QPushButton(tr("Save"), pDialog);
    auto* pCancelBtn = new QPushButton(tr("Cancel"), pDialog);
    pRow->addWidget(pSaveBtn);
    pRow->addWidget(pCancelBtn);
    pLayout->addLayout(pRow);

    connect(pMaxSlider, &QSlider::valueChanged, [pMaxLbl](int value) {
        pMaxLbl->setText(QObject::tr("Max enemies on screen: %1").arg(value));
    });
    connect(pSpdSlider, &QSlider::valueChanged, [pSpdLbl](int value) {
        pSpdLbl->setText(QObject::tr("Enemy speed level: %1").arg(value));
    });
    connect(pUpSlider, &QSlider::valueChanged, [pUpLbl](int value) {
        pUpLbl->setText(QObject::tr("Upgrade interval (sec): %1").arg(value));
    });

    connect(pSaveBtn, &QPushButton::clicked, this, [this, pDialog, pMaxSlider, pSpdSlider, pUpSlider, pBonus, pReward]() {
        m_config.maxEnemies = pMaxSlider->value();
        m_config.enemySpeedLevel = pSpdSlider->value();
        m_config.upgradeIntervalSec = pUpSlider->value();
        m_config.bonusMode = pBonus->isChecked();
        m_config.rewardMode = pReward->isChecked();
        saveSettings();
        pDialog->accept();
    });
    connect(pCancelBtn, &QPushButton::clicked, pDialog, &QDialog::reject);
    connect(pDialog, &QDialog::finished, this, [this, previousState]() {
        if (previousState == KCTSpaceWarStateData::PlayingGameState
            && m_controller.state() == KCTSpaceWarStateData::PausedGameState) {
            setGameState(KCTSpaceWarStateData::PlayingGameState);
        }
        setFocus();
    });

    pDialog->exec();
}

void KCTSpaceWarGame::loadSettings()
{
    QSettings settings(QStringLiteral("TypingGame"), QStringLiteral("SpaceWar"));
    m_config.maxEnemies = settings.value(QStringLiteral("MaxEnemies"), 5).toInt();
    m_config.enemySpeedLevel = settings.value(QStringLiteral("EnemySpeedLevel"), 5).toInt();
    m_config.upgradeIntervalSec = settings.value(QStringLiteral("UpgradeIntervalSec"), 10).toInt();
    m_config.bonusMode = settings.value(QStringLiteral("BonusMode"), false).toBool();
    m_config.rewardMode = settings.value(QStringLiteral("RewardMode"), false).toBool();
    m_config.spawnRatePercent = settings.value(QStringLiteral("SpawnRatePercent"), 6).toInt();

    m_config.maxEnemies = qBound(1, m_config.maxEnemies, 10);
    m_config.enemySpeedLevel = qBound(1, m_config.enemySpeedLevel, 10);
    m_config.upgradeIntervalSec = qBound(3, m_config.upgradeIntervalSec, 120);
    m_config.spawnRatePercent = qBound(3, m_config.spawnRatePercent, 15);
}

void KCTSpaceWarGame::saveSettings()
{
    QSettings settings(QStringLiteral("TypingGame"), QStringLiteral("SpaceWar"));
    settings.setValue(QStringLiteral("MaxEnemies"), m_config.maxEnemies);
    settings.setValue(QStringLiteral("EnemySpeedLevel"), m_config.enemySpeedLevel);
    settings.setValue(QStringLiteral("UpgradeIntervalSec"), m_config.upgradeIntervalSec);
    settings.setValue(QStringLiteral("BonusMode"), m_config.bonusMode);
    settings.setValue(QStringLiteral("RewardMode"), m_config.rewardMode);
    settings.setValue(QStringLiteral("SpawnRatePercent"), m_config.spawnRatePercent);
}
