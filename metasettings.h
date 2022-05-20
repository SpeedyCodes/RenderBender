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
    explicit metaSettings(QWidget *parent = nullptr, uintptr_t statOff = 0, int presValBeh = -1, QString settingsJsonPath = "", bool autoMcStartupBehaviour = false, bool behaviourOnMcShutdown = false);
    ~metaSettings();
    uintptr_t staticOffsetTransferVar;
    int presetValueBehaviourTransferVar;
    QString settingsJsonPathTransferVar;
    bool autoMcStartupBehaviourTransferVar;
    bool behaviourOnMcShutdownTransferVar;

private slots:
    void on_staticMemoryOffsetTxt_textChanged(const QString &arg1);
    void on_buttonBox_accepted();

    void on_presValBehBox_currentIndexChanged(int index);

    void on_settingsJSONlocBtn_pressed();

    void on_startupBehaviourCB_stateChanged(int arg1);

    void on_shutdownBehaviourCB_stateChanged(int arg1);

private:
    Ui::metaSettings *ui;
};

#endif // METASETTINGS_H
