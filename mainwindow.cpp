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
#include <cepresetexportdialog.h>
#include <QScreen>
#include <QCompleter>
#include <QLineEdit>
#include <chrono>
#include <thread>
#include <QTime>

enum class settingType { INT, FLOAT, BOOL};
DWORD GetProcessId(const wchar_t* procName);
const int defaultSpinboxPrecision = 7;
QWidget *fadeOutTarget;
DWORD targetProcessID;
HANDLE targetProcessHandle;
using namespace std;
vector<uintptr_t> baseAdresses;
class setting {
public:
    QString displayName;
    QString description;
    int baseOffsetIndex;
    //char *shortName;
    uintptr_t addr;
    int offset;
    int group;
    int index;
    //char *description;
    bool hasDefault;
    float defaultValue;
    float minVal;
    float maxVal;
    QWidget* widget;
    QPushButton* resetButton;
    QSlider* slider;
    settingType type;
    bool isOverridden = false;
    //int precision;
    void updateAddr(){
        if(baseAdresses.size() > baseOffsetIndex) addr = baseAdresses[baseOffsetIndex] + offset;
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
    bool resetToDefault(){
        if(!hasDefault) return true;
        toggleOverride(false);
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
    void toggleOverride(bool state){
        if (type == settingType::BOOL) return;
        isOverridden = state;
        slider->setEnabled(!state);
        if(state){
            if(type == settingType::INT){
                QSpinBox* s = (QSpinBox*)widget;
                s->setRange(-INT_MAX, INT_MAX);
            }else{//type is float, bools cannot be overridden
                QDoubleSpinBox* s = (QDoubleSpinBox*)widget;
                s->setRange(-FLT_MAX, FLT_MAX);
            }
        }
        else{
            float clampedVal = std::clamp((float)slider->value(), minVal, maxVal);
            write(clampedVal);
            if(type == settingType::INT){
                QSpinBox* s = (QSpinBox*)widget;
                s->setRange(minVal, maxVal);
            }else{//type is float, bools cannot be overridden
                QDoubleSpinBox* s = (QDoubleSpinBox*)widget;
                s->setRange(minVal, maxVal);
            }
        }
    }
    //https://stackoverflow.com/questions/351845/finding-the-type-of-an-object-in-c
    setting(int offsetIndex, QString offsetString, QString name, QString desc, int groupIndex, float defaultVal, float min, float max, float i, settingType varType, QWidget* w, QPushButton* rb
            , QSlider* sl, bool hasDefaultValue){
        baseOffsetIndex = offsetIndex;
        displayName = name;
        description = desc;
        offset = utils::hexToDec(offsetString);
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
        hasDefault = hasDefaultValue;
        if(!hasDefault) resetButton->setVisible(false);
    }

};
void MainWindow::temporaryHighlightFade() {
    if(fadeOutTarget == nullptr) return;
    fadeOutTarget->setStyleSheet("");
    fadeOutTarget = nullptr;
    return;
}

uintptr_t getBaseWorkingAddress(DWORD procId, HANDLE hProcess, uintptr_t staticOffset, vector<unsigned int> baseOffsetList);
uintptr_t computeSettingAddress(int settingIndex, uintptr_t base, QJsonObject &json);
QJsonObject json;
vector<setting> settings;
uintptr_t staticOffset;
bool presetValueBehaviour;
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
QStringList presetTitles;
vector<vector<float>> presetValues;
vector<QStringList> presetSettingNames;
vector<vector<unsigned int>> basePointers = {};
int defaultPreset;
bool isTargetProcessRestarting = false;

void MainWindow::updateStatusBar(int targetMessage){
    //targetMessage: 0->disconnected from mc process, 1->connected to mc process
    statusBar()->removeWidget(label);
    switch (targetMessage) {
        case 0:
            label = new QLabel("RenderBender " + utils::version + " | Disconnected from target process");
            statusBar()->addWidget(label);
            connectedToTargetProcess = false;
            break;
        case 1:
            label = new QLabel("RenderBender " + utils::version + " | Connected to target process");
            statusBar()->addWidget(label);
            connectedToTargetProcess = true;
            break;
    }
}
void CALLBACK mcShutdownHandler(void* lpParameter, bool TimerOrWaitFired)
{
    if(onMcShutdownBehaviour == true && !isTargetProcessRestarting){
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
        jsonObject.insert("presetValueBehaviour", false);
        jsonObject.insert("staticOffset", "0x044B0080");
        jsonObject.insert("autoMcStartupBehaviour", false);
        jsonObject.insert("behaviourOnMcShutdown", false);
        jsonObject.insert("defaultPresetIndex", 0);
        jsonObject.insert("enableHighDpiScaling", true);
        QMessageBox msgBox;
        msgBox.setText("The 'Static memory offset' setting has been set to 0x044B0080, the correct value for the latest Minecraft release version at the time of writing, 1.19.20. As this value can change depending on what Minecraft version you are using, you may need to change it in File->Preferences if you are using another version. Please consult the Github README (click Help->Usage) to find the correct value for you.");
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
    if(settingsJsonPath != "") {
        readJson(settingsJsonPath);
        GenerateUI();
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
    RecalculateBaseAdresses();
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
    if(path == "") return;
    utils::writeConfigProperty("settingsJSONpath", path);
    settingsJsonPath = path;
    readJson(path);
    GenerateUI();
}
void MainWindow::readJson(QString path){
    //make sure local filepaths are used starting from executable directory path
    QDir executableDir(QCoreApplication::applicationDirPath());
    QString absoluteJsonDir = executableDir.absoluteFilePath(path);

    QFile file;
    file.setFileName(absoluteJsonDir);
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
    settings[index].resetToDefault();
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
    QJsonArray basePointersJson = json.value(QString("pointerLists")).toArray();
    for(int i = 0; i < basePointersJson.size(); i++){
        vector<unsigned int> pointerList;
        QJsonArray basePointerListJson = basePointersJson[i].toArray();
        for(int j = 0; j < basePointerListJson.size(); j++){
            pointerList.push_back(utils::hexToDec(basePointerListJson[j].toString()));
        }
        basePointers.push_back(pointerList);
    }
    attachToTargetProcess();
    QJsonArray settingsJson = json.value(QString("settings")).toArray();
    QProgressDialog progress("Generating UI...", "stop", 0, settingsJson.count(), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(500);
    progress.setCancelButton(nullptr);
    QStringList groupNames;
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
        QString description = obj.value(QString("description")).toString();
        QPushButton *infoButton;
        if (description != "null"){
            infoButton = new QPushButton();
            infoButton->setText("What's this?");
            infoButton->setObjectName("infoButton" + QString::number(i));
            infoButton->setToolTip(description);
        } else{
            infoButton = nullptr;
        }
        QJsonValue defaultValObj = obj.value(QString("default"));
        bool hasDefault = (defaultValObj.toString() != "null");
        float defaultVal = defaultValObj.toDouble();
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
            QWidget *titleFrame = new QWidget();
            QHBoxLayout *titleLayout = new QHBoxLayout();
            QWidget *subFrame = new QWidget();
            QHBoxLayout *subLayout = new QHBoxLayout();
            QSpinBox *spinBox = new QSpinBox();
            QSlider *slider = new QSlider(Qt::Orientation::Horizontal);
            QCheckBox *overrideCB = new QCheckBox();
            overrideCB->setText("Override Min/Max");
            overrideCB->setObjectName("overrideCB" + QString::number(i));

            int min = obj.value(QString("min")).toInt();
            int max = obj.value(QString("max")).toInt();
            setting s(obj.value(QString("pointerIndex")).toInt(), obj.value(QString("offset")).toString(), infoText.toLocal8Bit().data(), description,
            groupIndex, defaultVal, min, max, i,  settingType::INT, spinBox, resetButton, slider, hasDefault);
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
            QObject::connect(overrideCB, SIGNAL(stateChanged(int)), this, SLOT(onRangeOverrideToggle(int)));

            titleFrame->setLayout(titleLayout);
            subFrame->setLayout(subLayout);
            mainFrame->setLayout(mainLayout);
            mainLayout->insertWidget(0, titleFrame);
            titleLayout->insertWidget(0, label);
            if(infoButton != nullptr) titleLayout->insertWidget(1, infoButton);
            subLayout->insertWidget(0, spinBox);
            subLayout->insertWidget(1, resetButton);
            subLayout->insertWidget(2, overrideCB);
            mainLayout->insertWidget(1, subFrame);
            mainLayout->insertWidget(2, slider);
            currentContainerDiv->insertWidget(0, mainFrame);
            widget = spinBox;
        }
        else if (type == "float"){
            QFrame *mainFrame = new QFrame();
            QVBoxLayout *mainLayout = new QVBoxLayout();
            QLabel* label = new QLabel(infoText);
            QWidget *titleFrame = new QWidget();
            QHBoxLayout *titleLayout = new QHBoxLayout();
            QWidget *subFrame = new QWidget();
            QHBoxLayout *subLayout = new QHBoxLayout();
            QDoubleSpinBox *spinBox = new QDoubleSpinBox();
            QSlider *slider = new QSlider(Qt::Orientation::Horizontal);
            QCheckBox *overrideCB = new QCheckBox();
            overrideCB->setText("Override Min/Max");
            overrideCB->setObjectName("overrideCB" + QString::number(i));

            float min = obj.value(QString("min")).toDouble();
            float max = obj.value(QString("max")).toDouble();
            setting s(obj.value(QString("pointerIndex")).toInt(), obj.value(QString("offset")).toString(), infoText.toLocal8Bit().data(), description,
            groupIndex,defaultVal, min, max, i, settingType::FLOAT, spinBox, resetButton, slider, hasDefault);
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
            QObject::connect(overrideCB, SIGNAL(stateChanged(int)), this, SLOT(onRangeOverrideToggle(int)));

            titleFrame->setLayout(titleLayout);
            subFrame->setLayout(subLayout);
            mainFrame->setLayout(mainLayout);
            mainLayout->insertWidget(0, titleFrame);
            titleLayout->insertWidget(0, label);
            if(infoButton != nullptr) titleLayout->insertWidget(1, infoButton);
            subLayout->insertWidget(0, spinBox);
            subLayout->insertWidget(1, resetButton);
            subLayout->insertWidget(2, overrideCB);
            mainLayout->insertWidget(1, subFrame);
            mainLayout->insertWidget(2, slider);
            currentContainerDiv->insertWidget(0, mainFrame);
            widget = spinBox;
        }
        else if (type == "bool"){
            QFrame *frame = new QFrame();
            QHBoxLayout *la = new QHBoxLayout();
            QFrame *subFrame = new QFrame();
            QVBoxLayout *subLayout = new QVBoxLayout();
            QCheckBox *cb = new QCheckBox(infoText);

            setting s(obj.value(QString("pointerIndex")).toInt(), obj.value(QString("offset")).toString(), infoText.toLocal8Bit().data(), description,
            groupIndex, defaultVal == 1, false, true, i, settingType::BOOL, cb, resetButton, nullptr, hasDefault);
            settings.push_back(s);
            bool val;
            s.read(val);
            subLayout->insertWidget(0, resetButton);
            if(infoButton != nullptr) subLayout->insertWidget(1, infoButton);
            subFrame->setLayout(subLayout);
            la->insertWidget(0, cb);
            la->insertWidget(1, subFrame);
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

    savepresetdialog = new savePresetDialog(this, &settingNames);
    savepresetdialog->setModal(true);
    progress.setValue(settingsJson.count());
    if(presetValueBehaviour){
        loadPreset(defaultPreset);
    }
    resize(QGuiApplication::primaryScreen()->availableGeometry().size() * 0.6);
    UIGenerated = true;
}

void MainWindow::on_actionExit_triggered()
{
    QCoreApplication::quit();
}

void MainWindow::on_actionPreferences_triggered()
{
    metasettings = new metaSettings(this, staticOffset, presetValueBehaviour, settingsJsonPath, autoMcStartupBehaviour, onMcShutdownBehaviour, defaultPreset, presetTitles);
    metasettings->setModal(true);
    if(metasettings->exec() == QDialog::Accepted){
        readPreferences();
        for (int i = 0; i < settingCount; i++) {
            settings[i].updateAddr();
        }
        RecalculateBaseAdresses();
        on_actionReread_all_setting_values_triggered();
    }
}
void MainWindow::readPreferences(bool onlyPresets){
    QFile file(QCoreApplication::applicationDirPath() +"/config.json");
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QString rawText = file.readAll();
    file.close();
    QJsonDocument document = QJsonDocument::fromJson(rawText.toUtf8());
    QJsonObject obj = document.object();
    //TODO: handle incomplete config files properly
    //if(!(obj.contains("staticOffset")*obj.contains("presetValueBehaviour")*obj.contains("settingsJSONpath"))) return false;
    QJsonArray presets = obj.value(QString("presets")).toArray();
    QMenu * m_load = ui->menuLoad_stored_preset;
    QMenu * m_delete = ui->menuDelete_preset;
    presetTitles.clear();
    m_load->clear();
    m_delete->clear();
    presetValues.clear();
    presetSettingNames.clear();
    if(!onlyPresets){
        staticOffset = utils::hexToDec(obj.value(QString("staticOffset")).toString());
        presetValueBehaviour = obj.value(QString("presetValueBehaviour")).toBool();
        settingsJsonPath = obj.value(QString("settingsJSONpath")).toString();
        autoMcStartupBehaviour = obj.value(QString("autoMcStartupBehaviour")).toBool();
        onMcShutdownBehaviour = obj.value(QString("behaviourOnMcShutdown")).toBool();
        defaultPreset = obj.value(QString("defaultPresetIndex")).toInt();
        if(!obj.value(QString("hasRunBefore")).toBool()){
            QMessageBox msgBox;
            msgBox.setText("The 'Static memory offset' setting has been set to 0x04177558, the correct value for the latest Minecraft release version at the time of writing, 1.19.2. As this value can change depending on what Minecraft version you are using, you may need to change it in File->Preferences if you are using another version. Please consult the Github README (click Help->Usage) to find the correct value for you.");
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();
            utils::writeConfigProperty("hasRunBefore", true);
        }
    }
    for(int presetCounter = 0;presetCounter < presets.size(); presetCounter++){
        vector<float> values;
        QStringList names;
        QJsonObject preset = presets[presetCounter].toObject();
        QString title = preset.value(QString("title")).toString();
        presetTitles.push_back(title);
        QAction* action_load = m_load->addAction(title);
        QAction* action_delete = m_delete->addAction(title);
        connect(action_load, SIGNAL(triggered()), SLOT(loadPresetFromButton()));
        connect(action_delete, SIGNAL(triggered()), SLOT(deletePreset()));
        QJsonObject valuesObject = preset.value(QString("values")).toObject();
        QStringList keys = valuesObject.keys();
        for(int j = 0; j < keys.size(); j++){
            values.push_back(valuesObject[keys[j]].toDouble());
            names.push_back(keys[j]);
        }
        presetValues.push_back(values);
        presetSettingNames.push_back(names);
    }
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

void MainWindow::on_actionSave_preset_triggered()
{
    vector<bool> presetEnabledList;
    if(savepresetdialog->exec() == QDialog::Accepted){
        presetEnabledList = savepresetdialog->readToggles();
    } else return;
    QString title = savepresetdialog->titleTransferVar;
    QProgressDialog progress("Saving current values as the new preset...", "stop", 0, settingCount, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(500);
    progress.setCancelButton(nullptr);

    QFile file(QCoreApplication::applicationDirPath() +"/config.json");
    file.open(QIODevice::ReadWrite | QIODevice::Text);
    QString rawText = file.readAll();
    QJsonDocument document = QJsonDocument::fromJson(rawText.toUtf8());
    QJsonObject jsonObject = document.object();

    QJsonArray presets = jsonObject.value(QString("presets")).toArray();
    QJsonObject newPreset;
    newPreset.insert("title", title);
    QJsonObject newPresetValues;
    for (int i = 0; i < settingCount; i++) {
        if(presetEnabledList[i]){
            switch (settings[i].type) {
                case settingType::FLOAT:{
                    QDoubleSpinBox *w = (QDoubleSpinBox*)settings[i].widget;
                    newPresetValues.insert(settings[i].displayName, (float)w->value());
                    break;
                }
                case settingType::INT:{
                    QSpinBox *w = (QSpinBox*)settings[i].widget;
                    newPresetValues.insert(settings[i].displayName, w->value());
                    break;
                }
                case settingType::BOOL:{
                    QCheckBox *w = (QCheckBox*)settings[i].widget;
                    newPresetValues.insert(settings[i].displayName, w->isChecked());
                    break;
                }
            }
        }
        progress.setValue(i);
    }
    newPreset.insert("values", newPresetValues);
    presets.append(newPreset);
    jsonObject.insert("presets", presets);
    QJsonDocument jsonDoc;
    jsonDoc.setObject(jsonObject);
    file.resize(0);
    file.write(jsonDoc.toJson());
    file.close();
    progress.setValue(settingCount);
    readPreferences(true);
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
    vector<float> settingValues;
    if(dialog->exec() == QDialog::Accepted){
        presetEnabledList = dialog->activeSettings;
        settingValues = dialog->settingValues;
    } else return;

    QProgressDialog progress("Saving Lua string as the new preset...", "stop", 0, settingCount, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(500);
    progress.setCancelButton(nullptr);

    QFile file(QCoreApplication::applicationDirPath() +"/config.json");
    file.open(QIODevice::ReadWrite | QIODevice::Text);
    QString rawText = file.readAll();
    QJsonDocument document = QJsonDocument::fromJson(rawText.toUtf8());
    QJsonObject jsonObject = document.object();
    QJsonArray presets = jsonObject.value(QString("presets")).toArray();
    QJsonObject newPreset;
    newPreset.insert("title", dialog->title);
    QJsonObject newPresetValues;
    for (int i = 0; i < settingCount; i++) {
        if(presetEnabledList[i]){
            switch (settings[i].type) {
                case settingType::FLOAT:{
                    newPresetValues.insert(settings[i].displayName, (float)settingValues[i]);
                    break;
                }
                case settingType::INT:{
                    newPresetValues.insert(settings[i].displayName, (float)settingValues[i]);
                    break;
                }
                case settingType::BOOL:{
                    newPresetValues.insert(settings[i].displayName, (float)settingValues[i]);
                    break;
                }
            }
        }
        progress.setValue(i);
    }
    newPreset.insert("values", newPresetValues);
    presets.append(newPreset);
    jsonObject.insert("presets", presets);
    QJsonDocument jsonDoc;
    jsonDoc.setObject(jsonObject);
    file.resize(0);
    file.write(jsonDoc.toJson());
    file.close();
    progress.setValue(settingCount);
    readPreferences(true);
    QMessageBox msgBox;
    msgBox.setText("The new preset has been saved. Would you like to apply it immediately? (you can always do this later from Edit->Load Preset->...)");
    msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    if(msgBox.exec() == QMessageBox::Yes){
        loadPreset(presetTitles.size()-1);
    }
}


void MainWindow::on_actionAttach_triggered()
{
    attachToTargetProcess();
}
void MainWindow::loadPresetFromButton(){
    QAction* a = (QAction*)sender();
    loadPreset(presetTitles.indexOf(a->text()));
}
void MainWindow::loadPreset(int index){
    on_actionReread_all_setting_values_triggered();
    changingUIvalues = true;
    int presetIndex = index;
    for(int i = 0; i < settingCount; i++){
        int settingIndex = presetSettingNames[presetIndex].indexOf(settings[i].displayName);
        if(settingIndex == -1){
            settings[i].resetToDefault();
        }else{
            switch (settings[i].type) {
                case settingType::FLOAT:{
                    if(settings[i].write(presetValues[presetIndex][settingIndex]) == true){
                        QDoubleSpinBox *w = (QDoubleSpinBox*)settings[i].widget;
                        w->setValue(presetValues[presetIndex][settingIndex]);
                        settings[i].slider->setValue(presetValues[presetIndex][settingIndex]*pow(10, defaultSpinboxPrecision));
                    }
                    break;
                }
                case settingType::INT:{
                    int convertedValue = (int)presetValues[presetIndex][settingIndex];
                    if(settings[i].write(convertedValue) == true){
                        QSpinBox *w = (QSpinBox*)settings[i].widget;
                        w->setValue(convertedValue);
                        settings[i].slider->setValue(convertedValue);
                    }
                    break;
                }
                case settingType::BOOL:{
                    bool convertedValue = (bool)presetValues[presetIndex][settingIndex];
                    if(settings[i].write(convertedValue) == true){
                        QCheckBox *w = (QCheckBox*)settings[i].widget;
                        w->setChecked(convertedValue);
                    }
                    break;
                }
            }
        }
    }
    changingUIvalues = false;
}

void MainWindow::deletePreset(){
    QAction* a = (QAction*)sender();
    int presetIndex = presetTitles.indexOf(a->text());
    presetTitles.erase(presetTitles.begin() + presetIndex);
    ui->menuLoad_stored_preset->removeAction(ui->menuLoad_stored_preset->actions().at(presetIndex));
    ui->menuDelete_preset->removeAction(ui->menuDelete_preset->actions().at(presetIndex));
    presetValues.erase(presetValues.begin() + presetIndex);
    presetSettingNames.erase(presetSettingNames.begin() + presetIndex);

    QFile file(QCoreApplication::applicationDirPath() +"/config.json");
    file.open(QIODevice::ReadWrite | QIODevice::Text);
    QString rawText = file.readAll();
    QJsonDocument document = QJsonDocument::fromJson(rawText.toUtf8());
    QJsonObject jsonObject = document.object();
    QJsonArray presets = jsonObject.value(QString("presets")).toArray();
    presets.erase(presets.begin() + presetIndex);
    jsonObject.insert("presets", presets);
    QJsonDocument jsonDoc;
    jsonDoc.setObject(jsonObject);
    file.resize(0);
    file.write(jsonDoc.toJson());
    file.close();
}


void MainWindow::on_actionSetDefaultValues_triggered()
{
    changingUIvalues = true;
    QProgressDialog progress("Setting all values to their defaults...", "stop", 0, settingCount, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(500);
    progress.setCancelButton(nullptr);
    for (int i = 0; i < settingCount; i++) {
        settings[i].resetToDefault();
        progress.setValue(i);
    }
    changingUIvalues = false;
    progress.setValue(settingCount);
}

void MainWindow::RecalculateBaseAdresses(){
    baseAdresses.clear();
    for(int i = 0; i < basePointers.size(); i++){
        baseAdresses.push_back(getBaseWorkingAddress(targetProcessID,targetProcessHandle, staticOffset, basePointers[i]));
    }
}

void MainWindow::on_actionRestartProcess_triggered()
{
    isTargetProcessRestarting = true;
    WinExec("taskkill /IM Minecraft.Windows.exe /F", SW_HIDE);
    this_thread::sleep_for(chrono::milliseconds(1000));
    utils::runMinecraft();
    this_thread::sleep_for(chrono::milliseconds(10000));
    attachToTargetProcess();
    isTargetProcessRestarting = false;
    on_actionReread_all_setting_values_triggered();
}


void MainWindow::on_actionCEPresetExport_triggered()
{    
    cepresetexportdialog *dialog = new cepresetexportdialog(this, &presetTitles, &presetValues, &presetSettingNames);
    dialog->setModal(true);
    dialog->exec();
}
void MainWindow::onRangeOverrideToggle(int state, int i){
    int index = i;
    if (i == -1)index = sender()->objectName().replace("overrideCB", "").toInt();
    settings[index].toggleOverride(state == Qt::Checked);
}
