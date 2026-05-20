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
#include <QColor>
#include <QEventLoop>
#include <QPaintEvent>
#include <QPalette>
#include <QSignalBlocker>
#include <QFont>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSettings>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>

#include "config/resourceconfig.h"
#include "services/asynclogger.h"
#include "services/deepseekwordclient.h"
#include "services/eventtracker.h"
#include "spacewarassets.h"
#include "spacewaraudio.h"

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

// Transparent full-window shield: blocks clicks without a separate native dialog window.
class KCTSettingsOverlay final : public QWidget
{
public:
    explicit KCTSettingsOverlay(QWidget* parent) : QWidget(parent)
    {
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAutoFillBackground(false);
        setFocusPolicy(Qt::StrongFocus);
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event);
    }
};

void drawPixmapCentered(QPainter& painter, const QPixmap& pixmap, double centerX, double centerY, int targetW,
                        int targetH)
{
    if (pixmap.isNull() || targetW <= 0 || targetH <= 0) {
        return;
    }
    const QPixmap scaled = pixmap.scaled(targetW, targetH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    painter.drawPixmap(QPointF(centerX - scaled.width() * 0.5, centerY - scaled.height() * 0.5), scaled);
}
} // namespace (file-local tuning constants and reward topic list)

KCTSpaceWarGame::KCTSpaceWarGame(QWidget* parent)
    : QWidget(parent)
    , m_timer(new QTimer(this))
{
    setMinimumSize(400, 300);
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet(KCTResourceConfig::gameBackgroundStyle());

    loadSettings();

    const QString appDir = QCoreApplication::applicationDirPath();
    m_logger = std::make_unique<KCTAsyncLogger>(appDir);
    m_tracker = std::make_unique<KCTEventTracker>(appDir);
    m_audio = std::make_unique<KCTSpaceWarAudioService>(appDir);
    m_assets.load(appDir + QStringLiteral("/assets"));

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
    connect(m_exitButton, &QPushButton::clicked, this, &KCTSpaceWarGame::onExitClicked);

    m_settingsButton = new QPushButton(tr("Settings"), this);
    connect(m_settingsButton, &QPushButton::clicked, this, &KCTSpaceWarGame::onSettingsClicked);

    m_pauseButton = new QPushButton(tr("Pause"), this);
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

    m_viewport.updateFromClientSize(s_designWidth, s_designHeight);
    m_lastGameWidth = m_viewport.gameWidth();
    m_lastGameHeight = m_viewport.gameHeight();
    regenerateStars();
    layoutTopButtons();
}

