//    Copyright (C) 2022 Jesse Daems
//    Read the full copyright notice in mainwindow.cpp


#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QTextStream>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFile f(":qdarkstyle/dark/style.qss");

    if (!f.exists())   {
        printf("Unable to set QDarkStyleSheet, file not found\n");
    }
    else   {
        f.open(QFile::ReadOnly | QFile::Text);
        QTextStream ts(&f);
        qApp->setStyleSheet(ts.readAll());
    }
    MainWindow w;
    if(!w.allowedToBoot)return 0;
    w.show();
    return a.exec();
}
