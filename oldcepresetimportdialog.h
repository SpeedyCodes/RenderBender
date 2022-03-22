//    Copyright (C) 2022 Jesse Daems
//    Read the full copyright notice in mainwindow.cpp
#pragma once
#ifndef OLDCEPRESETIMPORTDIALOG_H
#define OLDCEPRESETIMPORTDIALOG_H

#include <QDialog>

namespace Ui
{
    class oldCEPresetImportdialog;
}

class oldCEPresetImportdialog : public QDialog
{
    Q_OBJECT

public:
    explicit oldCEPresetImportdialog(QWidget *parent = nullptr, QStringList *names = new QStringList());
    std::vector<bool> activeSettings;
    std::vector<float> settingValues;
    ~oldCEPresetImportdialog();
    void getResults();

private slots:
    void on_stringInput_textChanged();

private:
    Ui::oldCEPresetImportdialog *ui;
};

#endif // OLDCEPRESETIMPORTDIALOG_H
