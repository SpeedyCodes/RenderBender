//    Copyright (C) 2022 Jesse Daems
//    Read the full copyright notice in mainwindow.cpp
#pragma once
#ifndef UTILS_H
#define UTILS_H
#include <QApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>


class utils
{
public:
    static uintptr_t hexToDec(QString hex);
    static QString decToHex(uintptr_t dec);
    template<typename T>
    static void writeConfigProperty(QString key, const T& value){
        QFile file(QCoreApplication::applicationDirPath() +"/config.json");
        file.open(QIODevice::ReadWrite | QIODevice::Text);
        QString rawText = file.readAll();
        QJsonDocument document = QJsonDocument::fromJson(rawText.toUtf8());
        QJsonObject jsonObject = document.object();
        jsonObject.insert(key, value);
        QJsonDocument jsonDoc;
        jsonDoc.setObject(jsonObject);
        file.resize(0);
        file.write(jsonDoc.toJson());
        file.close();
    }
    static void runMinecraft();
    static QString version;
};

#endif // UTILS_H
