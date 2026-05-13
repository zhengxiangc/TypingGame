/* -------------------------------------------------------------------------
//  文件名      :  mainwindow.cpp
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Main menu window: launches Save Apples and Space War sub-games.
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#include "mainwindow.h"

#include "applegame.h"
#include "spacewar/spacewargame.h"
#include "ui_mainwindow.h"

KCTMainWindow::KCTMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle(tr("Typing Game"));

    connect(ui->saveAppleBtn, &QPushButton::clicked,
            this, &KCTMainWindow::onSaveAppleClicked);
    connect(ui->spaceWarBtn, &QPushButton::clicked,
            this, &KCTMainWindow::onSpaceWarClicked);
    connect(ui->exitBtn, &QPushButton::clicked,
            this, &KCTMainWindow::onExitClicked);
}

KCTMainWindow::~KCTMainWindow()
{
    delete ui;
}

void KCTMainWindow::onSaveAppleClicked()
{
    this->hide();

    KCTAppleGame *pGame = new KCTAppleGame();
    pGame->setAttribute(Qt::WA_DeleteOnClose);

    connect(pGame, &KCTAppleGame::destroyed, this, &KCTMainWindow::show);

    pGame->show();

    pGame->activateWindow();
    pGame->setFocus(Qt::ActiveWindowFocusReason);
}

void KCTMainWindow::onSpaceWarClicked()
{
    this->hide();

    KCTSpaceWarGame* pGame = new KCTSpaceWarGame();
    pGame->setAttribute(Qt::WA_DeleteOnClose);

    connect(pGame, &KCTSpaceWarGame::destroyed, this, &KCTMainWindow::show);

    pGame->show();
    pGame->activateWindow();
    pGame->setFocus(Qt::ActiveWindowFocusReason);
}

void KCTMainWindow::onExitClicked()
{
    close();
}
