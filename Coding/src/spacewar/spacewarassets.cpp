/* -------------------------------------------------------------------------
//  文件名      :  spacewarassets.cpp
//  功能描述    :  Loads Space War image assets from assets/images/spacewar/.
// -------------------------------------------------------------------------*/

#include "spacewarassets.h"

#include <QDir>
#include <QFile>

QPixmap KCTSpaceWarAssets::loadPixmap(const QString& dir, const QString& fileName)
{
    const QString path = dir + QLatin1Char('/') + fileName;
    if (!QFile::exists(path)) {
        return {};
    }
    QPixmap pixmap(path);
    if (pixmap.isNull()) {
        return {};
    }
    return pixmap;
}

QPixmap KCTSpaceWarAssets::cropHorizontalHalf(const QPixmap& source, bool rightHalf)
{
    if (source.isNull()) {
        return {};
    }
    const int halfW = source.width() / 2;
    const QRect rect(rightHalf ? halfW : 0, 0, halfW, source.height());
    return source.copy(rect);
}

bool KCTSpaceWarAssets::load(const QString& assetsRoot)
{
    const QString imageDir = assetsRoot + QStringLiteral("/images/spacewar");

    m_background = loadPixmap(imageDir, QStringLiteral("SPACE_BACKGROUND.png"));
    m_ship = loadPixmap(imageDir, QStringLiteral("SPACE_SHIP.png"));
    m_bomb = loadPixmap(imageDir, QStringLiteral("SPACE_BOMB.png"));
    m_enemyPlane = loadPixmap(imageDir, QStringLiteral("SPACE_ENEMY_0.png"));
    m_enemyMeteor = loadPixmap(imageDir, QStringLiteral("SPACE_ENEMY_4.png"));
    m_captionBack = loadPixmap(imageDir, QStringLiteral("SPACE_CAPTION_BACK.png"));
    m_explosionSheet = loadPixmap(imageDir, QStringLiteral("SPACE_EXPLOSION_0.png"));
    m_lifeBarEmpty = loadPixmap(imageDir, QStringLiteral("SPACE_LIFE.png"));
    m_lifeBarFill = loadPixmap(imageDir, QStringLiteral("SPACE_LIFE_OVER.png"));
    m_labelLife = loadPixmap(imageDir, QStringLiteral("SPACE_LABEL_LIFE.png"));
    m_labelScore = loadPixmap(imageDir, QStringLiteral("SPACE_LABEL_SCORE.png"));
    m_labelTime = loadPixmap(imageDir, QStringLiteral("SPACE_LABEL_TIME.png"));
    m_settingsBackground = loadPixmap(imageDir, QStringLiteral("APPLE_SETUP.png"));

    const QPixmap checkboxSheet = loadPixmap(imageDir, QStringLiteral("CHECKBOX_BUTTON.png"));
    m_checkboxUnchecked = cropHorizontalHalf(checkboxSheet, false);
    m_checkboxChecked = cropHorizontalHalf(checkboxSheet, true);

    const QPixmap starsStrip = loadPixmap(imageDir, QStringLiteral("SPACE_STARS.png"));
    m_starPixels.clear();
    if (!starsStrip.isNull() && starsStrip.width() >= 1) {
        const int nStars = qMin(5, starsStrip.width());
        m_starPixels.reserve(nStars);
        for (int i = 0; i < nStars; ++i) {
            m_starPixels.append(starsStrip.copy(i, 0, 1, 1));
        }
    }

    if (!m_explosionSheet.isNull()) {
        m_explosionCols = 3;
        m_explosionRows = 3;
        m_explosionFrameCount = m_explosionCols * m_explosionRows - 1;
    }

    m_loaded = !m_background.isNull() && !m_ship.isNull();
    return m_loaded;
}

QPixmap KCTSpaceWarAssets::explosionFrame(int frameIndex) const
{
    if (m_explosionSheet.isNull() || m_explosionCols <= 0 || m_explosionRows <= 0) {
        return {};
    }
    const int idx = qBound(0, frameIndex, explosionFrameCount() - 1);
    const int frameW = m_explosionSheet.width() / m_explosionCols;
    const int frameH = m_explosionSheet.height() / m_explosionRows;
    const int col = idx % m_explosionCols;
    const int row = idx / m_explosionCols;
    return m_explosionSheet.copy(col * frameW, row * frameH, frameW, frameH);
}
