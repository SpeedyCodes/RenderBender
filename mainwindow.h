//    Copyright (C) 2022 Jesse Daems
//    Read the full copyright notice in mainwindow.cpp

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <metasettings.h>
#include <savepresetdialog.h>
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
    void readJson(QString path);
    void GenerateUI();
    bool allowedToBoot;
private slots:
    void selectJson();
    void onSettingValueChanged(int i);
    void onSettingValueChanged(double i);
    void on_actionExit_triggered();
    void on_actionPreferences_triggered();
    bool readPreferences();
    void on_actionReread_all_setting_values_triggered();
    void on_actionSetPresetValues_triggered();
    void onResetToPresetButtonClicked();
    void onSliderValueChanged(int index);
    void on_actionSave_preset_triggered();

    void on_actionUsage_triggered();

    void on_actionCEPresetImport_triggered();

private:
    Ui::MainWindow *ui;
    metaSettings *metasettings;
    savePresetDialog *savepresetdialog;
};
#endif // MAINWINDOW_H
