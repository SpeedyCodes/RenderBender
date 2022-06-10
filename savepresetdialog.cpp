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

#include "savepresetdialog.h"
#include "ui_savepresetdialog.h"
#include "qdebug.h"

std::vector<QListWidgetItem*> listCheckBoxes;
QString titleTransferVar;
savePresetDialog::savePresetDialog(QWidget *parent, QStringList *names, std::vector<bool> enabledPresets) :
    QDialog(parent),
    ui(new Ui::savePresetDialog)
{
    ui->setupUi(this);
    for (int i = 0;i < names->length();i++) {
        QListWidgetItem *listItem = new QListWidgetItem(names->at(i),ui->settingList);
        if (enabledPresets[i])listItem->setCheckState(Qt::Checked);
        else listItem->setCheckState(Qt::Unchecked);
        listCheckBoxes.push_back(listItem);
    }
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

std::vector<bool> savePresetDialog::readToggles(){
    std::vector<bool> values;
    for (int i = 0;i < listCheckBoxes.size();i++) {
        values.push_back(listCheckBoxes[i]->checkState() == Qt::Checked);
    }
    return values;
}

savePresetDialog::~savePresetDialog()
{
    delete ui;
}

void savePresetDialog::on_enableAllBtn_clicked()
{
    for (int i = 0;i < listCheckBoxes.size();i++) {
        listCheckBoxes[i]->setCheckState(Qt::Checked);
    }
}


void savePresetDialog::on_disableAllBtn_clicked()
{
    for (int i = 0;i < listCheckBoxes.size();i++) {
        listCheckBoxes[i]->setCheckState(Qt::Unchecked);
    }
}


void savePresetDialog::on_titleInput_textChanged(const QString &arg1)
{
    titleTransferVar = arg1;
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(arg1 != "");
}

