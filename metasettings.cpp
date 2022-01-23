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

uintptr_t staticOffsetTransferVar;
metaSettings::metaSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::metaSettings)
{
    ui->setupUi(this);
    ui->staticMemoryOffsetTxt->setValidator(new QRegExpValidator(QRegExp("0[xX][0-9a-fA-F]+"), ui->staticMemoryOffsetTxt));
}

metaSettings::~metaSettings()
{
    delete ui;
}

void metaSettings::on_staticMemoryOffsetTxt_textChanged(const QString &arg1)
{
    staticOffsetTransferVar = utils::hexToDec(arg1);
}
void metaSettings::updateStaticOffset(uintptr_t newValue){
    staticOffsetTransferVar = newValue;
    ui->staticMemoryOffsetTxt->setText(utils::decToHex(staticOffsetTransferVar));
}

