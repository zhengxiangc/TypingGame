/* -------------------------------------------------------------------------
//  文件名      :  gameviewport.cpp
//  功能描述    :  Letterboxed uniform scale from 800x600 design space to client area.
// -------------------------------------------------------------------------*/

#include "gameviewport.h"

#include <QtGlobal>

#include <QGuiApplication>
#include <QScreen>

void KCTGameViewport::updateFromClientSize(int clientWidth, int clientHeight)
{
    if (clientWidth < 1) {
        clientWidth = 1;
    }
    if (clientHeight < 1) {
        clientHeight = 1;
    }

    const double scaleX = static_cast<double>(clientWidth) / static_cast<double>(designWidth());
    const double scaleY = static_cast<double>(clientHeight) / static_cast<double>(designHeight());
    m_scale = qMin(scaleX, scaleY);

    m_gameWidth = qMax(1, qRound(static_cast<double>(designWidth()) * m_scale));
    m_gameHeight = qMax(1, qRound(static_cast<double>(designHeight()) * m_scale));
    m_offsetX = (clientWidth - m_gameWidth) / 2;
    m_offsetY = (clientHeight - m_gameHeight) / 2;
}

int KCTGameViewport::scaled(int designPixels) const
{
    return qMax(1, qRound(static_cast<double>(designPixels) * m_scale));
}

double KCTGameViewport::scaled(double designPixels) const
{
    return designPixels * m_scale;
}

int KCTGameViewport::mapDesignX(int designX) const
{
    return m_offsetX + scaled(designX);
}

int KCTGameViewport::mapDesignY(int designY) const
{
    return m_offsetY + scaled(designY);
}

int KCTGameViewport::mapDesignX(double designX) const
{
    return m_offsetX + qRound(scaled(designX));
}

int KCTGameViewport::mapDesignY(double designY) const
{
    return m_offsetY + qRound(scaled(designY));
}

QSize KCTGameViewport::defaultWindowSize()
{
    QScreen* pScreen = QGuiApplication::primaryScreen();
    if (!pScreen) {
        return QSize(designWidth(), designHeight());
    }

    const QRect available = pScreen->availableGeometry();
    int clientW = qMax(designWidth(), qRound(static_cast<double>(available.width()) * 0.85));
    int clientH = qMax(designHeight(), qRound(static_cast<double>(available.height()) * 0.85));

    const qreal dpr = pScreen->devicePixelRatio();
    if (dpr > 1.0) {
        const int dpiMinW = qRound(static_cast<double>(designWidth()) * dpr);
        const int dpiMinH = qRound(static_cast<double>(designHeight()) * dpr);
        clientW = qMax(clientW, dpiMinW);
        clientH = qMax(clientH, dpiMinH);
    }

    return QSize(clientW, clientH);
}
