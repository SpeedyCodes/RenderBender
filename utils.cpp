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
#include <windows.h>

QString utils::version = "v0.3.1 Feature Testing #2";

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
    return "0x" + QString::number( dec, 16 );
}
void utils::runMinecraft(){
    LPCWSTR url = L"explorer.exe";
    ShellExecute(NULL, L"open", url, L"shell:appsFolder\\Microsoft.MinecraftUWP_8wekyb3d8bbwe!App", NULL, SW_SHOWNORMAL);
}

