/* -------------------------------------------------------------------------
//  文件名      :  main.cpp
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Qt application entry; constructs QApplication and main menu window.
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#include "mainwindow.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    KCTMainWindow mainWindow;
    mainWindow.show();
    return QCoreApplication::exec();
}
