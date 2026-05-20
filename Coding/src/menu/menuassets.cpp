/* -------------------------------------------------------------------------
//  文件名      :  menuassets.cpp
//  功能描述    :  Loads main-menu image assets from assets/images/menu/.
// -------------------------------------------------------------------------*/

#include "menuassets.h"

#include <QFile>

QPixmap KCTMenuAssets::loadPixmap(const QString& dir, const QString& fileName)
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

QPixmap KCTMenuAssets::scaledThumbnail(const QPixmap& source, const QSize& targetSize)
{
    if (source.isNull()) {
        return {};
    }
    return source.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

bool KCTMenuAssets::load(const QString& assetsRoot)
{
    const QString imageDir = assetsRoot + QStringLiteral("/images/menu");

    m_appIcon = loadPixmap(imageDir, QStringLiteral("MENU_APP_ICON.png"));
    m_titleArt = loadPixmap(imageDir, QStringLiteral("MENU_TITLE_ART.png"));
    m_headerBackground = loadPixmap(imageDir, QStringLiteral("MENU_HEADER_BG.png"));
    m_saveAppleThumbnail = loadPixmap(imageDir, QStringLiteral("MENU_SAVE_APPLE_THUMB.png"));
    m_spaceWarThumbnail = loadPixmap(imageDir, QStringLiteral("MENU_SPACE_WAR_THUMB.png"));

    return true;
}
