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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPushButton>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCheckBox>
#include <QLabel>
#include <QSpinBox>
#include <QDebug>
#include <QFileDialog>
#include <QStandardPaths>
#include <Windows.h>
#include <sstream>
#include <metasettings.h>
#include <utils.h>
#include <QMessageBox>

enum class settingType { INT, FLOAT, BOOL};
HANDLE readPrep();
class setting {
public:
    char *displayName;
    //char *shortName;
    uintptr_t* base;
    uintptr_t addr;
    int offset;
    char *group;
    int index;
    //char *description;
    float defaultValue;
    float minVal;
    float maxVal;
    QWidget* widget;
    settingType type;
    //int precision;
    void updateAddr(){
        addr = *base + offset;
    }
    void read(int &resultVar){
        ReadProcessMemory(readPrep(), (BYTE*)addr, &resultVar, sizeof(resultVar), nullptr);
    }
    void read(float &resultVar){
        ReadProcessMemory(readPrep(), (BYTE*)addr, &resultVar, sizeof(resultVar), nullptr);
    }
    void read(bool &resultVar){
        ReadProcessMemory(readPrep(), (BYTE*)addr, &resultVar, sizeof(resultVar), nullptr);
    }
    //https://stackoverflow.com/questions/351845/finding-the-type-of-an-object-in-c
    setting(uintptr_t &baseAddr, QString offsetString, char *name, char *groupName, float defaultVal, float min, float max, float i, settingType varType, QWidget* w){
        displayName = name;
        offset = utils::hexToDec(offsetString);
        base = &baseAddr;
        updateAddr();
        group = groupName;
        index = i;
        type = varType;
        defaultValue = defaultVal;
        minVal = min;
        maxVal = max;
        widget = w;
    }

};

int changeSetting(float value, uintptr_t settingAddr);
int changeSetting(int value, uintptr_t settingAddr);
int changeSetting(bool value, uintptr_t settingAddr);
uintptr_t getBaseWorkingAddress(uintptr_t staticOffset);
uintptr_t computeSettingAddress(int settingIndex, uintptr_t base, QJsonObject &json);
void readSetting(uintptr_t settingAddr, float &resultVar);
void readSetting(uintptr_t settingAddr, int &resultVar);
void readSetting(uintptr_t settingAddr, bool &resultVar);
using namespace std;
QJsonObject json;
uintptr_t sunAzimuthAddress = 0;
vector<setting> settings;
uintptr_t staticOffset;
bool UIGenerated = false;
QString settingsJsonPath = "";
bool changingUIvalues = false;

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->selectJsonBtn, SIGNAL(clicked()), this, SLOT(selectJson()));
    readPreferences();
    if(settingsJsonPath != "") {
        readJson(settingsJsonPath);
        GenerateUI();
        UIGenerated = true;
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::onValueChanged() {
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    delete button;
}
void MainWindow::selectJson(){
    QString path = QFileDialog::getOpenFileName(this,tr("Open JSON"),  QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).constFirst(), tr("JSON Files (*.json)"));
    if(path == ""){return;}
    utils::writeConfigProperty("settingsJSONpath", path);
    if(UIGenerated){
        QMessageBox msgBox;
        msgBox.setText("The new setting file's location has been saved. Please restart RenderBender now for the changes to take effect.");
        msgBox.exec();
    }else{
        readJson(path);
        GenerateUI();
        UIGenerated = true;
    }
}
void MainWindow::readJson(QString path){
    QFile file;
    file.setFileName(path);
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString rawText = file.readAll();
    file.close();
    QJsonDocument document = QJsonDocument::fromJson(rawText.toUtf8());
    json = document.object();
}

