/* -------------------------------------------------------------------------
//  文件名      :  gameviewport.h
//  功能描述    :  Design-resolution (800x600) viewport with uniform scale and letterbox.
// -------------------------------------------------------------------------*/

#ifndef __CLASSEXAM_GAMEVIEWPORT_H__
#define __CLASSEXAM_GAMEVIEWPORT_H__

#include <QSize>

class KCTGameViewport
{
public:
    static constexpr int designWidth() { return 800; }
    static constexpr int designHeight() { return 600; }

    void updateFromClientSize(int clientWidth, int clientHeight);

    double scale() const { return m_scale; }
    int gameWidth() const { return m_gameWidth; }
    int gameHeight() const { return m_gameHeight; }
    int offsetX() const { return m_offsetX; }
    int offsetY() const { return m_offsetY; }

    int scaled(int designPixels) const;
    double scaled(double designPixels) const;

    int mapDesignX(int designX) const;
    int mapDesignY(int designY) const;
    int mapDesignX(double designX) const;
    int mapDesignY(double designY) const;

    static QSize defaultWindowSize();

private:
    double m_scale = 1.0;
    int m_gameWidth = designWidth();
    int m_gameHeight = designHeight();
    int m_offsetX = 0;
    int m_offsetY = 0;
};

#endif // __CLASSEXAM_GAMEVIEWPORT_H__
