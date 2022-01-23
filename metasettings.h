//    Copyright (C) 2022 Jesse Daems
//    Read the full copyright notice in mainwindow.cpp

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
    explicit metaSettings(QWidget *parent = nullptr);
    ~metaSettings();
    uintptr_t staticOffsetTransferVar;
    void updateStaticOffset(uintptr_t newValue);

private slots:
    void on_staticMemoryOffsetTxt_textChanged(const QString &arg1);

private:
    Ui::metaSettings *ui;
};

#endif // METASETTINGS_H