KCTSpaceWarGame::~KCTSpaceWarGame()
{
    if (m_timer) {
        m_timer->stop();
    }
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
        m_controller.centerPlayer(gameWidth(), playerWidth());
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
        m_controller.centerPlayer(gameWidth(), playerWidth());
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
    m_controller.startGame(m_config, gameWidth(), playerWidth());
    m_prevEnemyCount = 0;
    m_prevExplosionCount = 0;
    m_prevUpgradeTier = 0;
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

void KCTSpaceWarGame::detectGameplayAudioEvents()
{
    if (!m_audio) {
        return;
    }
    const auto& data = m_controller.data();
    if (data.enemies.size() > m_prevEnemyCount) {
        m_audio->playSfx(KCTSpaceWarSfxId::PlaneOutSfxId);
    }
    if (data.explosions.size() > m_prevExplosionCount) {
        m_audio->playSfx(KCTSpaceWarSfxId::BlastSfxId);
    }
    if (data.upgradeTier > m_prevUpgradeTier) {
        m_audio->playSfx(KCTSpaceWarSfxId::UpgradeSfxId);
    }
    m_prevEnemyCount = data.enemies.size();
    m_prevExplosionCount = data.explosions.size();
    m_prevUpgradeTier = data.upgradeTier;
}

void KCTSpaceWarGame::updateGame()
{
    if (m_controller.state() != KCTSpaceWarStateData::PlayingGameState) {
        return;
    }

    const int nHealthBefore = m_controller.data().health;
    m_controller.tick(m_config, gs_timerTickMilliseconds, gameWidth(), gameHeight(), enemySize(), playerWidth(),
                      playerHeight(), anchorPlayerY());

    if (m_controller.data().health < nHealthBefore) {
        trackEvent(QStringLiteral("health_loss"));
        logLine(QStringLiteral("health_loss"));
    }

    detectGameplayAudioEvents();

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
                m_controller.spawnFlyingRewardWord(w, gameWidth(), gameHeight());
                if (m_audio) {
                    m_audio->playSfx(KCTSpaceWarSfxId::WordOutSfxId);
                }
                logLine(QStringLiteral("reward_cache spawn word=%1 queue_after=%2 http_pending=%3")
                            .arg(w)
                            .arg(m_rewardWordQueue.size())
                            .arg(m_rewardRequestPending ? 1 : 0));
            }
        }
        maybeRequestRewardWordBatch();
    }

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
                m_audio->playSfx(KCTSpaceWarSfxId::BlastSfxId);
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

    if (m_controller.handleLetterInput(key, m_config, gameWidth(), enemySize(), playerWidth(), playerHeight(),
                                     anchorPlayerY())) {
        if (m_audio) {
            m_audio->playSfx(KCTSpaceWarSfxId::ShootSfxId);
        }
        trackEvent(QStringLiteral("key_hit_success"));
        logLine(QStringLiteral("key_hit_success"));
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

void KCTSpaceWarGame::drawEnemies(QPainter& painter) const
{
    const int nEnemyPx = enemySize();
    const int nCaptionW = m_viewport.scaled(27);
    const int nCaptionH = m_viewport.scaled(21);
    const int nLetterFont = m_viewport.scaled(16);
    QFont letterFont(QStringLiteral("Arial"), nLetterFont, QFont::Bold);

    for (const KCTEnemyEntity& enemy : m_controller.data().enemies) {
        const QPixmap& sprite = enemy.isMeteor ? m_assets.enemyMeteor() : m_assets.enemyPlane();
        const double ecx = enemy.x + nEnemyPx * 0.5;
        const double ecy = enemy.y + nEnemyPx * 0.5;
        if (!sprite.isNull()) {
            drawPixmapCentered(painter, sprite, ecx, ecy, nEnemyPx, nEnemyPx);
        } else {
            painter.setBrush(enemy.isMeteor ? QColor(140, 120, 100) : QColor(120, 140, 180));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(QPointF(ecx, ecy), nEnemyPx * 0.45, nEnemyPx * 0.45);
        }

        const double capY = enemy.y + nEnemyPx * 0.22;
        if (!m_assets.captionBack().isNull()) {
            drawPixmapCentered(painter, m_assets.captionBack(), ecx, capY, nCaptionW, nCaptionH);
        }
        painter.setFont(letterFont);
        painter.setPen(QPen(Qt::white));
        painter.drawText(QRectF(enemy.x, capY - nCaptionH * 0.5, nEnemyPx, nCaptionH), Qt::AlignCenter,
                         QString(enemy.letter));
    }
}

void KCTSpaceWarGame::drawHud(QPainter& painter) const
{
    const auto& data = m_controller.data();
    const int labelH = m_viewport.scaled(22);
    const int hudY = m_viewport.scaled(50);

    if (!m_assets.labelLife().isNull()) {
        const int labelW = m_viewport.scaled(m_assets.labelLife().width());
        painter.drawPixmap(m_viewport.scaled(14), hudY, labelW, labelH, m_assets.labelLife());
    }

    const int barX = m_viewport.scaled(58);
    const int barY = hudY + m_viewport.scaled(1);
    const int barW = m_viewport.scaled(m_assets.lifeBarEmpty().isNull() ? 181 : m_assets.lifeBarEmpty().width());
    const int barH = m_viewport.scaled(m_assets.lifeBarEmpty().isNull() ? 22 : m_assets.lifeBarEmpty().height());
    if (!m_assets.lifeBarEmpty().isNull()) {
        painter.drawPixmap(barX, barY, barW, barH, m_assets.lifeBarEmpty());
    }
    if (!m_assets.lifeBarFill().isNull() && data.health > 0) {
        const int fillW = barW * data.health / KCTSpaceWarController::maxHealthValue;
        const QPixmap fillStrip = m_assets.lifeBarFill().scaled(fillW, barH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        painter.drawPixmap(barX, barY, fillStrip);
    }

    if (!m_assets.labelScore().isNull()) {
        const int scoreLabelX = m_viewport.scaled(300);
        const int labelW = m_viewport.scaled(m_assets.labelScore().width());
        painter.drawPixmap(scoreLabelX, hudY, labelW, labelH, m_assets.labelScore());
        painter.setPen(Qt::white);
        painter.setFont(QFont(QStringLiteral("Arial"), m_viewport.scaled(14), QFont::Bold));
        painter.drawText(QRect(scoreLabelX + labelW + m_viewport.scaled(6), hudY, m_viewport.scaled(80), labelH),
                         Qt::AlignVCenter | Qt::AlignLeft, QString::number(data.score));
    }

    if (!m_assets.labelTime().isNull()) {
        const int timeLabelX = m_viewport.scaled(620);
        const int labelW = m_viewport.scaled(m_assets.labelTime().width());
        painter.drawPixmap(timeLabelX, hudY, labelW, labelH, m_assets.labelTime());
        const qint64 totalSec = data.playingElapsedMs / 1000;
        const QString timeText =
            QStringLiteral("%1:%2")
                .arg(totalSec / 60, 2, 10, QChar('0'))
                .arg(totalSec % 60, 2, 10, QChar('0'));
        painter.setPen(Qt::white);
        painter.setFont(QFont(QStringLiteral("Arial"), m_viewport.scaled(14), QFont::Bold));
        painter.drawText(QRect(timeLabelX + labelW + m_viewport.scaled(4), hudY, m_viewport.scaled(70), labelH),
                         Qt::AlignVCenter | Qt::AlignLeft, timeText);
    }
}

void KCTSpaceWarGame::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0));

    painter.save();
    painter.translate(m_viewport.offsetX(), m_viewport.offsetY());

    const int gw = gameWidth();
    const int gh = gameHeight();
    if (!m_assets.background().isNull()) {
        painter.drawPixmap(0, 0, gw, gh,
                           m_assets.background().scaled(gw, gh, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    } else {
        painter.fillRect(0, 0, gw, gh, QColor(12, 18, 42));
    }

    const QVector<QPixmap>& starPixels = m_assets.starPixels();
    for (int i = 0; i < m_stars.size(); ++i) {
        const QPoint& starPosition = m_stars[i];
        if (!starPixels.isEmpty()) {
            const QPixmap& starPix = starPixels.at(i % starPixels.size());
            const int starSize = m_viewport.scaled(2 + (i % 3));
            painter.drawPixmap(starPosition.x(), starPosition.y(), starSize, starSize, starPix);
        } else {
            painter.setPen(QPen(QColor(220, 220, 255, 40), 1));
            painter.drawPoint(starPosition);
        }
    }

    const auto& data = m_controller.data();

    const int nPlayerY = anchorPlayerY();
    const int nPlayerW = playerWidth();
    const int nPlayerH = playerHeight();
    const double playerX = data.playerX;
    const double shipCx = playerX + nPlayerW * 0.5;
    const double shipCy = nPlayerY + nPlayerH * 0.5;
    const int shipDrawW = m_viewport.scaled(56);
    const int shipDrawH = m_viewport.scaled(64);
    if (!m_assets.ship().isNull()) {
        drawPixmapCentered(painter, m_assets.ship(), shipCx, shipCy, shipDrawW, shipDrawH);
    } else {
        QPolygonF ship;
        ship << QPointF(playerX + nPlayerW * 0.5, nPlayerY)
             << QPointF(playerX + m_viewport.scaled(4), nPlayerY + nPlayerH)
             << QPointF(playerX + nPlayerW - m_viewport.scaled(4), nPlayerY + nPlayerH);
        painter.setBrush(QColor(72, 200, 255));
        painter.setPen(QPen(QColor(180, 240, 255), qMax(1, m_viewport.scaled(2))));
        painter.drawPolygon(ship);
    }

    drawEnemies(painter);

    const int nBombSize = m_viewport.scaled(m_assets.bomb().isNull() ? 10 : m_assets.bomb().width());
    for (const KCTBulletEntity& bullet : data.bullets) {
        if (!m_assets.bomb().isNull()) {
            drawPixmapCentered(painter, m_assets.bomb(), bullet.x, bullet.y, nBombSize, nBombSize);
        } else {
            const double bulletRadius = m_viewport.scaled(4.0);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(255, 235, 120));
            painter.drawEllipse(QPointF(bullet.x, bullet.y), bulletRadius, bulletRadius);
        }
    }

    const int nExplosionSize = m_viewport.scaled(64);
    for (const KCTExplosionEntity& explosion : data.explosions) {
        const QPixmap frame = m_assets.explosionFrame(explosion.frame);
        if (!frame.isNull()) {
            drawPixmapCentered(painter, frame, explosion.x, explosion.y, nExplosionSize, nExplosionSize);
        } else {
            const int nFrame = explosion.frame;
            const double radius = m_viewport.scaled(6.0 + static_cast<double>(nFrame) * 8.0);
            const int nAlpha = qMax(30, 230 - nFrame * 26);
            painter.setBrush(QColor(255, 140 - nFrame * 12, 30, nAlpha));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(QPointF(explosion.x, explosion.y), radius, radius * 0.85);
        }
    }

    if (m_controller.state() == KCTSpaceWarStateData::PlayingGameState
        || m_controller.state() == KCTSpaceWarStateData::PausedGameState) {
        drawHud(painter);
    }

    if (m_controller.hasActiveFlyingRewardWord()) {
        const auto& rw = m_controller.data().flyingRewardWord;
        QFont rewardFont(QStringLiteral("Arial"), m_viewport.scaled(22), QFont::Bold);
        painter.setFont(rewardFont);
        const int nTyped = m_controller.data().rewardTypingIndex;
        const QString& full = rw.text;
        double x = rw.x;
        const int letterSpacing = m_viewport.scaled(2);
        for (int i = 0; i < full.size(); ++i) {
            const QString ch = full.mid(i, 1);
            if (i < nTyped) {
                painter.setPen(QPen(QColor(120, 255, 200), qMax(1, m_viewport.scaled(2))));
            } else {
                painter.setPen(QPen(QColor(255, 250, 220), qMax(1, m_viewport.scaled(2))));
            }
            painter.drawText(QPointF(x, rw.y), ch);
            x += painter.fontMetrics().horizontalAdvance(ch) + letterSpacing;
        }
    }

    if (m_controller.state() == KCTSpaceWarStateData::IdleGameState) {
        painter.setPen(QPen(Qt::white, 1));
        painter.setFont(QFont(QStringLiteral("Arial"), m_viewport.scaled(20)));
        painter.drawText(QRect(0, gh / 2 - m_viewport.scaled(40), gw, m_viewport.scaled(60)), Qt::AlignCenter,
                         tr("Press ENTER to start"));
    } else if (m_controller.state() == KCTSpaceWarStateData::PausedGameState) {
        painter.setPen(QPen(QColor(255, 220, 120), 1));
        painter.setFont(QFont(QStringLiteral("Arial"), m_viewport.scaled(20)));
        painter.drawText(QRect(0, gh / 2 - m_viewport.scaled(40), gw, m_viewport.scaled(60)), Qt::AlignCenter,
                         tr("PAUSED — SPACE to resume"));
    } else if (m_controller.state() == KCTSpaceWarStateData::EndGameState) {
        painter.setPen(QPen(QColor(255, 120, 120), 1));
        painter.setFont(QFont(QStringLiteral("Arial"), m_viewport.scaled(22)));
        painter.drawText(QRect(0, gh / 2 - m_viewport.scaled(50), gw, m_viewport.scaled(100)), Qt::AlignCenter,
                         tr("GAME OVER\nPress ENTER to retry"));
    }

    painter.restore();
}