void MainWindow::onSettingValueChanged(double i){
    if(changingUIvalues)return;
    int index = sender()->objectName().replace("settingUIelement", "").toInt();
    changeSetting(static_cast<float>(i), settings[index].addr);

}
void MainWindow::onSettingValueChanged(int i){
    if(changingUIvalues)return;
    int index = sender()->objectName().replace("settingUIelement", "").toInt();
    if(settings[index].type == settingType::BOOL){
        changeSetting(i==1, settings[index].addr);
    }
    changeSetting(i, settings[index].addr);
}
void MainWindow::GenerateUI(){
    delete ui->selectJsonBtn;
    delete ui->selectJsonLabel;
    QJsonArray settingsJson = json.value(QString("settings")).toArray();
    int layoutCounter = 0;
    QList<QVBoxLayout *> layouts = centralWidget()->findChildren<QVBoxLayout *>();
    QStringList groupNames;
    changingUIvalues = true;
    for (int i = 0; i < settingsJson.count(); i++) {
        //https://doc.qt.io/qt-5/qtabwidget.html#addTab
        QJsonObject obj = settingsJson[i].toObject();
        QString groupName = obj.value(QString("group")).toString();
        if(!groupNames.contains(groupName)){
            groupNames << groupName;
            //layoutCounter = groupNames.indexOf(groupName);
        }
        if(i != 0 && i%15 == 0){
            layoutCounter++;
        }
        QString type = obj.value(QString("type")).toString();
        QString infoText = obj.value(QString("displayName")).toString();
        QWidget *widget;
        if (type == "int"){
            QLabel* label = new QLabel(infoText);
            layouts[layoutCounter]->insertWidget(0, label);
            QSpinBox *spinBox = new QSpinBox();
            int val;
            setting s(sunAzimuthAddress, obj.value(QString("offset")).toString(), infoText.toLocal8Bit().data(),
            groupName.toLocal8Bit().data(), obj.value(QString("default")).toInt(), obj.value(QString("min")).toInt(),
            obj.value(QString("max")).toInt(), i,  settingType::INT, spinBox);
            settings.push_back(s);
            s.read(val);
            layouts[layoutCounter]->insertWidget(1, spinBox);
            spinBox->setValue(val);
            QObject::connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(onSettingValueChanged(int)));
            //QObject::connect(spinBox, &QSpinBox::valueChanged, this, &MainWindow::onSettingValueChanged);
            widget = spinBox;
        }
        else if (type == "float"){
            QLabel* label = new QLabel(infoText);
            layouts[layoutCounter]->insertWidget(0, label);
            QDoubleSpinBox *spinBox = new QDoubleSpinBox();
            spinBox->setDecimals(7);
            setting s(sunAzimuthAddress, obj.value(QString("offset")).toString(), infoText.toLocal8Bit().data(),
            groupName.toLocal8Bit().data(), obj.value(QString("default")).toDouble(), obj.value(QString("min")).toDouble(),
            obj.value(QString("max")).toDouble(), i, settingType::FLOAT, spinBox);
            settings.push_back(s);
            float val;
            s.read(val);
            layouts[layoutCounter]->insertWidget(1, spinBox);
            spinBox->setValue(val);
            QObject::connect(spinBox, SIGNAL(valueChanged(double)), this, SLOT(onSettingValueChanged(double)));
            widget = spinBox;
        }
        else if (type == "bool"){
            QCheckBox *cb = new QCheckBox(infoText);
            setting s(sunAzimuthAddress, obj.value(QString("offset")).toString(), infoText.toLocal8Bit().data(),
            groupName.toLocal8Bit().data(), obj.value(QString("default")).toBool(), false,
            true, i, settingType::BOOL, cb);
            settings.push_back(s);
            bool val;
            s.read(val);
            layouts[layoutCounter]->insertWidget(0, cb);
            cb->setChecked(val);
            QObject::connect(cb, SIGNAL(stateChanged(int)), this, SLOT(onSettingValueChanged(int)));
            widget = cb;
        }else{
            continue;
        }
        widget->setStyleSheet("margin: 5 0 5 0;");
        widget->setObjectName("settingUIelement" + QString::number(i));
    }
    changingUIvalues = false;
}



void MainWindow::on_actionExit_triggered()
{
    QCoreApplication::quit();
}


void MainWindow::on_actionPreferences_triggered()
{
    metasettings = new metaSettings(this);
    metasettings->setModal(true);
    if (staticOffset != 0) metasettings->updateStaticOffset(staticOffset);
    if(metasettings->exec() == QDialog::Accepted){
        utils::writeConfigProperty("staticOffset", utils::decToHex(metasettings->staticOffsetTransferVar));
        readPreferences();
        for (int i = 0; i < settings.size(); i++) {
            settings[i].updateAddr();
        }
        on_actionReread_all_setting_values_triggered();
    }
}
void MainWindow::readPreferences(){
    if(!QFile::exists(QCoreApplication::applicationDirPath() +"/config.json")){return;}
    QFile file(QCoreApplication::applicationDirPath() +"/config.json");
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString rawText = file.readAll();
    file.close();
    QJsonDocument document = QJsonDocument::fromJson(rawText.toUtf8());
    QJsonObject obj = document.object();
    staticOffset = utils::hexToDec(obj.value(QString("staticOffset")).toString());
    settingsJsonPath = obj.value(QString("settingsJSONpath")).toString();
    sunAzimuthAddress = getBaseWorkingAddress(staticOffset);
}


void MainWindow::on_actionReread_all_setting_values_triggered()
{
    changingUIvalues = true;
    for (int i = 0; i < settings.size(); i++) {
        switch (settings[i].type) {
            case settingType::FLOAT:{
                float newVal = 0;
                settings[i].read(newVal);
                QDoubleSpinBox *widget = (QDoubleSpinBox*)settings[i].widget;
                widget->setValue(newVal);
                break;
            }
            case settingType::INT:{
                int newVal = 0;
                settings[i].read(newVal);
                QSpinBox *widget = (QSpinBox*)settings[i].widget;
                widget->setValue(newVal);
                break;
            }
            case settingType::BOOL:{
                bool newVal = false;
                settings[i].read(newVal);
                QCheckBox *widget = (QCheckBox*)settings[i].widget;
                widget->setChecked(newVal);
                break;
            }
        }

    }
    changingUIvalues = false;
}


void MainWindow::on_actionSelect_new_settings_JSON_file_triggered()
{
    selectJson();
}

