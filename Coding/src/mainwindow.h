/* -------------------------------------------------------------------------
//  文件名      :  mainwindow.h
//  创建者      :  classexam
//  创建时间    :  2026-05-11
//  功能描述    :  Declares KCTMainWindow (application main menu shell).
//  版权信息    :  Copyright (c) classexam. All rights reserved.
// -------------------------------------------------------------------------*/

#ifndef __CLASSEXAM_MAINWINDOW_H__
#define __CLASSEXAM_MAINWINDOW_H__

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class KCTMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit KCTMainWindow(QWidget *parent = nullptr);
    ~KCTMainWindow() override;

private slots:
    void onSaveAppleClicked();
    void onSpaceWarClicked();
    void onExitClicked();

private:
    KCTMainWindow(const KCTMainWindow&) = delete;
    KCTMainWindow& operator=(const KCTMainWindow&) = delete;

    Ui::MainWindow *ui;
};

#endif // __CLASSEXAM_MAINWINDOW_H__
