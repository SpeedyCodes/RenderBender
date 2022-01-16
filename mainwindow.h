//    Copyright (C) 2022 Jesse Daems
//    Read the full copyright notice in mainwindow.cpp

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void onAddWidget();
    void onValueChanged();
    void ReadJson();
    void GenerateUI();
private slots:
    void selectJson();
    void onSettingValueChanged(int i);
    void onSettingValueChanged(double i);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