void KCTSpaceWarGame::onSettingsClicked()
{
    const bool resumeAfterDialog = (m_controller.state() == KCTSpaceWarStateData::PlayingGameState);
    if (resumeAfterDialog) {
        m_timer->stop();
        if (m_audio) {
            m_audio->pauseBgm();
        }
    }

    KCTSettingsOverlay overlay(this);
    overlay.setGeometry(rect());
    overlay.raise();

    QWidget panel(&overlay);
    panel.setFixedSize(360, 360);
    panel.setAutoFillBackground(false);
    panel.move((overlay.width() - panel.width()) / 2, (overlay.height() - panel.height()) / 2);
    panel.raise();

    auto* pPanelBg = new QLabel(&panel);
    pPanelBg->setGeometry(0, 0, 360, 360);
    pPanelBg->setScaledContents(true);
    if (!m_assets.settingsBackground().isNull()) {
        pPanelBg->setPixmap(m_assets.settingsBackground().scaled(360, 360, Qt::KeepAspectRatioByExpanding,
                                                                 Qt::SmoothTransformation));
    } else {
        pPanelBg->setStyleSheet(KCTResourceConfig::dialogStyle());
    }
    pPanelBg->lower();

    auto* pLayout = new QVBoxLayout(&panel);
    pLayout->setContentsMargins(16, 16, 16, 16);

    const QString settingsLabelStyle = QStringLiteral("color: white; background: transparent;");
    auto* pMaxLbl = new QLabel(tr("Max enemies on screen: %1").arg(m_config.maxEnemies), &panel);
    pMaxLbl->setStyleSheet(settingsLabelStyle);
    auto* pMaxSlider = new QSlider(Qt::Horizontal, &panel);
    pMaxSlider->setRange(1, 10);
    {
        const QSignalBlocker blocker(pMaxSlider);
        pMaxSlider->setValue(m_config.maxEnemies);
    }

    auto* pSpdLbl = new QLabel(tr("Enemy speed level: %1").arg(m_config.enemySpeedLevel), &panel);
    pSpdLbl->setStyleSheet(settingsLabelStyle);
    auto* pSpdSlider = new QSlider(Qt::Horizontal, &panel);
    pSpdSlider->setRange(1, 10);
    {
        const QSignalBlocker blocker(pSpdSlider);
        pSpdSlider->setValue(m_config.enemySpeedLevel);
    }

    auto* pUpLbl = new QLabel(tr("Upgrade interval (sec): %1").arg(m_config.upgradeIntervalSec), &panel);
    pUpLbl->setStyleSheet(settingsLabelStyle);
    auto* pUpSlider = new QSlider(Qt::Horizontal, &panel);
    pUpSlider->setRange(3, 45);
    {
        const QSignalBlocker blocker(pUpSlider);
        pUpSlider->setValue(m_config.upgradeIntervalSec);
    }

    auto* pBonus = new QCheckBox(tr("Bonus mode (double score)"), &panel);
    pBonus->setChecked(m_config.bonusMode);
    pBonus->setStyleSheet(settingsLabelStyle);

    auto* pReward = new QCheckBox(tr("Reward mode (flying words, heal on full match)"), &panel);
    pReward->setChecked(m_config.rewardMode);
    pReward->setStyleSheet(settingsLabelStyle + QStringLiteral(" QCheckBox::indicator { width: 0px; height: 0px; }"));
    const QSize checkboxIconSize(m_assets.checkboxUnchecked().isNull() ? QSize(40, 20)
                                                                       : m_assets.checkboxUnchecked().size());
    pReward->setIconSize(checkboxIconSize);
    const auto updateRewardCheckboxIcon = [pReward, this](bool checked) {
        const QPixmap iconPixmap =
            checked ? m_assets.checkboxChecked() : m_assets.checkboxUnchecked();
        if (!iconPixmap.isNull()) {
            pReward->setIcon(QIcon(iconPixmap));
        }
    };
    updateRewardCheckboxIcon(pReward->isChecked());
    connect(pReward, &QCheckBox::toggled, pReward, updateRewardCheckboxIcon);

    pLayout->addWidget(pMaxLbl);
    pLayout->addWidget(pMaxSlider);
    pLayout->addWidget(pSpdLbl);
    pLayout->addWidget(pSpdSlider);
    pLayout->addWidget(pUpLbl);
    pLayout->addWidget(pUpSlider);
    pLayout->addWidget(pBonus);
    pLayout->addWidget(pReward);

    auto* pRow = new QHBoxLayout();
    auto* pSaveBtn = new QPushButton(tr("Save"), &panel);
    auto* pCancelBtn = new QPushButton(tr("Cancel"), &panel);
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

    QEventLoop loop;
    connect(pSaveBtn, &QPushButton::clicked, this, [&]() {
        m_config.maxEnemies = pMaxSlider->value();
        m_config.enemySpeedLevel = pSpdSlider->value();
        m_config.upgradeIntervalSec = pUpSlider->value();
        m_config.bonusMode = pBonus->isChecked();
        m_config.rewardMode = pReward->isChecked();
        saveSettings();
        loop.quit();
    });
    connect(pCancelBtn, &QPushButton::clicked, &loop, &QEventLoop::quit);

    overlay.show();
    panel.show();
    panel.setFocus();
    loop.exec();

    if (resumeAfterDialog && m_controller.state() == KCTSpaceWarStateData::PlayingGameState) {
        if (!m_timer->isActive()) {
            m_timer->start(gs_timerTickMilliseconds);
        }
        if (m_audio) {
            m_audio->resumeBgm();
        }
    }
    setFocus();
}

