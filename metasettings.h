//    Copyright (C) 2022 Jesse Daems
//    Read the full copyright notice in mainwindow.cpp
#pragma once
#ifndef METASETTINGS_H
#define METASETTINGS_H

#include <QDialog>

namespace Ui {
class metaSettings;
}

class metaSettings : public QDialog
{
    Q_OBJECT

public:
    explicit metaSettings(QWidget *parent = nullptr, uintptr_t statOff = 0, bool presValBeh = false, QString settingsJsonPath = "", bool autoMcStartupBehaviour = false, bool behaviourOnMcShutdown = false, int defaultPreset = 0, QStringList presetNames = {});
    ~metaSettings();
    uintptr_t staticOffsetTransferVar;
    bool presetValueBehaviourTransferVar;
    QString settingsJsonPathTransferVar;
    bool autoMcStartupBehaviourTransferVar;
    bool behaviourOnMcShutdownTransferVar;
    int defaultPresetTransferVar;

private slots:
    void on_staticMemoryOffsetTxt_textChanged(const QString &arg1);
    void on_buttonBox_accepted();

    void on_presValBehBox_currentIndexChanged(int index);

    void on_settingsJSONlocBtn_pressed();

    void on_startupBehaviourCB_stateChanged(int arg1);

    void on_shutdownBehaviourCB_stateChanged(int arg1);

    void on_loadPresetcheckBox_stateChanged(int arg1);

private:
    Ui::metaSettings *ui;
};

#endif // METASETTINGS_H
