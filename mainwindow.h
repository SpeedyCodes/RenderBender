//    Copyright (C) 2022 Jesse Daems
//    Read the full copyright notice in mainwindow.cpp

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <metasettings.h>
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
    void readJson(QString path);
    void GenerateUI();
private slots:
    void selectJson();
    void onSettingValueChanged(int i);
    void onSettingValueChanged(double i);

    void on_actionExit_triggered();

    void on_actionPreferences_triggered();
    void readPreferences();

    void on_actionReread_all_setting_values_triggered();

    void on_actionSelect_new_settings_JSON_file_triggered();

private:
    Ui::MainWindow *ui;
    metaSettings *metasettings;
};
#endif // MAINWINDOW_H