void KCTSpaceWarGame::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    applyViewport();
}

void KCTSpaceWarGame::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    if (!m_initialSizeApplied) {
        resize(KCTGameViewport::defaultWindowSize());
        m_initialSizeApplied = true;
    }
    applyViewport();
}

int KCTSpaceWarGame::enemySize() const
{
    return m_viewport.scaled(s_designEnemySize);
}

int KCTSpaceWarGame::playerWidth() const
{
    return m_viewport.scaled(s_designPlayerWidth);
}

int KCTSpaceWarGame::playerHeight() const
{
    return m_viewport.scaled(s_designPlayerHeight);
}

int KCTSpaceWarGame::anchorPlayerY() const
{
    return m_viewport.scaled(s_designHeight - s_designPlayerAnchorOffset);
}

int KCTSpaceWarGame::gameWidth() const
{
    return m_viewport.gameWidth();
}

int KCTSpaceWarGame::gameHeight() const
{
    return m_viewport.gameHeight();
}

void KCTSpaceWarGame::layoutTopButtons()
{
    const int btnFont = m_viewport.scaled(12);
    m_exitButton->setGeometry(m_viewport.mapDesignX(s_designWidth - 88), m_viewport.mapDesignY(10),
                              m_viewport.scaled(72), m_viewport.scaled(28));
    m_exitButton->setStyleSheet(KCTResourceConfig::exitButtonStyle(btnFont));

    m_settingsButton->setGeometry(m_viewport.mapDesignX(s_designWidth - 178), m_viewport.mapDesignY(10),
                                  m_viewport.scaled(80), m_viewport.scaled(28));
    m_settingsButton->setStyleSheet(KCTResourceConfig::settingsButtonStyle(btnFont));

    m_pauseButton->setGeometry(m_viewport.mapDesignX(s_designWidth - 268), m_viewport.mapDesignY(10),
                               m_viewport.scaled(80), m_viewport.scaled(28));
    m_pauseButton->setStyleSheet(KCTResourceConfig::pauseButtonStyle(btnFont));
}

