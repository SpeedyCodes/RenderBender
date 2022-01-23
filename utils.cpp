//    RenderBender:A third party program to change Minecraft RTX's settings externally, directly in-memory.
//    Copyright (C) 2022 Jesse Daems

//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.

//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "utils.h"
#include <sstream>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
uintptr_t utils::hexToDec(QString hex){
    QByteArray ba = hex.toLocal8Bit();
    const char *Addr = ba.data();
    uintptr_t dec;
    std::stringstream ss;
    ss << std::hex << Addr;
    ss >> dec;
    return dec;
}
QString utils::decToHex(uintptr_t dec){
    return QString::number( dec, 16 );
}
void utils::writeConfigProperty(QString key, QString value){
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
