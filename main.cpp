//    Copyright (C) 2022 Jesse Daems
//    Read the full copyright notice in mainwindow.cpp


#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <Windows.h>
int main(int argc, char *argv[])
{
    //enabling high dpi scaling if necessary before application is intialized
    //requires reading config.json in this convoluted way
    char selfdir[MAX_PATH] = {0};
    GetModuleFileNameA(NULL, selfdir, MAX_PATH);
    QString dir = QString::fromLocal8Bit(selfdir);
    QFile file(dir.left(dir.lastIndexOf(QChar('\\')))+"\\config.json");
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString rawText = file.readAll();
    file.close();
    QJsonDocument document = QJsonDocument::fromJson(rawText.toUtf8());
    if(document.object().value(QString("enableHighDpiScaling")).toBool() == true) QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    //initialize application
    QApplication a(argc, argv);
    QApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);
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
