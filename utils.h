//    Copyright (C) 2022 Jesse Daems
//    Read the full copyright notice in mainwindow.cpp
#ifndef UTILS_H
#define UTILS_H
#include <QApplication>


class utils
{
public:
    static uintptr_t hexToDec(QString hex);
    static QString decToHex(uintptr_t dec);
    static void writeConfigProperty(QString key, QString value);
};

#endif // UTILS_H
