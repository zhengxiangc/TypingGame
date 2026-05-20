/* -------------------------------------------------------------------------
//  文件名      :  mainwindow.cpp
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Kingsoft-style main menu: custom title bar and game cards.
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#include "mainwindow.h"

#include "applegame.h"
#include "spacewar/spacewargame.h"

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QFont>
#include <QPalette>
#include <QPushButton>
#include <QEvent>
#include <QSizePolicy>
#include <QVBoxLayout>

#include <functional>

namespace {

constexpr int kMenuWidth = 920;
constexpr int kMenuHeight = 640;
constexpr int kTitleBarHeight = 96;
constexpr int kLogoSize = 52;
constexpr int kTitleArtHeight = 58;
constexpr int kFooterHeight = 28;
constexpr int kThumbWidth = 180;
constexpr int kThumbHeight = 130;

const char kMenuBlue[] = "#4989e9";
const char kTitleBarStyle[] =
    "QWidget#menuTitleBar { background-color: #4989e9; }"
    "QLabel#menuTitleArt { background: transparent; }"
    "QPushButton#menuWinBtn {"
    "  background: transparent; color: white; border: none;"
    "  min-width: 44px; max-width: 44px; min-height: 36px; font-size: 16px;"
    "}"
    "QPushButton#menuWinBtn:hover { background-color: rgba(255,255,255,0.15); }";

const char kContentStyle[] =
    "QWidget#menuContent { background-color: white; }"
    "QLabel#menuSectionTitle { color: #333333; font-size: 14px; font-weight: bold; }"
    "QLabel#menuGameName { color: #333333; font-size: 13px; }"
    "QLabel#menuThumb {"
    "  background-color: #f0f0f0; border: 1px solid #cccccc;"
    "}";

const char kFooterStyle[] = "QWidget#menuFooter { background-color: #4989e9; }";

class KCTMenuTitleBar : public QWidget
{
public:
    explicit KCTMenuTitleBar(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setObjectName(QStringLiteral("menuTitleBar"));
        setFixedHeight(kTitleBarHeight);
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragOffset = event->globalPosition().toPoint() - window()->frameGeometry().topLeft();
            event->accept();
            return;
        }
        QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            QWidget* top = window();
            if (top && !top->isMaximized()) {
                top->move(event->globalPosition().toPoint() - m_dragOffset);
            }
            event->accept();
            return;
        }
        QWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        m_dragging = false;
        QWidget::mouseReleaseEvent(event);
    }

    void mouseDoubleClickEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            QWidget* top = window();
            if (top) {
                if (top->isMaximized()) {
                    top->showNormal();
                } else {
                    top->showMaximized();
                }
            }
            event->accept();
            return;
        }
        QWidget::mouseDoubleClickEvent(event);
    }

private:
    bool m_dragging = false;
    QPoint m_dragOffset;
};

class KCTGameCard : public QWidget
{
public:
    explicit KCTGameCard(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setCursor(Qt::PointingHandCursor);
    }

    void setClickHandler(const std::function<void()>& handler) { m_clickHandler = handler; }

protected:
    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && rect().contains(event->pos()) && m_clickHandler) {
            m_clickHandler();
            event->accept();
            return;
        }
        QWidget::mouseReleaseEvent(event);
    }

    void enterEvent(QEnterEvent* event) override
    {
        setStyleSheet(QStringLiteral("background-color: rgba(74, 144, 226, 0.08); border-radius: 4px;"));
        QWidget::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override
    {
        setStyleSheet(QString());
        QWidget::leaveEvent(event);
    }

private:
    std::function<void()> m_clickHandler;
};

QPushButton* makeWindowButton(const QString& text, const char* objectName, QWidget* parent)
{
    auto* btn = new QPushButton(text, parent);
    btn->setObjectName(QString::fromUtf8(objectName));
    btn->setFlat(true);
    btn->setFocusPolicy(Qt::NoFocus);
    btn->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; color: #ffffff; border: none; }"
        "QPushButton:hover { background-color: rgba(255,255,255,0.15); color: #ffffff; }"));
    QPalette palette = btn->palette();
    palette.setColor(QPalette::ButtonText, Qt::white);
    btn->setPalette(palette);
    return btn;
}

QPixmap scaleTitleArtPixmap(const QPixmap& source)
{
    if (source.isNull()) {
        return {};
    }
    return source.scaledToHeight(kTitleArtHeight, Qt::SmoothTransformation);
}

QPixmap placeholderTitleArt()
{
    QPixmap pixmap(320, kTitleArtHeight);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPixelSize(24);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(pixmap.rect(), Qt::AlignLeft | Qt::AlignVCenter,
                     QCoreApplication::translate("KCTMainWindow", "Kingsoft Typing Tutor 2016"));
    return pixmap;
}

} // namespace

KCTMainWindow::KCTMainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    resize(kMenuWidth, kMenuHeight);
    setMinimumSize(800, 600);
    setWindowTitle(tr("Kingsoft Typing Tutor 2016"));

    const QString assetsRoot = QCoreApplication::applicationDirPath() + QStringLiteral("/assets");
    m_menuAssets.load(assetsRoot);

    buildUi();
    applyMenuAssets();
}

KCTMainWindow::~KCTMainWindow() = default;

void KCTMainWindow::buildUi()
{
    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    m_titleBar = new KCTMenuTitleBar(central);
    m_titleBar->setStyleSheet(QString::fromUtf8(kTitleBarStyle));

    auto* titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(0);

    auto* brandArea = new QWidget(m_titleBar);
    brandArea->setStyleSheet(QStringLiteral("background: transparent;"));
    auto* brandLayout = new QHBoxLayout(brandArea);
    brandLayout->setContentsMargins(18, 12, 0, 12);
    brandLayout->setSpacing(12);

    m_logoLabel = new QLabel(brandArea);
    m_logoLabel->setFixedSize(kLogoSize, kLogoSize);
    m_logoLabel->setScaledContents(true);
    m_logoLabel->setStyleSheet(QStringLiteral("background: transparent;"));

    m_titleArtLabel = new QLabel(brandArea);
    m_titleArtLabel->setObjectName(QStringLiteral("menuTitleArt"));
    m_titleArtLabel->setFixedHeight(kTitleArtHeight);
    m_titleArtLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_titleArtLabel->setScaledContents(false);
    m_titleArtLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    brandLayout->addWidget(m_logoLabel);
    brandLayout->addWidget(m_titleArtLabel, 0, Qt::AlignVCenter);

    titleLayout->addWidget(brandArea, 0, Qt::AlignVCenter);
    titleLayout->addStretch(1);

    auto* btnBar = new QWidget(m_titleBar);
    btnBar->setStyleSheet(QStringLiteral("background: transparent;"));
    auto* btnLayout = new QHBoxLayout(btnBar);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(0);

    auto* minBtn = makeWindowButton(QStringLiteral("\u2212"), "menuWinBtn", btnBar);
    m_maximizeBtn = makeWindowButton(QStringLiteral("\u25a1"), "menuWinBtn", btnBar);
    auto* closeBtn = makeWindowButton(QStringLiteral("\u00d7"), "menuWinBtn", btnBar);

    btnLayout->addWidget(minBtn);
    btnLayout->addWidget(m_maximizeBtn);
    btnLayout->addWidget(closeBtn);

    titleLayout->addWidget(btnBar, 0, Qt::AlignTop | Qt::AlignRight);

    connect(minBtn, &QPushButton::clicked, this, &KCTMainWindow::onMinimizeClicked);
    connect(m_maximizeBtn, &QPushButton::clicked, this, &KCTMainWindow::onMaximizeClicked);
    connect(closeBtn, &QPushButton::clicked, this, &KCTMainWindow::onCloseClicked);

    auto* content = new QWidget(central);
    content->setObjectName(QStringLiteral("menuContent"));
    content->setStyleSheet(QString::fromUtf8(kContentStyle));

    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(32, 20, 32, 24);
    contentLayout->setSpacing(16);

    auto* sectionTitle = new QLabel(tr("\u25b6 Classic Games"), content);
    sectionTitle->setObjectName(QStringLiteral("menuSectionTitle"));

    auto* cardsRow = new QHBoxLayout();
    cardsRow->setSpacing(48);
    cardsRow->addStretch(1);

    const QSize thumbSize(kThumbWidth, kThumbHeight);

    auto setupCard = [this, content, thumbSize](QWidget*& cardOut, QLabel*& thumbOut,
                                                  const QString& gameName) {
        cardOut = new KCTGameCard(content);
        auto* cardLayout = new QVBoxLayout(cardOut);
        cardLayout->setContentsMargins(8, 8, 8, 8);
        cardLayout->setSpacing(8);
        cardLayout->setAlignment(Qt::AlignHCenter);

        thumbOut = new QLabel(cardOut);
        thumbOut->setObjectName(QStringLiteral("menuThumb"));
        thumbOut->setFixedSize(thumbSize);
        thumbOut->setAlignment(Qt::AlignCenter);
        thumbOut->setScaledContents(false);

        auto* nameLabel = new QLabel(gameName, cardOut);
        nameLabel->setObjectName(QStringLiteral("menuGameName"));
        nameLabel->setAlignment(Qt::AlignHCenter);

        cardLayout->addWidget(thumbOut, 0, Qt::AlignHCenter);
        cardLayout->addWidget(nameLabel, 0, Qt::AlignHCenter);
    };

    setupCard(m_saveAppleCard, m_saveAppleThumb, tr("Save Apples"));
    setupCard(m_spaceWarCard, m_spaceWarThumb, tr("Space War"));

    cardsRow->addWidget(m_saveAppleCard);
    cardsRow->addWidget(m_spaceWarCard);
    cardsRow->addStretch(1);

    contentLayout->addWidget(sectionTitle);
    contentLayout->addSpacing(8);
    contentLayout->addLayout(cardsRow);
    contentLayout->addStretch(1);

    auto* footer = new QWidget(central);
    footer->setObjectName(QStringLiteral("menuFooter"));
    footer->setFixedHeight(kFooterHeight);
    footer->setStyleSheet(QString::fromUtf8(kFooterStyle));

    rootLayout->addWidget(m_titleBar);
    rootLayout->addWidget(content, 1);
    rootLayout->addWidget(footer);

    static_cast<KCTGameCard*>(m_saveAppleCard)->setClickHandler([this]() { onSaveAppleClicked(); });
    static_cast<KCTGameCard*>(m_spaceWarCard)->setClickHandler([this]() { onSpaceWarClicked(); });
}

void KCTMainWindow::applyMenuAssets()
{
    const QSize thumbSize(kThumbWidth, kThumbHeight);

    if (!m_menuAssets.appIcon().isNull()) {
        m_logoLabel->setPixmap(
            m_menuAssets.appIcon().scaled(kLogoSize, kLogoSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        m_logoLabel->setPixmap(
            placeholderThumbnail(QSize(kLogoSize, kLogoSize), QColor(255, 200, 80), QString()));
    }

    QPixmap titleArt = scaleTitleArtPixmap(m_menuAssets.titleArt());
    if (titleArt.isNull()) {
        titleArt = placeholderTitleArt();
    }
    m_titleArtLabel->setPixmap(titleArt);
    m_titleArtLabel->setFixedSize(titleArt.size());

    if (!m_menuAssets.headerBackground().isNull() && m_titleBar) {
        const QString bgPath = QCoreApplication::applicationDirPath()
                               + QStringLiteral("/assets/images/menu/MENU_HEADER_BG.png");
        const QString bg = QStringLiteral(
                               "QWidget#menuTitleBar {"
                               "  background-color: %1;"
                               "  border-image: url(%2) 0 0 0 0 stretch stretch;"
                               "}")
                               .arg(QString::fromUtf8(kMenuBlue), bgPath);
        m_titleBar->setStyleSheet(bg + QString::fromUtf8(kTitleBarStyle));
    }

    QPixmap saveThumb = KCTMenuAssets::scaledThumbnail(m_menuAssets.saveAppleThumbnail(), thumbSize);
    if (saveThumb.isNull()) {
        saveThumb = placeholderThumbnail(thumbSize, QColor(200, 230, 200), tr("Save Apples"));
    }
    m_saveAppleThumb->setPixmap(saveThumb);

    QPixmap spaceThumb = KCTMenuAssets::scaledThumbnail(m_menuAssets.spaceWarThumbnail(), thumbSize);
    if (spaceThumb.isNull()) {
        spaceThumb = placeholderThumbnail(thumbSize, QColor(30, 40, 70), tr("Space War"));
    }
    m_spaceWarThumb->setPixmap(spaceThumb);
}

QPixmap KCTMainWindow::placeholderThumbnail(const QSize& size, const QColor& fillColor,
                                            const QString& hint) const
{
    QPixmap pixmap(size);
    pixmap.fill(fillColor);
    if (!hint.isEmpty()) {
        QPainter painter(&pixmap);
        painter.setPen(QColor(102, 102, 102));
        painter.drawText(pixmap.rect(), Qt::AlignCenter, hint);
    }
    return pixmap;
}

void KCTMainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange) {
        updateMaximizeButton();
    }
    QMainWindow::changeEvent(event);
}

void KCTMainWindow::updateMaximizeButton()
{
    if (!m_maximizeBtn) {
        return;
    }
    m_maximizeBtn->setText(isMaximized() ? QStringLiteral("\u25a3") : QStringLiteral("\u25a1"));
}

void KCTMainWindow::onMinimizeClicked()
{
    showMinimized();
}

void KCTMainWindow::onMaximizeClicked()
{
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
    updateMaximizeButton();
}

void KCTMainWindow::onCloseClicked()
{
    close();
}

void KCTMainWindow::onSaveAppleClicked()
{
    hide();

    auto* pGame = new KCTAppleGame();
    pGame->setAttribute(Qt::WA_DeleteOnClose);

    connect(pGame, &KCTAppleGame::destroyed, this, [this]() {
        show();
        updateMaximizeButton();
    });

    pGame->show();
    pGame->activateWindow();
    pGame->setFocus(Qt::ActiveWindowFocusReason);
}

void KCTMainWindow::onSpaceWarClicked()
{
    hide();

    auto* pGame = new KCTSpaceWarGame();
    pGame->setAttribute(Qt::WA_DeleteOnClose);

    connect(pGame, &KCTSpaceWarGame::destroyed, this, [this]() {
        show();
        updateMaximizeButton();
    });

    pGame->show();
    pGame->activateWindow();
    pGame->setFocus(Qt::ActiveWindowFocusReason);
}
