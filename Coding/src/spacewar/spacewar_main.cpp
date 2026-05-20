/* -------------------------------------------------------------------------
//  文件名      :  spacewar_main.cpp
//  创建者      :  陈正翔
//  创建时间    :  2026-05-11
//  功能描述    :  Stand-alone Space War executable entry (optional external CMake project).
// -------------------------------------------------------------------------*/

#include "spacewargame.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char* argv[])
{
    QApplication application(argc, argv);
    KCTSpaceWarGame mainWindow;
    mainWindow.setWindowTitle(QObject::tr("Space War"));
    mainWindow.show();
    return QCoreApplication::exec();
}
