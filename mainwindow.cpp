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
#include <QSlider>
#include <QProgressDialog>
#include <QDesktopServices>

enum class settingType { INT, FLOAT, BOOL};
HANDLE readPrep();
DWORD GetProcessId(const wchar_t* procName);
const int defaultSpinboxPrecision = 7;
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
    QPushButton* resetButton;
    QSlider* slider;
    settingType type;
    //int precision;
    void updateAddr(){
        addr = *base + offset;
    }
    template<typename T>
    void read(T& resultVar){
        ReadProcessMemory(readPrep(), (BYTE*)addr, &resultVar, sizeof(resultVar), nullptr);
        resetButton->setEnabled((float)resultVar != (float)defaultValue);
    }
    template <typename T>
    bool write(T& value)
    {
        T settingValue = 0;
        DWORD procId = GetProcessId(L"Minecraft.Windows.exe");
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, procId);
        ReadProcessMemory(hProcess, (BYTE*)addr, &settingValue, sizeof(settingValue), nullptr);
        if(settingValue == value){resetButton->setEnabled((float)settingValue != defaultValue); return true;} // value already correct, stop
        WriteProcessMemory(hProcess, (BYTE*)addr, &value, sizeof(value), nullptr);
        ReadProcessMemory(hProcess, (BYTE*)addr, &settingValue, sizeof(settingValue), nullptr);
        resetButton->setEnabled((float)settingValue != (float)defaultValue);
        return (settingValue == value); //value changed succesfully/failed to change value
    }
    int resetToPreset(){
        switch (type) {
            case settingType::FLOAT:{
                if(write(defaultValue) == true){
                    QDoubleSpinBox *w = (QDoubleSpinBox*)widget;
                    w->setValue(defaultValue);
                    slider->setValue(defaultValue*pow(10, defaultSpinboxPrecision));
                    return true;
                }
                break;
            }
            case settingType::INT:{
                int convertedValue = (int)defaultValue;
                if(write(convertedValue) == true){
                    QSpinBox *w = (QSpinBox*)widget;
                    w->setValue(convertedValue);
                    slider->setValue(convertedValue);
                    return true;
                }
                break;
            }
            case settingType::BOOL:{
                bool convertedValue = (bool)defaultValue;
                if(write(convertedValue) == true){
                    QCheckBox *w = (QCheckBox*)widget;
                    w->setChecked(convertedValue);
                    return true;
                }
                break;
            }
        }
        return false;
    }
    void presetIntake(){
        switch (type) {
            case settingType::FLOAT:{
                QDoubleSpinBox *w = (QDoubleSpinBox*)widget;
                defaultValue = (float)w->value();
                break;
            }
            case settingType::INT:{
                QSpinBox *w = (QSpinBox*)widget;
                defaultValue = w->value();
                break;
            }
            case settingType::BOOL:{
                QCheckBox *w = (QCheckBox*)widget;
                defaultValue = w->isChecked();
                break;
            }
        }
    }
    //https://stackoverflow.com/questions/351845/finding-the-type-of-an-object-in-c
    setting(uintptr_t &baseAddr, QString offsetString, char *name, char *groupName, float defaultVal, float min, float max, float i, settingType varType, QWidget* w, QPushButton* rb
            , QSlider* sl){
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
        resetButton = rb;
        slider = sl;
    }

};
uintptr_t getBaseWorkingAddress(uintptr_t staticOffset);
uintptr_t computeSettingAddress(int settingIndex, uintptr_t base, QJsonObject &json);
using namespace std;
QJsonObject json;
uintptr_t sunAzimuthAddress = 0;
vector<setting> settings;
uintptr_t staticOffset;
int presetValueBehaviour;
bool UIGenerated = false;
QString settingsJsonPath = "";
bool changingUIvalues = false;
bool allowedToBoot = true;
int settingCount;

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow)
{
    while(GetProcessId(L"Minecraft.Windows.exe") == 0){
        QMessageBox msgBox;
        msgBox.setText("The Minecraft process was not detected. Please start up Minecraft first, and then press OK to try again. Alternatively, press cancel to quit.");
        msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
        if(msgBox.exec() == QMessageBox::Cancel){
            allowedToBoot = false;
            return;
        }
    }
    ui->setupUi(this);
    connect(ui->selectJsonBtn, SIGNAL(clicked()), this, SLOT(selectJson()));
    QFile config(QCoreApplication::applicationDirPath() +"/config.json");
    if(!config.exists()){
        QJsonObject jsonObject;
        jsonObject.insert("presetValueBehaviour", 1);
        jsonObject.insert("staticOffset", "0x044FF788");
        QMessageBox msgBox;
        msgBox.setText("The 'Static memory offset' setting has been set to 0x044FF788, the correct value for the latest Minecraft release version at the time of writing, 1.18.12. As this value can change depending on what Minecraft version you are using, you may need to change it in File->Preferences if you are using another version. Please consult the Github README (click Help->Usage) to find the correct value for you.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        QJsonDocument jsonDoc;
        jsonDoc.setObject(jsonObject);
        config.open(QIODevice::WriteOnly);
        config.write(jsonDoc.toJson());
    }
    config.close();
    if(readPreferences()) {
        readJson(settingsJsonPath);
        GenerateUI();
        UIGenerated = true;
    }
    QLabel* label = new QLabel("RenderBender v0.2.0");
    statusBar()->addWidget(label);
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::selectJson(){
    QString path = QFileDialog::getOpenFileName(this,tr("Open JSON"),  QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).constFirst(), tr("JSON Files (*.json)"));
    if(path == ""){return;}
    utils::writeConfigProperty("settingsJSONpath", path);
    settingsJsonPath = path;
    readJson(path);
    GenerateUI();
    UIGenerated = true;
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
    float convertedValue = static_cast<float>(i);
    changingUIvalues = true;
    settings[index].slider->setValue(convertedValue*pow(10, defaultSpinboxPrecision));
    changingUIvalues = false;
    settings[index].write(convertedValue);

}
void MainWindow::onSettingValueChanged(int i){
    if(changingUIvalues)return;
    int index = sender()->objectName().replace("settingUIelement", "").toInt();
    if(settings[index].type == settingType::BOOL){
        bool convertedValue = i==Qt::Checked;
        settings[index].write(convertedValue);
    }else{
        changingUIvalues = true;
        settings[index].slider->setValue(i);
        changingUIvalues = false;
        settings[index].write(i);
    }
}
void MainWindow::onSliderValueChanged(int newValue){
    if(changingUIvalues)return;
    int index = sender()->objectName().replace("settingSlider", "").toInt();
    if(settings[index].type == settingType::FLOAT){
        float newValueCorrected = newValue/pow(10, defaultSpinboxPrecision);
        settings[index].write(newValueCorrected);
        QDoubleSpinBox *widget = (QDoubleSpinBox*)settings[index].widget;
        changingUIvalues = true;
        widget->setValue(newValueCorrected);
        changingUIvalues = false;
    }else{
        settings[index].write(newValue);
        QSpinBox *widget = (QSpinBox*)settings[index].widget;
        changingUIvalues = true;
        widget->setValue(newValue);
        changingUIvalues = false;
    }
}
void MainWindow::onResetToPresetButtonClicked(){
    changingUIvalues = true;
    int index = sender()->objectName().replace("settingResetButton", "").toInt();
    settings[index].resetToPreset();
    changingUIvalues = false;
}
void MainWindow::GenerateUI(){    
    delete ui->selectJsonBtn;
    delete ui->selectJsonLabel;
    QJsonArray settingsJson = json.value(QString("settings")).toArray();
    QProgressDialog progress("Generating UI...", "stop", 0, settingsJson.count(), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(500);
    progress.setCancelButton(nullptr);

    int layoutCounter = 0;
    QList<QVBoxLayout *> layouts = centralWidget()->findChildren<QVBoxLayout *>();
    QStringList groupNames;
    changingUIvalues = true;
    for (int i = 0; i < settingsJson.count(); i++) {
        progress.setValue(i);
        //https://doc.qt.io/qt-5/qtabwidget.html#addTab
        QJsonObject obj = settingsJson[i].toObject();
        QString groupName = obj.value(QString("group")).toString();
        if(!groupNames.contains(groupName)){
            groupNames << groupName;
            //layoutCounter = groupNames.indexOf(groupName);
        }
        if(i != 0 && i%14 == 0){
            layoutCounter++;
        }
        QString type = obj.value(QString("type")).toString();
        QString infoText = obj.value(QString("displayName")).toString();
        QWidget *widget;
        QPushButton *resetButton = new QPushButton();
        resetButton->setText("Reset");
        if (type == "int"){
            QFrame *mainFrame = new QFrame();
            QVBoxLayout *mainLayout = new QVBoxLayout();
            QLabel *label = new QLabel(infoText);
            QWidget *subFrame = new QWidget();
            QHBoxLayout *subLayout = new QHBoxLayout();
            QSpinBox *spinBox = new QSpinBox();
            QSlider *slider = new QSlider(Qt::Orientation::Horizontal);

            int min = obj.value(QString("min")).toString().toInt();
            int max = obj.value(QString("max")).toString().toInt();
            setting s(sunAzimuthAddress, obj.value(QString("offset")).toString(), infoText.toLocal8Bit().data(),
            groupName.toLocal8Bit().data(), obj.value(QString("default")).toString().toInt(), min,
            max, i,  settingType::INT, spinBox, resetButton, slider);
            settings.push_back(s);
            int val;
            s.read(val);

            mainLayout->setSpacing(0);
            mainLayout->setContentsMargins(0, 0, 0, 0);
            subLayout->setSpacing(10);
            subLayout->setContentsMargins(0, 0, 0, 0);
            spinBox->setRange(min, max);
            spinBox->setValue(val);
            QObject::connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(onSettingValueChanged(int)));
            slider->setRange(min, max);
            slider->setValue(val);
            slider->setObjectName("settingSlider" + QString::number(i));
            slider->setStyleSheet("margin: 5 0 0 0;");
            QObject::connect(slider, SIGNAL(valueChanged(int)), this, SLOT(onSliderValueChanged(int)));

            subFrame->setLayout(subLayout);
            mainFrame->setLayout(mainLayout);
            mainLayout->insertWidget(0, label);
            subLayout->insertWidget(0, spinBox);
            subLayout->insertWidget(1, resetButton);
            mainLayout->insertWidget(1, subFrame);
            mainLayout->insertWidget(2, slider);
            layouts[layoutCounter]->insertWidget(0, mainFrame);
            widget = spinBox;
        }
        else if (type == "float"){
            QFrame *mainFrame = new QFrame();
            QVBoxLayout *mainLayout = new QVBoxLayout();
            QLabel* label = new QLabel(infoText);
            QWidget *subFrame = new QWidget();
            QHBoxLayout *subLayout = new QHBoxLayout();
            QDoubleSpinBox *spinBox = new QDoubleSpinBox();
            QSlider *slider = new QSlider(Qt::Orientation::Horizontal);

            float min = obj.value(QString("min")).toString().toFloat();
            float max = obj.value(QString("max")).toString().toFloat();
            setting s(sunAzimuthAddress, obj.value(QString("offset")).toString(), infoText.toLocal8Bit().data(),
            groupName.toLocal8Bit().data(),obj.value(QString("default")).toString().toFloat(), min,
            max, i, settingType::FLOAT, spinBox, resetButton, slider);
            settings.push_back(s);
            float val;
            s.read(val);

            mainLayout->setSpacing(0);
            mainLayout->setContentsMargins(0, 0, 0, 0);
            subLayout->setSpacing(10);
            subLayout->setContentsMargins(0, 0, 0, 0);
            spinBox->setRange(min, max);
            spinBox->setValue(val);
            QObject::connect(spinBox, SIGNAL(valueChanged(double)), this, SLOT(onSettingValueChanged(double)));
            spinBox->setDecimals(defaultSpinboxPrecision);
            slider->setRange(min*pow(10, defaultSpinboxPrecision), max*pow(10, defaultSpinboxPrecision));
            slider->setValue(val*pow(10, defaultSpinboxPrecision));
            slider->setObjectName("settingSlider" + QString::number(i));
            slider->setStyleSheet("margin: 5 0 0 0;");
            QObject::connect(slider, SIGNAL(valueChanged(int)), this, SLOT(onSliderValueChanged(int)));

            subFrame->setLayout(subLayout);
            mainFrame->setLayout(mainLayout);
            mainLayout->insertWidget(0, label);
            subLayout->insertWidget(0, spinBox);
            subLayout->insertWidget(1, resetButton);
            mainLayout->insertWidget(1, subFrame);
            mainLayout->insertWidget(2, slider);
            layouts[layoutCounter]->insertWidget(0, mainFrame);
            widget = spinBox;
        }
        else if (type == "bool"){
            QFrame *frame = new QFrame();
            QHBoxLayout *la = new QHBoxLayout();
            QCheckBox *cb = new QCheckBox(infoText);
            setting s(sunAzimuthAddress, obj.value(QString("offset")).toString(), infoText.toLocal8Bit().data(),
            groupName.toLocal8Bit().data(), obj.value(QString("default")).toString() == "1" || obj.value(QString("default")).toString() == "1", false,
            true, i, settingType::BOOL, cb, resetButton, nullptr);
            settings.push_back(s);
            bool val;
            s.read(val);
            la->insertWidget(0, cb);
            la->insertWidget(1, resetButton);
            frame->setLayout(la);
            layouts[layoutCounter]->insertWidget(0, frame);
            cb->setChecked(val);
            QObject::connect(cb, SIGNAL(stateChanged(int)), this, SLOT(onSettingValueChanged(int)));
            widget = cb;
        }
        else{
            continue;
        }

        QObject::connect(resetButton, &QPushButton::clicked, this, &MainWindow::onResetToPresetButtonClicked);
        resetButton->setObjectName("settingResetButton" + QString::number(i));
        widget->setStyleSheet("margin: 0 0 0 0;");
        widget->setObjectName("settingUIelement" + QString::number(i));
    }
    changingUIvalues = false;
    settingCount = settingsJson.count();
    progress.setValue(settingsJson.count());
    if(presetValueBehaviour == 2){
        on_actionSetPresetValues_triggered();
    }else if(presetValueBehaviour == 1){
        QMessageBox msgBox;
        msgBox.setText("Would you like to set all the values to their presets now? (you can always do this later from Edit->Reset all to preset value)");
        msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
        if(msgBox.exec() == QMessageBox::Yes){
            on_actionSetPresetValues_triggered();
        }
    }
}

void MainWindow::on_actionExit_triggered()
{
    QCoreApplication::quit();
}

void MainWindow::on_actionPreferences_triggered()
{
    metasettings = new metaSettings(this, staticOffset, presetValueBehaviour, settingsJsonPath);
    metasettings->setModal(true);
    if(metasettings->exec() == QDialog::Accepted){
        readPreferences();
        for (int i = 0; i < settingCount; i++) {
            settings[i].updateAddr();
        }
        on_actionReread_all_setting_values_triggered();
    }
}
bool MainWindow::readPreferences(){
    QFile file(QCoreApplication::applicationDirPath() +"/config.json");
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString rawText = file.readAll();
    file.close();
    QJsonDocument document = QJsonDocument::fromJson(rawText.toUtf8());
    QJsonObject obj = document.object();
    //TODO: handle incomplete config files properly
    //if(!(obj.contains("staticOffset")*obj.contains("presetValueBehaviour")*obj.contains("settingsJSONpath"))) return false;
    staticOffset = utils::hexToDec(obj.value(QString("staticOffset")).toString());
    presetValueBehaviour = obj.value(QString("presetValueBehaviour")).toInt();
    settingsJsonPath = obj.value(QString("settingsJSONpath")).toString();
    sunAzimuthAddress = getBaseWorkingAddress(staticOffset);
    return (settingsJsonPath != "");
}


void MainWindow::on_actionReread_all_setting_values_triggered()
{
    changingUIvalues = true;
    QProgressDialog progress("Rereading values...", "stop", 0, settingCount, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(500);
    progress.setCancelButton(nullptr);
    sunAzimuthAddress = getBaseWorkingAddress(staticOffset);
    for (int i = 0; i < settingCount; i++) {
        switch (settings[i].type) {
            case settingType::FLOAT:{
                float newVal = 0;
                settings[i].read(newVal);
                QDoubleSpinBox *widget = (QDoubleSpinBox*)settings[i].widget;
                widget->setValue(newVal);
                settings[i].slider->setValue(newVal*pow(10, defaultSpinboxPrecision));

                break;
            }
            case settingType::INT:{
                int newVal = 0;
                settings[i].read(newVal);
                QSpinBox *widget = (QSpinBox*)settings[i].widget;
                widget->setValue(newVal);
                settings[i].slider->setValue(newVal);
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
        progress.setValue(i);
    }
    changingUIvalues = false;
    progress.setValue(settingCount);
}

void MainWindow::on_actionSetPresetValues_triggered()
{
    changingUIvalues = true;
    QProgressDialog progress("Setting all values to their presets...", "stop", 0, settingCount, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(500);
    progress.setCancelButton(nullptr);
    for (int i = 0; i < settingCount; i++) {
        settings[i].resetToPreset();
        progress.setValue(i);
    }
    changingUIvalues = false;
    progress.setValue(settingCount);
}


void MainWindow::on_actionSave_preset_triggered()
{
    QProgressDialog progress("Saving current values as the new preset...", "stop", 0, settingCount, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(500);
    progress.setCancelButton(nullptr);
    for (int i = 0; i < settingCount; i++) {
        settings[i].presetIntake();
        progress.setValue(i/2);
    }

    QFile file(settingsJsonPath);
    file.open(QIODevice::ReadWrite | QIODevice::Text);
    QString rawText = file.readAll();
    QJsonDocument document = QJsonDocument::fromJson(rawText.toUtf8());
    QJsonObject jsonObject = document.object();
    QJsonArray settingsArray = jsonObject.value("settings").toArray();

    for (int i = 0; i < settingCount; i++) {
        QJsonObject setting = settingsArray.at(i).toObject();
        setting.insert("default", QString::number(settings[i].defaultValue));
        settingsArray.removeAt(i);
        settingsArray.insert(i, setting);
        progress.setValue(settingCount/2 + i/2);
    }

    jsonObject.insert("settings", settingsArray);
    QJsonDocument jsonDoc;
    jsonDoc.setObject(jsonObject);
    file.resize(0);
    file.write(jsonDoc.toJson());
    file.close();
    progress.setValue(settingCount);
    QMessageBox msgBox;
    msgBox.setText("The new preset has been saved.");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}


void MainWindow::on_actionUsage_triggered()
{
    QDesktopServices::openUrl(QUrl("https://github.com/SpeedyCodes/RenderBender#Usage"));
}

