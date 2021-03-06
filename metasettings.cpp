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

#include "metasettings.h"
#include "ui_metasettings.h"
#include <QRegExpValidator>
#include <sstream>
#include <QDebug>
#include <utils.h>
#include <QStandardPaths>
#include <QFileDialog>
#include <QMessageBox>

uintptr_t staticOffsetTransferVar;
bool loadPresetOnStartupTransferVar;
bool presetValueBehaviourTransferVar;
QString settingsJsonPathTransferVar = "";
bool autoMcStartupBehaviourTransferVar;
bool behaviourOnMcShutdownTransferVar;
int defaultPresetTransferVar;
metaSettings::metaSettings(QWidget *parent, uintptr_t statOff, bool presValBeh, QString settingsJsonPath, bool autoMcStartupBehaviour, bool behaviourOnMcShutdown, int defaultPreset, QStringList presetNames):
    QDialog(parent),
    ui(new Ui::metaSettings)
{
    ui->setupUi(this);
    staticOffsetTransferVar = statOff;
    presetValueBehaviourTransferVar = presValBeh;
    settingsJsonPathTransferVar = settingsJsonPath;
    autoMcStartupBehaviourTransferVar = autoMcStartupBehaviour;
    behaviourOnMcShutdownTransferVar = behaviourOnMcShutdown;
    for(int i = 0; i < presetNames.size(); i++){
        ui->presValBehBox->addItem(presetNames[i]);
    }
    if(presetNames.size() == 0){
        ui->presValBehBox->setEnabled(false);
        ui->loadPresetcheckBox->setEnabled(false);
    }
    defaultPresetTransferVar = defaultPreset;
    if (staticOffsetTransferVar != 0) ui->staticMemoryOffsetTxt->setText(utils::decToHex(staticOffsetTransferVar));
    ui->presValBehBox->setCurrentIndex(defaultPresetTransferVar);
    if (settingsJsonPathTransferVar != "") ui->JSONpathLabel->setText(settingsJsonPathTransferVar);
    ui->startupBehaviourCB->setChecked(autoMcStartupBehaviourTransferVar);
    ui->shutdownBehaviourCB->setChecked(behaviourOnMcShutdownTransferVar);
    ui->staticMemoryOffsetTxt->setValidator(new QRegExpValidator(QRegExp("0[xX][0-9a-fA-F]+"), ui->staticMemoryOffsetTxt));
    ui->loadPresetcheckBox->setChecked(presetValueBehaviourTransferVar);
}

metaSettings::~metaSettings()
{
    delete ui;
}

void metaSettings::on_staticMemoryOffsetTxt_textChanged(const QString &arg1)
{
    staticOffsetTransferVar = utils::hexToDec(arg1);
}

void metaSettings::on_presValBehBox_currentIndexChanged(int index)
{
    defaultPresetTransferVar = index;
}

void metaSettings::on_buttonBox_accepted()
{
    utils::writeConfigProperty("staticOffset", utils::decToHex(staticOffsetTransferVar));
    utils::writeConfigProperty("settingsJSONpath", settingsJsonPathTransferVar);
    utils::writeConfigProperty("presetValueBehaviour", presetValueBehaviourTransferVar);
    utils::writeConfigProperty("autoMcStartupBehaviour", autoMcStartupBehaviourTransferVar);
    utils::writeConfigProperty("behaviourOnMcShutdown", behaviourOnMcShutdownTransferVar);
    utils::writeConfigProperty("defaultPresetIndex", defaultPresetTransferVar);
}

void metaSettings::on_settingsJSONlocBtn_pressed()
{
    QString newPath = QFileDialog::getOpenFileName(this,tr("Open JSON"),  QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).constFirst(), tr("JSON Files (*.json)"));
    if(newPath != ""){
        settingsJsonPathTransferVar = newPath;
        QMessageBox msgBox;
        msgBox.setText("The new setting file's location will be saved as soon as you press OK. Please restart RenderBender afterwards for the changes to take effect.");
        msgBox.exec();
        ui->JSONpathLabel->setText(settingsJsonPathTransferVar);
    }
}


void metaSettings::on_startupBehaviourCB_stateChanged(int arg1)
{
    autoMcStartupBehaviourTransferVar = (arg1>0);
}


void metaSettings::on_shutdownBehaviourCB_stateChanged(int arg1)
{
    behaviourOnMcShutdownTransferVar = (arg1>0);
}


void metaSettings::on_loadPresetcheckBox_stateChanged(int arg1)
{
    presetValueBehaviourTransferVar = (arg1>0);
}

