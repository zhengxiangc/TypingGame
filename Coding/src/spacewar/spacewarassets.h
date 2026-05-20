/* -------------------------------------------------------------------------
//  文件名      :  spacewarassets.h
//  功能描述    :  Loads Space War image assets from assets/images/spacewar/.
// -------------------------------------------------------------------------*/

#ifndef __CLASSEXAM_SPACEWARASSETS_H__
#define __CLASSEXAM_SPACEWARASSETS_H__

#include <QPixmap>
#include <QString>
#include <QVector>

class KCTSpaceWarAssets
{
public:
    bool load(const QString& assetsRoot);

    bool isLoaded() const { return m_loaded; }

    const QPixmap& background() const { return m_background; }
    const QPixmap& ship() const { return m_ship; }
    const QPixmap& bomb() const { return m_bomb; }
    const QPixmap& enemyPlane() const { return m_enemyPlane; }
    const QPixmap& enemyMeteor() const { return m_enemyMeteor; }
    const QPixmap& captionBack() const { return m_captionBack; }
    const QPixmap& explosionSheet() const { return m_explosionSheet; }
    const QPixmap& lifeBarEmpty() const { return m_lifeBarEmpty; }
    const QPixmap& lifeBarFill() const { return m_lifeBarFill; }
    const QPixmap& labelLife() const { return m_labelLife; }
    const QPixmap& labelScore() const { return m_labelScore; }
    const QPixmap& labelTime() const { return m_labelTime; }
    const QPixmap& settingsBackground() const { return m_settingsBackground; }
    const QPixmap& checkboxUnchecked() const { return m_checkboxUnchecked; }
    const QPixmap& checkboxChecked() const { return m_checkboxChecked; }

    const QVector<QPixmap>& starPixels() const { return m_starPixels; }

    int explosionFrameCols() const { return m_explosionCols; }
    int explosionFrameRows() const { return m_explosionRows; }
    int explosionFrameCount() const { return m_explosionFrameCount; }
    QPixmap explosionFrame(int frameIndex) const;

private:
    static QPixmap loadPixmap(const QString& dir, const QString& fileName);
    static QPixmap cropHorizontalHalf(const QPixmap& source, bool rightHalf);

    bool m_loaded = false;
    int m_explosionCols = 3;
    int m_explosionRows = 3;
    int m_explosionFrameCount = 8;

    QPixmap m_background;
    QPixmap m_ship;
    QPixmap m_bomb;
    QPixmap m_enemyPlane;
    QPixmap m_enemyMeteor;
    QPixmap m_captionBack;
    QPixmap m_explosionSheet;
    QPixmap m_lifeBarEmpty;
    QPixmap m_lifeBarFill;
    QPixmap m_labelLife;
    QPixmap m_labelScore;
    QPixmap m_labelTime;
    QPixmap m_settingsBackground;
    QPixmap m_checkboxUnchecked;
    QPixmap m_checkboxChecked;
    QVector<QPixmap> m_starPixels;
};

#endif // __CLASSEXAM_SPACEWARASSETS_H__
