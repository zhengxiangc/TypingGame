/* -------------------------------------------------------------------------
//  文件名      :  menuassets.h
//  功能描述    :  Loads main-menu image assets from assets/images/menu/.
// -------------------------------------------------------------------------*/

#ifndef __CLASSEXAM_MENUASSETS_H__
#define __CLASSEXAM_MENUASSETS_H__

#include <QPixmap>
#include <QString>

class KCTMenuAssets
{
public:
    bool load(const QString& assetsRoot);

    const QPixmap& appIcon() const { return m_appIcon; }
    const QPixmap& titleArt() const { return m_titleArt; }
    const QPixmap& headerBackground() const { return m_headerBackground; }
    const QPixmap& saveAppleThumbnail() const { return m_saveAppleThumbnail; }
    const QPixmap& spaceWarThumbnail() const { return m_spaceWarThumbnail; }

    static QPixmap scaledThumbnail(const QPixmap& source, const QSize& targetSize);

private:
    static QPixmap loadPixmap(const QString& dir, const QString& fileName);

    QPixmap m_appIcon;
    QPixmap m_titleArt;
    QPixmap m_headerBackground;
    QPixmap m_saveAppleThumbnail;
    QPixmap m_spaceWarThumbnail;
};

#endif // __CLASSEXAM_MENUASSETS_H__
