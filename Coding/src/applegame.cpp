/* -------------------------------------------------------------------------
//  文件名      :  applegame.cpp
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Save Apples view: rendering, timers, settings dialog, audio and telemetry hooks.
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#include "applegame.h"

#include <QCloseEvent>
#include <QComboBox>
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
#include <QSettings>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>

#include "config/resourceconfig.h"
#include "services/asynclogger.h"
#include "services/eventtracker.h"
#include "services/gameaudioservice.h"
#include "services/sfxid.h"

KCTAppleGame::KCTAppleGame(QWidget *parent)
    : QWidget(parent)
    , m_timer(new QTimer(this))
    , m_messageLabel(nullptr)
    , m_exitButton(nullptr)
    , m_settingsButton(nullptr)
    , m_pauseButton(nullptr)
{
    setFixedSize(s_gameWidth, s_gameHeight);
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet(KCTResourceConfig::gameBackgroundStyle());
    setWindowTitle(KCTResourceConfig::titleIdle());

    loadSettings();

    const QString appDir = QCoreApplication::applicationDirPath();
    m_logger = std::make_unique<KCTAsyncLogger>(appDir);
    m_tracker = std::make_unique<KCTEventTracker>(appDir);
    m_audio = std::make_unique<KCTGameAudioService>(appDir);

    m_exitButton = new QPushButton(tr("Exit to Menu"), this);
    m_exitButton->setGeometry(s_gameWidth - 100, 10, 80, 30);
    m_exitButton->setStyleSheet(KCTResourceConfig::exitButtonStyle());
    connect(m_exitButton, &QPushButton::clicked, this, &KCTAppleGame::onExitClicked);

    m_settingsButton = new QPushButton(tr("Settings"), this);
    m_settingsButton->setGeometry(s_gameWidth - 190, 10, 80, 30);
    m_settingsButton->setStyleSheet(KCTResourceConfig::settingsButtonStyle());
    connect(m_settingsButton, &QPushButton::clicked, this, &KCTAppleGame::onSettingsClicked);

    m_pauseButton = new QPushButton(tr("Pause"), this);
    m_pauseButton->setGeometry(s_gameWidth - 280, 10, 80, 30);
    m_pauseButton->setStyleSheet(KCTResourceConfig::pauseButtonStyle());
    connect(m_pauseButton, &QPushButton::clicked, this, [this]() {
        if (m_controller.state() == KCTGameStateData::PlayingGameState) {
            setGameState(KCTGameStateData::PausedGameState);
        } else if (m_controller.state() == KCTGameStateData::PausedGameState) {
            setGameState(KCTGameStateData::PlayingGameState);
        }
    });

    // Avoid Space/Enter activating buttons as "click" — keep gameplay keys on KCTAppleGame.
    m_exitButton->setFocusPolicy(Qt::NoFocus);
    m_settingsButton->setFocusPolicy(Qt::NoFocus);
    m_pauseButton->setFocusPolicy(Qt::NoFocus);

    connect(m_timer, &QTimer::timeout, this, &KCTAppleGame::updateGame);
}

KCTAppleGame::~KCTAppleGame()
{
    if (m_timer) {
        m_timer->stop();
    }
    clearAppleLabels();
}

void KCTAppleGame::setGameState(GameState newState)
{
    const GameState oldState = m_controller.state();
    m_controller.setState(newState);
    const GameState now = m_controller.state();

    if (m_audio) {
        if (oldState == KCTGameStateData::PlayingGameState && now == KCTGameStateData::PausedGameState) {
            m_audio->pauseBgm();
        } else if (oldState == KCTGameStateData::PausedGameState && now == KCTGameStateData::PlayingGameState) {
            m_audio->resumeBgm();
        } else if (now == KCTGameStateData::IdleGameState || now == KCTGameStateData::EndGameState) {
            m_audio->stopBgm();
        }
    }

    if ((oldState == KCTGameStateData::PlayingGameState && now == KCTGameStateData::PausedGameState)
        || (oldState == KCTGameStateData::PausedGameState && now == KCTGameStateData::PlayingGameState)) {
        trackEvent(QStringLiteral("pause_toggle"));
        logLine(QStringLiteral("pause_toggle"));
    }

    switch (now) {
    case KCTGameStateData::IdleGameState:
        m_timer->stop();
        setWindowTitle(KCTResourceConfig::titleIdle());
        break;
    case KCTGameStateData::PlayingGameState:
        if (!m_timer->isActive()) {
            m_timer->start(50);
        }
        updateUI();
        break;
    case KCTGameStateData::PausedGameState:
        m_timer->stop();
        setWindowTitle(KCTResourceConfig::titlePaused());
        break;
    case KCTGameStateData::EndGameState:
        m_timer->stop();
        setWindowTitle(KCTResourceConfig::titleEnd(m_controller.data().level));
        break;
    }
    update();
}

KCTAppleGame::GameState KCTAppleGame::gameState() const
{
    return m_controller.state();
}

void KCTAppleGame::startGame()
{
    loadSettings();
    applySettings();

    m_controller.startGame(m_config);
    createInitialApples();
    clearAppleLabels();
    syncAppleLabels();
    setGameState(KCTGameStateData::PlayingGameState);
    trackEvent(QStringLiteral("game_start"));
    logLine(QStringLiteral("game_start"));
    if (m_audio) {
        m_audio->startBgmIfAvailable();
    }
    setFocus();
}

void KCTAppleGame::createInitialApples()
{
    m_controller.ensureMinimumApples(2, m_config, s_gameWidth, s_appleSize);
}

void KCTAppleGame::syncAppleLabels()
{
    const auto& apples = m_controller.data().apples;
    while (m_appleLabels.size() > apples.size()) {
        QLabel* pLabel = m_appleLabels.takeLast();
        delete pLabel;
    }
    while (m_appleLabels.size() < apples.size()) {
        QLabel* pLabel = new QLabel(this);
        pLabel->setFixedSize(s_appleSize, s_appleSize);
        pLabel->setAlignment(Qt::AlignCenter);
        pLabel->setStyleSheet(KCTResourceConfig::appleStyle());
        pLabel->show();
        m_appleLabels.append(pLabel);
    }

    for (int i = 0; i < apples.size(); ++i) {
        m_appleLabels[i]->setText(QString(apples[i].letter));
        m_appleLabels[i]->move(apples[i].x, apples[i].y);
    }
}

void KCTAppleGame::clearAppleLabels()
{
    for (QLabel* pLabel : m_appleLabels) {
        delete pLabel;
    }
    m_appleLabels.clear();
}

void KCTAppleGame::updateGame()
{
    if (m_controller.state() != KCTGameStateData::PlayingGameState) {
        return;
    }

    const int nFailsBefore = m_controller.data().failCount;
    m_controller.tick(m_config, s_gameWidth, s_appleSize, s_failLine);
    if (m_controller.data().failCount > nFailsBefore) {
        if (m_audio) {
            m_audio->playSfx(KCTSfxId::MissSfxId);
        }
        trackEvent(QStringLiteral("apple_reach_failline"));
        logLine(QStringLiteral("apple_reach_failline"));
    }
    syncAppleLabels();
    updateUI();
}

void KCTAppleGame::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Space:
        if (m_controller.state() == KCTGameStateData::PlayingGameState) {
            setGameState(KCTGameStateData::PausedGameState);
        } else if (m_controller.state() == KCTGameStateData::PausedGameState) {
            setGameState(KCTGameStateData::PlayingGameState);
        }
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (m_controller.state() == KCTGameStateData::IdleGameState) {
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

    if (m_controller.handleLetterInput(key, m_config, s_gameWidth, s_appleSize)) {
        if (m_controller.data().levelComplete) {
            if (m_audio) {
                m_audio->playSfx(KCTSfxId::LevelCompleteSfxId);
            }
            trackEvent(QStringLiteral("level_complete"));
            logLine(QStringLiteral("level_complete"));
            m_timer->stop();
            showLevelCompleteMessage();
            QTimer::singleShot(2000, this, [this]() {
                if (m_messageLabel) {
                    delete m_messageLabel;
                    m_messageLabel = nullptr;
                }
                m_controller.prepareNextLevel(m_config, s_gameWidth, s_appleSize);
                syncAppleLabels();
                updateUI();
                if (m_controller.state() == KCTGameStateData::PlayingGameState) {
                    m_timer->start(50);
                }
            });
        } else {
            if (m_audio) {
                m_audio->playSfx(KCTSfxId::HitSfxId);
            }
            trackEvent(QStringLiteral("key_hit_success"));
            logLine(QStringLiteral("key_hit_success"));
            syncAppleLabels();
            updateUI();
        }
    }
}

void KCTAppleGame::showLevelCompleteMessage()
{
    if (m_messageLabel) {
        delete m_messageLabel;
    }
    m_messageLabel = new QLabel(KCTResourceConfig::levelCompleteMessage(), this);
    m_messageLabel->setAlignment(Qt::AlignCenter);
    m_messageLabel->setStyleSheet(KCTResourceConfig::levelMessageStyle());
    m_messageLabel->setGeometry(150, s_gameHeight / 2 - 50, 500, 100);
    m_messageLabel->show();
}

void KCTAppleGame::updateUI()
{
    if (m_controller.state() != KCTGameStateData::PlayingGameState) {
        return;
    }

    const auto& data = m_controller.data();
    const int nTotalAttempts = data.score + data.failCount;
    const int nAccuracy = nTotalAttempts > 0 ? (data.score * 100) / nTotalAttempts : 0;
    setWindowTitle(KCTResourceConfig::titlePlaying(
        data.level,
        data.targetScore,
        data.score,
        data.failCount,
        nAccuracy,
        data.speed));
}

void KCTAppleGame::onExitClicked()
{
    if (m_timer) {
        m_timer->stop();
    }
    close();
}

void KCTAppleGame::closeEvent(QCloseEvent* event)
{
    if (m_timer) {
        m_timer->stop();
    }
    trackEvent(QStringLiteral("game_exit"));
    logLine(QStringLiteral("game_exit"));
    if (m_audio) {
        m_audio->stopBgm();
    }
    clearAppleLabels();
    event->accept();
}

void KCTAppleGame::logLine(const QString& message)
{
    if (m_logger) {
        m_logger->log(message);
    }
}

void KCTAppleGame::trackEvent(const QString& eventName)
{
    if (!m_tracker) {
        return;
    }

    QJsonObject jsonObject;
    jsonObject[QStringLiteral("unix_ms")] = QDateTime::currentMSecsSinceEpoch();
    jsonObject[QStringLiteral("event")] = eventName;

    const auto& d = m_controller.data();
    jsonObject[QStringLiteral("level")] = d.level;
    jsonObject[QStringLiteral("score")] = d.score;
    jsonObject[QStringLiteral("fail_count")] = d.failCount;
    jsonObject[QStringLiteral("game_state")] = int(d.state);

    const QJsonDocument doc(jsonObject);
    m_tracker->trackJsonLine(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

void KCTAppleGame::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), QColor(30, 30, 40));
    painter.setPen(QPen(Qt::red, 3, Qt::SolidLine));
    painter.drawLine(0, s_failLine, s_gameWidth, s_failLine);

    painter.setPen(QPen(Qt::red, 1, Qt::SolidLine));
    painter.setFont(QFont("Arial", 10));
    painter.drawText(10, s_failLine - 5, KCTResourceConfig::failLineText());

    if (m_controller.state() == KCTGameStateData::IdleGameState) {
        painter.setPen(QPen(Qt::white, 1, Qt::SolidLine));
        painter.setFont(QFont("Arial", 20));
        painter.drawText(s_gameWidth / 2 - 200, s_gameHeight / 2 - 50, KCTResourceConfig::pressEnterText());
    }

    if (m_controller.state() == KCTGameStateData::PausedGameState) {
        painter.setPen(QPen(Qt::yellow, 1, Qt::SolidLine));
        painter.setFont(QFont("Arial", 20));
        painter.drawText(s_gameWidth / 2 - 150, s_gameHeight / 2 - 50, KCTResourceConfig::pausedHintText());
    }
}

void KCTAppleGame::onSettingsClicked()
{
    GameState previousState = m_controller.state();
    if (m_controller.state() == KCTGameStateData::PlayingGameState) {
        setGameState(KCTGameStateData::PausedGameState);
    }

    QDialog* pSettingsDialog = new QDialog(this);
    pSettingsDialog->setWindowTitle(tr("Game Settings"));
    pSettingsDialog->setFixedSize(300, 350);
    pSettingsDialog->setStyleSheet(KCTResourceConfig::dialogStyle());
    pSettingsDialog->setModal(true);

    QVBoxLayout* pLayout = new QVBoxLayout(pSettingsDialog);
    pLayout->addWidget(new QLabel(tr("Difficulty:")));
    QComboBox* pDifficultyCombo = new QComboBox();
    pDifficultyCombo->addItem(tr("Easy"));
    pDifficultyCombo->addItem(tr("Normal"));
    pDifficultyCombo->addItem(tr("Hard"));
    pDifficultyCombo->setCurrentIndex(1);
    pLayout->addWidget(pDifficultyCombo);

    pLayout->addSpacing(20);
    pLayout->addWidget(new QLabel(tr("Initial Speed: %1").arg(m_config.baseSpeed)));
    QSlider* pSpeedSlider = new QSlider(Qt::Horizontal);
    pSpeedSlider->setRange(1, 5);
    pSpeedSlider->setValue(m_config.baseSpeed);
    pLayout->addWidget(pSpeedSlider);

    pLayout->addWidget(new QLabel(tr("Target Score per Level: %1").arg(m_config.baseTarget)));
    QSlider* pTargetSlider = new QSlider(Qt::Horizontal);
    pTargetSlider->setRange(3, 10);
    pTargetSlider->setValue(m_config.baseTarget);
    pLayout->addWidget(pTargetSlider);

    pLayout->addWidget(new QLabel(tr("Max Apples on Screen: %1").arg(m_config.maxApples)));
    QSlider* pMaxApplesSlider = new QSlider(Qt::Horizontal);
    pMaxApplesSlider->setRange(3, 8);
    pMaxApplesSlider->setValue(m_config.maxApples);
    pLayout->addWidget(pMaxApplesSlider);

    pLayout->addSpacing(20);
    QHBoxLayout* pButtonLayout = new QHBoxLayout();
    QPushButton* pSaveBtn = new QPushButton(tr("Save"));
    QPushButton* pCancelBtn = new QPushButton(tr("Cancel"));
    pButtonLayout->addWidget(pSaveBtn);
    pButtonLayout->addWidget(pCancelBtn);
    pLayout->addLayout(pButtonLayout);

    connect(pSpeedSlider, &QSlider::valueChanged, [pSpeedSlider](int val) {
        QWidget* pParent = pSpeedSlider->parentWidget();
        QLabel* pLabel = pParent->findChildren<QLabel*>().value(1);
        if (pLabel) {
            pLabel->setText(QObject::tr("Initial Speed: %1").arg(val));
        }
    });
    connect(pTargetSlider, &QSlider::valueChanged, [pTargetSlider](int val) {
        QWidget* pParent = pTargetSlider->parentWidget();
        QLabel* pLabel = pParent->findChildren<QLabel*>().value(2);
        if (pLabel) {
            pLabel->setText(QObject::tr("Target Score per Level: %1").arg(val));
        }
    });
    connect(pMaxApplesSlider, &QSlider::valueChanged, [pMaxApplesSlider](int val) {
        QWidget* pParent = pMaxApplesSlider->parentWidget();
        QLabel* pLabel = pParent->findChildren<QLabel*>().value(3);
        if (pLabel) {
            pLabel->setText(QObject::tr("Max Apples on Screen: %1").arg(val));
        }
    });

    connect(pDifficultyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [pSpeedSlider, pTargetSlider, pMaxApplesSlider](int index) {
                if (index == 0) {
                    pSpeedSlider->setValue(1);
                    pTargetSlider->setValue(3);
                    pMaxApplesSlider->setValue(8);
                } else if (index == 1) {
                    pSpeedSlider->setValue(2);
                    pTargetSlider->setValue(5);
                    pMaxApplesSlider->setValue(6);
                } else {
                    pSpeedSlider->setValue(4);
                    pTargetSlider->setValue(8);
                    pMaxApplesSlider->setValue(4);
                }
            });

    connect(pSaveBtn, &QPushButton::clicked, [this, pSpeedSlider, pTargetSlider, pMaxApplesSlider, pSettingsDialog]() {
        m_config.baseSpeed = pSpeedSlider->value();
        m_config.baseTarget = pTargetSlider->value();
        m_config.maxApples = pMaxApplesSlider->value();
        saveSettings();
        pSettingsDialog->accept();
    });
    connect(pCancelBtn, &QPushButton::clicked, pSettingsDialog, &QDialog::reject);
    connect(pSettingsDialog, &QDialog::finished, this, [this, previousState]() {
        if (previousState == KCTGameStateData::PlayingGameState && m_controller.state() == KCTGameStateData::PausedGameState) {
            setGameState(KCTGameStateData::PlayingGameState);
        }
        setFocus();
    });

    pSettingsDialog->exec();
}

void KCTAppleGame::applySettings()
{
    auto& data = m_controller.data();
    if (m_controller.state() == KCTGameStateData::IdleGameState || m_controller.state() == KCTGameStateData::PlayingGameState) {
        data.speed = m_config.baseSpeed;
        data.targetScore = m_config.baseTarget;
    }
}

void KCTAppleGame::loadSettings()
{
    QSettings settings("TypingGame", "SaveApples");
    m_config.baseSpeed = settings.value("BaseSpeed", 2).toInt();
    m_config.baseTarget = settings.value("BaseTarget", 5).toInt();
    m_config.maxApples = settings.value("MaxApples", 6).toInt();
    m_config.spawnRate = settings.value("SpawnRate", 5).toInt();

    if (m_config.baseSpeed < 1) m_config.baseSpeed = 1;
    if (m_config.baseSpeed > 5) m_config.baseSpeed = 5;
    if (m_config.baseTarget < 3) m_config.baseTarget = 3;
    if (m_config.baseTarget > 10) m_config.baseTarget = 10;
    if (m_config.maxApples < 3) m_config.maxApples = 3;
    if (m_config.maxApples > 8) m_config.maxApples = 8;

    applySettings();
}

void KCTAppleGame::saveSettings()
{
    QSettings settings("TypingGame", "SaveApples");
    settings.setValue("BaseSpeed", m_config.baseSpeed);
    settings.setValue("BaseTarget", m_config.baseTarget);
    settings.setValue("MaxApples", m_config.maxApples);
    settings.setValue("SpawnRate", m_config.spawnRate);
}
