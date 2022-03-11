//    Copyright (C) 2022 Jesse Daems
//    Read the full copyright notice in mainwindow.cpp
#ifndef SAVEPRESETDIALOG_H
#define SAVEPRESETDIALOG_H

#include <QDialog>

namespace Ui {
class savePresetDialog;
}

class savePresetDialog : public QDialog
{
    Q_OBJECT

public:
    explicit savePresetDialog(QWidget *parent = nullptr, QStringList *names = new QStringList(), std::vector<bool> enabledPresets = {});
    std::vector<bool> readToggles();
    ~savePresetDialog();

private slots:
    void on_enableAllBtn_clicked();

    void on_disableAllBtn_clicked();

private:
    Ui::savePresetDialog *ui;
};

#endif // SAVEPRESETDIALOG_H