void KCTSpaceWarGame::regenerateStars()
{
    m_stars.resize(90);
    const int gw = qMax(1, gameWidth());
    const int gh = qMax(1, gameHeight());
    for (QPoint& starPosition : m_stars) {
        starPosition =
            QPoint(QRandomGenerator::global()->bounded(0, gw), QRandomGenerator::global()->bounded(0, gh));
    }
}

void KCTSpaceWarGame::relayoutControllerForViewport(int oldGameWidth, int oldGameHeight)
{
    const GameState state = m_controller.state();
    if (state == KCTSpaceWarStateData::PlayingGameState || state == KCTSpaceWarStateData::PausedGameState) {
        m_controller.rescaleEntitiesForGameSize(oldGameWidth, oldGameHeight, gameWidth(), gameHeight());
    } else {
        m_controller.centerPlayer(gameWidth(), playerWidth());
    }
}

void KCTSpaceWarGame::applyViewport()
{
    const int oldGameWidth = m_lastGameWidth;
    const int oldGameHeight = m_lastGameHeight;

    m_viewport.updateFromClientSize(width(), height());

    const int newGameWidth = gameWidth();
    const int newGameHeight = gameHeight();
    const bool gameSizeChanged =
        (oldGameWidth > 0 && oldGameHeight > 0 && (oldGameWidth != newGameWidth || oldGameHeight != newGameHeight));

    layoutTopButtons();

    if (gameSizeChanged) {
        relayoutControllerForViewport(oldGameWidth, oldGameHeight);
        regenerateStars();
    }

    m_lastGameWidth = newGameWidth;
    m_lastGameHeight = newGameHeight;

    update();
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
