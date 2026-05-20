/* -------------------------------------------------------------------------
//  文件名      :  mainwindow.h
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Declares KCTMainWindow (Kingsoft-style main menu shell).
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#ifndef __CLASSEXAM_MAINWINDOW_H__
#define __CLASSEXAM_MAINWINDOW_H__

#include <QMainWindow>

#include "menu/menuassets.h"

class QLabel;
class QPushButton;
class QWidget;

class KCTMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit KCTMainWindow(QWidget *parent = nullptr);
    ~KCTMainWindow() override;

private slots:
    void onSaveAppleClicked();
    void onSpaceWarClicked();
    void onMinimizeClicked();
    void onMaximizeClicked();
    void onCloseClicked();

private:
    void buildUi();
    void applyMenuAssets();
    QPixmap placeholderThumbnail(const QSize& size, const QColor& fillColor, const QString& hint) const;
    void updateMaximizeButton();

    KCTMainWindow(const KCTMainWindow&) = delete;
    KCTMainWindow& operator=(const KCTMainWindow&) = delete;

    KCTMenuAssets m_menuAssets;
    QWidget* m_titleBar = nullptr;
    QLabel* m_logoLabel = nullptr;
    QLabel* m_titleArtLabel = nullptr;
    QLabel* m_saveAppleThumb = nullptr;
    QLabel* m_spaceWarThumb = nullptr;
    QPushButton* m_maximizeBtn = nullptr;
    QWidget* m_saveAppleCard = nullptr;
    QWidget* m_spaceWarCard = nullptr;

protected:
    void changeEvent(QEvent* event) override;
};

#endif // __CLASSEXAM_MAINWINDOW_H__
