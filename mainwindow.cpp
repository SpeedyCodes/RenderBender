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
#include <QTimer>
#include <metasettings.h>
#include <utils.h>
#include <QMessageBox>
#include <QSlider>
#include <QProgressDialog>
#include <QDesktopServices>
#include <oldcepresetimportdialog.h>
#include <QScreen>
#include <QCompleter>
#include <QLineEdit>
#include<chrono>
#include<thread>

enum class settingType { INT, FLOAT, BOOL};
DWORD GetProcessId(const wchar_t* procName);
const int defaultSpinboxPrecision = 7;
QWidget *fadeOutTarget;
DWORD targetProcessID;
HANDLE targetProcessHandle;
class setting {
public:
    char *displayName;
    //char *shortName;
    uintptr_t* base;
    uintptr_t addr;
    int offset;
    int group;
    int index;
    //char *description;
    bool presetEnabled;
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
        ReadProcessMemory(targetProcessHandle, (BYTE*)addr, &resultVar, sizeof(resultVar), nullptr);
        resetButton->setEnabled((float)resultVar != (float)defaultValue);
    }
    template <typename T>
    bool write(T& value)
    {
        T settingValue = 0;
        ReadProcessMemory(targetProcessHandle, (BYTE*)addr, &settingValue, sizeof(settingValue), nullptr);
        if(settingValue == value){resetButton->setEnabled((float)settingValue != defaultValue); return true;} // value already correct, stop
        WriteProcessMemory(targetProcessHandle, (BYTE*)addr, &value, sizeof(value), nullptr);
        ReadProcessMemory(targetProcessHandle, (BYTE*)addr, &settingValue, sizeof(settingValue), nullptr);
        resetButton->setEnabled((float)settingValue != (float)defaultValue);
        return (settingValue == value); //value changed succesfully/failed to change value
    }
    bool resetToPreset(){
        if(!presetEnabled) return true;
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
    void presetIntake(bool enabled){
        presetEnabled = enabled;
        resetButton->setVisible(presetEnabled);
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
    void presetIntake(bool enabled, float value){
        presetEnabled = enabled;
        resetButton->setVisible(presetEnabled);
        switch (type) {
            case settingType::FLOAT:{
                defaultValue = value;
                break;
            }
            case settingType::INT:{
                defaultValue = (int)value;
                break;
            }
            case settingType::BOOL:{
                defaultValue = (bool)value;
                break;
            }
        }
    }
    //https://stackoverflow.com/questions/351845/finding-the-type-of-an-object-in-c
    setting(uintptr_t &baseAddr, QString offsetString, char *name, int groupIndex, float defaultVal, float min, float max, float i, settingType varType, QWidget* w, QPushButton* rb
            , QSlider* sl, bool hasPreset){
        displayName = name;
        offset = utils::hexToDec(offsetString);
        base = &baseAddr;
        updateAddr();
        group = groupIndex;
        index = i;
        type = varType;
        defaultValue = defaultVal;
        minVal = min;
        maxVal = max;
        widget = w;
        resetButton = rb;
        slider = sl;
        presetEnabled = hasPreset;
        if(!presetEnabled) resetButton->setVisible(false);
    }

};
void MainWindow::temporaryHighlightFade() {
    if(fadeOutTarget == nullptr) return;
    fadeOutTarget->setStyleSheet("");
    fadeOutTarget = nullptr;
    return;
}

uintptr_t getBaseWorkingAddress(DWORD procId, HANDLE hProcess, uintptr_t staticOffset);
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
QStringList settingNames;
bool autoMcStartupBehaviour;
int onMcShutdownBehaviour;
QLabel* label;
MainWindow* w;
bool connectedToTargetProcess = false;

void MainWindow::updateStatusBar(int targetMessage){
    //targetMessage: 0->disconnected from mc process, 1->connected to mc process
    statusBar()->removeWidget(label);
    switch (targetMessage) {
        case 0:
            label = new QLabel("RenderBender " + utils::version + " feature testing #1" + " | Disconnected from target process");
            statusBar()->addWidget(label);
            connectedToTargetProcess = false;
            break;
        case 1:
            label = new QLabel("RenderBender " + utils::version + " feature testing #1" + " | Connected to target process");
            statusBar()->addWidget(label);
            connectedToTargetProcess = true;
            break;
    }
}
void CALLBACK mcShutdownHandler(void* lpParameter, bool TimerOrWaitFired)
{
    if(onMcShutdownBehaviour == true){
        QCoreApplication::quit();
    }else if(onMcShutdownBehaviour == false){
        emit w->statusbarsignal(0) ;
    }
}

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->selectJsonBtn, SIGNAL(clicked()), this, SLOT(selectJson()));
    QFile config(QCoreApplication::applicationDirPath() +"/config.json");
    w = this;
    connect(this, SIGNAL(statusbarsignal(int)), SLOT(updateStatusBar(int)));
    if(!config.exists()){
        QJsonObject jsonObject;
        jsonObject.insert("presetValueBehaviour", 1);
        jsonObject.insert("staticOffset", "0x053D61C8");
        jsonObject.insert("autoMcStartupBehaviour", false);
        jsonObject.insert("behaviourOnMcShutdown", false);
        QMessageBox msgBox;
        msgBox.setText("The 'Static memory offset' setting has been set to 0x053D61C8, the correct value for the latest Minecraft release version at the time of writing, 1.18.31. As this value can change depending on what Minecraft version you are using, you may need to change it in File->Preferences if you are using another version. Please consult the Github README (click Help->Usage) to find the correct value for you.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
        QJsonDocument jsonDoc;
        jsonDoc.setObject(jsonObject);
        config.open(QIODevice::WriteOnly);
        config.write(jsonDoc.toJson());
    }
    config.close();
    readPreferences();
    if(GetProcessId(L"Minecraft.Windows.exe") == 0 && autoMcStartupBehaviour){
        utils::runMinecraft();
        this_thread::sleep_for(chrono::milliseconds(10000));
    }
    attachToTargetProcess();
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
void MainWindow::attachToTargetProcess(){
    targetProcessID = GetProcessId(L"Minecraft.Windows.exe");
    if(targetProcessID == 0){
        updateStatusBar(0);
        return;
    }
    targetProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, targetProcessID);
    sunAzimuthAddress = getBaseWorkingAddress(targetProcessID,targetProcessHandle, staticOffset);
    for (int i = 0; i < settingCount; i++) {
        settings[i].updateAddr();
    }
    HANDLE hNewHandle;
    RegisterWaitForSingleObject(&hNewHandle, targetProcessHandle , (WAITORTIMERCALLBACK)mcShutdownHandler, NULL, INFINITE, WT_EXECUTEONLYONCE);
    on_actionReread_all_setting_values_triggered();
    updateStatusBar(1);
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
    QLayoutItem *child;
    while ((child = ui->horizontalLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
    delete ui->centralwidget->layout();
    ui->centralwidget->setLayout(new QVBoxLayout);
    QWidget *searchWidget = new QWidget();
    searchWidget->setLayout(new QHBoxLayout());
    searchWidget->layout()->addWidget(new QLabel("Search"));
    QLineEdit* searchbox = new QLineEdit();
    searchWidget->layout()->addWidget(searchbox);
    ui->centralwidget->layout()->addWidget(searchWidget);
    QTabWidget *tabwidget = new QTabWidget();
    ui->centralwidget->layout()->addWidget(tabwidget);
    QJsonArray settingsJson = json.value(QString("settings")).toArray();
    QProgressDialog progress("Generating UI...", "stop", 0, settingsJson.count(), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(500);
    progress.setCancelButton(nullptr);
    QStringList groupNames;
    vector<bool> enabledPresets = {};
    changingUIvalues = true;
    for (int i = 0; i < settingsJson.count(); i++) {
        progress.setValue(i);
        //https://doc.qt.io/qt-5/qtabwidget.html#addTab
        QJsonObject obj = settingsJson[i].toObject();
        QString groupName = obj.value(QString("group")).toString();
        if(!groupNames.contains(groupName)){
            groupNames << groupName;
            QWidget* w = new QWidget;
            QHBoxLayout* layout = new QHBoxLayout;
            tabwidget->addTab(w, groupName);
            QWidget* div1 = new QWidget;
            QWidget* div2 = new QWidget;
            QWidget* div3 = new QWidget;
            QVBoxLayout* la1 = new QVBoxLayout;
            la1->insertStretch(-1);
            div1->setLayout(la1);
            QVBoxLayout* la2 = new QVBoxLayout;
            la2->insertStretch(-1);
            div2->setLayout(la2);
            QVBoxLayout* la3 = new QVBoxLayout;
            la3->insertStretch(-1);
            div3->setLayout(la3);
            layout->addWidget(div1);
            layout->addWidget(div2);
            layout->addWidget(div3);
            w->setLayout(layout);
        }
        QString type = obj.value(QString("type")).toString();
        QString infoText = obj.value(QString("displayName")).toString();
        settingNames << infoText;
        QString defaultVal = obj.value(QString("default")).toString();
        enabledPresets.push_back(defaultVal != "null");
        QWidget *widget;
        QPushButton *resetButton = new QPushButton();
        resetButton->setText("Reset");
        int groupIndex = groupNames.indexOf(groupName);
        QVBoxLayout* tabFrame = (QVBoxLayout*)tabwidget->widget(groupIndex)->layout();
        QVBoxLayout* currentContainerDiv;
        //currentContainerDiv = (QVBoxLayout*)tabFrame->itemAt(0)->widget()->layout();
        for (int i = 0; i < tabFrame->count(); i++) {
            QWidget* w = tabFrame->itemAt(i)->widget();
            if(!tabFrame->itemAt(i)->isEmpty() && w->layout()->count() < 10){
                currentContainerDiv = (QVBoxLayout*)w->layout();
                break;
            }
        }
        tabFrame->insertStretch(-1);
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
            groupIndex, defaultVal.toInt(), min,
            max, i,  settingType::INT, spinBox, resetButton, slider, defaultVal != "null");
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
            currentContainerDiv->insertWidget(0, mainFrame);
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
            groupIndex,defaultVal.toFloat(), min,
            max, i, settingType::FLOAT, spinBox, resetButton, slider, defaultVal != "null");
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
            currentContainerDiv->insertWidget(0, mainFrame);
            widget = spinBox;
        }
        else if (type == "bool"){
            QFrame *frame = new QFrame();
            QHBoxLayout *la = new QHBoxLayout();
            QCheckBox *cb = new QCheckBox(infoText);

            setting s(sunAzimuthAddress, obj.value(QString("offset")).toString(), infoText.toLocal8Bit().data(),
            groupIndex, defaultVal == "1", false,
            true, i, settingType::BOOL, cb, resetButton, nullptr, defaultVal != "null");
            settings.push_back(s);
            bool val;
            s.read(val);
            la->insertWidget(0, cb);
            la->insertWidget(1, resetButton);
            frame->setLayout(la);
            cb->setChecked(val);
            currentContainerDiv->insertWidget(0, frame);
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

    QCompleter *fileEditCompleter = new QCompleter(settingNames, this);
    fileEditCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    fileEditCompleter->setCompletionMode(QCompleter::InlineCompletion);
    searchbox->setCompleter(fileEditCompleter);
    connect(fileEditCompleter, QOverload<const QString &>::of(&QCompleter::highlighted),
        [=](const QString &text){
        if(fadeOutTarget != nullptr) temporaryHighlightFade();
        ((QFrame)settings[settingNames.indexOf(text)].widget).setFrameStyle(QFrame::Panel | QFrame::Raised);
        tabwidget->setCurrentIndex(settings[settingNames.indexOf(text)].group);
        if(settings[settingNames.indexOf(text)].type == settingType::BOOL){
            fadeOutTarget = settings[settingNames.indexOf(text)].widget->parentWidget();
        }else{
            fadeOutTarget = settings[settingNames.indexOf(text)].widget->parentWidget()->parentWidget();
        }

        fadeOutTarget->setStyleSheet("");
        fadeOutTarget->setStyleSheet("background-color: rgb(38, 50, 66)");
        QTimer::singleShot(1000, this, SLOT(temporaryHighlightFade()));
});

    savepresetdialog = new savePresetDialog(this, &settingNames, enabledPresets);
    savepresetdialog->setModal(true);
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
    resize(QGuiApplication::primaryScreen()->availableGeometry().size() * 0.5);
}

void MainWindow::on_actionExit_triggered()
{
    QCoreApplication::quit();
}

void MainWindow::on_actionPreferences_triggered()
{
    metasettings = new metaSettings(this, staticOffset, presetValueBehaviour, settingsJsonPath, autoMcStartupBehaviour, onMcShutdownBehaviour);
    metasettings->setModal(true);
    if(metasettings->exec() == QDialog::Accepted){
        readPreferences();
        for (int i = 0; i < settingCount; i++) {
            settings[i].updateAddr();
        }
        sunAzimuthAddress = getBaseWorkingAddress(targetProcessID,targetProcessHandle, staticOffset);
        on_actionReread_all_setting_values_triggered();
    }
}
void MainWindow::readPreferences(){
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
    autoMcStartupBehaviour = obj.value(QString("autoMcStartupBehaviour")).toBool();
    onMcShutdownBehaviour = obj.value(QString("behaviourOnMcShutdown")).toBool();
}


void MainWindow::on_actionReread_all_setting_values_triggered()
{
    changingUIvalues = true;
    QProgressDialog progress("Rereading values...", "stop", 0, settingCount, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(500);
    progress.setCancelButton(nullptr);
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
    vector<bool> presetEnabledList;
    if(savepresetdialog->exec() == QDialog::Accepted){
        presetEnabledList = savepresetdialog->readToggles();
    } else return;
    QProgressDialog progress("Saving current values as the new preset...", "stop", 0, settingCount, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(500);
    progress.setCancelButton(nullptr);
    for (int i = 0; i < settingCount; i++) {
        settings[i].presetIntake(presetEnabledList[i]);
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
        if(settings[i].presetEnabled) setting.insert("default", QString::number(settings[i].defaultValue));
        else setting.insert("default", "null");
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


void MainWindow::on_actionCEPresetImport_triggered()
{
    oldCEPresetImportdialog *dialog = new oldCEPresetImportdialog(this, &settingNames);
    dialog->setModal(true);
    vector<bool> presetEnabledList;
    std::vector<float> settingValues;
    if(dialog->exec() == QDialog::Accepted){
        presetEnabledList = dialog->activeSettings;
        settingValues = dialog->settingValues;
    } else return;

    QProgressDialog progress("Saving current values as the new preset...", "stop", 0, settingCount, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(500);
    progress.setCancelButton(nullptr);
    for (int i = 0; i < settingCount; i++) {
        settings[i].presetIntake(presetEnabledList[i], settingValues[i]);
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
        if(settings[i].presetEnabled) setting.insert("default", QString::number(settings[i].defaultValue));
        else setting.insert("default", "null");
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

    msgBox.setText("Would you like to set all the values to their presets now? (you can always do this later from Edit->Reset all to preset value)");
    msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    if(msgBox.exec() == QMessageBox::Yes){
        on_actionSetPresetValues_triggered();
    }
}


void MainWindow::on_actionAttach_triggered()
{
    attachToTargetProcess();
}

