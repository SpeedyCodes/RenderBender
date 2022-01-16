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

#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <Windows.h>
#include <windows.h>
#include <TlHelp32.h>
#include <QtDebug>
#include <string>
#include <cstring>
#include <QJsonArray>
#include <QJsonObject>
using namespace std;

DWORD GetProcessId(const wchar_t* procName)
{
    DWORD procId = 0;
    HANDLE hSnap = (CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    if (hSnap != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 procEntry;
        procEntry.dwSize = sizeof(procEntry);
        if (Process32First(hSnap, &procEntry))
        {
            do
            {
                if (!_wcsicmp(procEntry.szExeFile, procName))
                {
                    procId = procEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnap, &procEntry));
        }
    }
    CloseHandle(hSnap);
    return procId;
}

uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t* modName)
{
    uintptr_t modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32First(hSnap, &modEntry))
        {
            do
            {
                if (!_wcsicmp(modEntry.szModule, modName))
                {
                    modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnap, &modEntry));
        }
    }
    CloseHandle(hSnap);
    return modBaseAddr;
}

uintptr_t FindDMAAddress(HANDLE hProc, uintptr_t ptr, std::vector<unsigned int> offsets)
{
    uintptr_t addr = ptr;
    for (unsigned int i = 0; i < offsets.size(); ++i)
    {
        ReadProcessMemory(hProc, (BYTE*)addr, &addr, sizeof(addr), 0);
        addr += offsets[i];
    }
    return addr;
}
uintptr_t getBaseWorkingAddress(uintptr_t staticOffset){
    DWORD procId = GetProcessId(L"Minecraft.Windows.exe");
    uintptr_t moduleBase = GetModuleBaseAddress(procId, L"Minecraft.Windows.exe");
    HANDLE hProcess = 0;
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, procId);
    uintptr_t dynamicPtrBaseAddr = moduleBase + staticOffset;

    std::vector<unsigned int> baseOffsets = { 0x40, 0xA0, 0xC0, 0xDC };
    return FindDMAAddress(hProcess, dynamicPtrBaseAddr, baseOffsets);
}

int changeSetting(float value, uintptr_t settingAddr)
{
    float settingValue = 0;
    DWORD procId = GetProcessId(L"Minecraft.Windows.exe");
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, procId);
    ReadProcessMemory(hProcess, (BYTE*)settingAddr, &settingValue, sizeof(settingValue), nullptr);
    if(settingValue == value){ return 0;} // value already correct, stop
    WriteProcessMemory(hProcess, (BYTE*)settingAddr, &value, sizeof(value), nullptr);
    ReadProcessMemory(hProcess, (BYTE*)settingAddr, &settingValue, sizeof(settingValue), nullptr);
    if (settingValue == value) return 1; //value changed succesfully
    else return -1; // failed to change value
}int changeSetting(int value, uintptr_t settingAddr)
{
    int settingValue = 0;
    DWORD procId = GetProcessId(L"Minecraft.Windows.exe");
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, procId);
    ReadProcessMemory(hProcess, (BYTE*)settingAddr, &settingValue, sizeof(settingValue), nullptr);
    if(settingValue == value){ return 0;} // value already correct, stop
    WriteProcessMemory(hProcess, (BYTE*)settingAddr, &value, sizeof(value), nullptr);
    ReadProcessMemory(hProcess, (BYTE*)settingAddr, &settingValue, sizeof(settingValue), nullptr);
    if (settingValue == value) return 1; //value changed succesfully
    else return -1; // failed to change value
}int changeSetting(bool value, uintptr_t settingAddr)
{
    bool settingValue = 0;
    DWORD procId = GetProcessId(L"Minecraft.Windows.exe");
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, procId);
    ReadProcessMemory(hProcess, (BYTE*)settingAddr, &settingValue, sizeof(settingValue), nullptr);
    if(settingValue == value){ return 0;} // value already correct, stop
    WriteProcessMemory(hProcess, (BYTE*)settingAddr, &value, sizeof(value), nullptr);
    ReadProcessMemory(hProcess, (BYTE*)settingAddr, &settingValue, sizeof(settingValue), nullptr);
    if (settingValue == value) return 1; //value changed succesfully
    else return -1; // failed to change value
}

uintptr_t computeSettingAddress(int settingIndex, uintptr_t base, QJsonObject &json){
    QJsonArray settings = json.value(QString("settings")).toArray();
    QJsonObject setting = settings[settingIndex].toObject();
    QByteArray ba = setting.value(QString("offset")).toString().toLocal8Bit();
    const char *Addr = ba.data();
    unsigned int x;
    std::stringstream ss;
    ss << std::hex << Addr;
    ss >> x;
    return base + static_cast<int>(x);
}
HANDLE readPrep(){
    DWORD procId = GetProcessId(L"Minecraft.Windows.exe");
    return OpenProcess(PROCESS_ALL_ACCESS, NULL, procId);
}
void readSetting(uintptr_t settingAddr, int &resultVar)
{
    ReadProcessMemory(readPrep(), (BYTE*)settingAddr, &resultVar, sizeof(resultVar), nullptr);
}
void readSetting(uintptr_t settingAddr, float &resultVar)
{
    ReadProcessMemory(readPrep(), (BYTE*)settingAddr, &resultVar, sizeof(resultVar), nullptr);
}
void readSetting(uintptr_t settingAddr, bool &resultVar)
{
    ReadProcessMemory(readPrep(), (BYTE*)settingAddr, &resultVar, sizeof(resultVar), nullptr);
}
