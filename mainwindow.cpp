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

enum class settingType { INT, FLOAT, BOOL};
HANDLE readPrep();
class setting {
public:
    char *displayName;
    //char *shortName;
    uintptr_t addr;
    char *group;
    int index;
    //char *description;
    float defaultValue;
    float minVal;
    float maxVal;
    settingType type;
    //int precision;
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
    setting(uintptr_t baseAddr, QString offset, char *name, char *groupName, float defaultVal, float min, float max, float i, settingType varType){
        displayName = name;
        QByteArray b = offset.toLocal8Bit();
        unsigned int x;
        std::stringstream ss;
        ss << std::hex << b.constData();
        ss >> x;
        addr = baseAddr + static_cast<int>(x);
        group = groupName;
        index = i;
        type = varType;
        defaultValue = defaultVal;
        minVal = min;
        maxVal = max;
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
#include <vector>
QJsonObject json;
uintptr_t sunAzimuthAddress;
vector<setting> settings;

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->selectJsonBtn, SIGNAL(clicked()), this, SLOT(selectJson()));
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
    QFile file;
    file.setFileName(QFileDialog::getOpenFileName(this,tr("Open JSON"),  QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).constFirst(), tr("JSON Files (*.json)")));
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString rawText = file.readAll();
    file.close();
    QJsonDocument document = QJsonDocument::fromJson(rawText.toUtf8());
    json = document.object();
    sunAzimuthAddress = getBaseWorkingAddress(0x04191C40);
    GenerateUI();
}
void MainWindow::onSettingValueChanged(double i){
    int index = sender()->objectName().replace("settingUIelement", "").toInt();
    changeSetting(static_cast<float>(i), settings[index].addr);

}
void MainWindow::onSettingValueChanged(int i){
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
            obj.value(QString("max")).toInt(), i,  settingType::FLOAT);
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
            setting s(sunAzimuthAddress, obj.value(QString("offset")).toString(), infoText.toLocal8Bit().data(),
            groupName.toLocal8Bit().data(), obj.value(QString("default")).toDouble(), obj.value(QString("min")).toDouble(),
            obj.value(QString("max")).toDouble(), i, settingType::FLOAT);
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
            true, i, settingType::BOOL);
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
}


