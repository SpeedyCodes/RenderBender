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

#include "oldcepresetimportdialog.h"
#include "ui_oldcepresetimportdialog.h"
#include <qstringlist.h>
#include <qdebug.h>
#include <utils.h>
#include <QPushButton>

QStringList settingNamesDuplicate;
std::vector<bool> activeSettings;
std::vector<float> settingValues;

oldCEPresetImportdialog::oldCEPresetImportdialog(QWidget *parent, QStringList *names) : QDialog(parent), 
ui(new Ui::oldCEPresetImportdialog)
{
    ui->setupUi(this);
    settingNamesDuplicate = *names;
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

oldCEPresetImportdialog::~oldCEPresetImportdialog()
{
    delete ui;
}

void oldCEPresetImportdialog::on_stringInput_textChanged()
{
    QTextDocument *doc = ui->stringInput->document();
    QString text = doc->toPlainText();
    text = text.replace("local settings = ", "");
    text = text.replace("{", "");
    text = text.replace("}", "");
    QStringList pieces = text.split(",");
    QStringList names;
    std::vector<float> results;
    bool alternator = true;
    for (int i = 0; i < pieces.size(); i++)
    {
        if (alternator)
        {
            names.append(pieces[i].replace("'", "").replace("\"", "").trimmed());
        }
        else
        {
            results.push_back(pieces[i].toFloat());
        }
        alternator = !alternator;
    }
    if (names.size() != results.size())
    {
        ui->validationResultLabel->setText("The number of setting names is not equal to the number of setting values (did you forget a comma?)");
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        return;
    }
    for (int i = 0; i < names.size(); i++)
    {
        if (!settingNamesDuplicate.contains(names[i]))
        {
            ui->validationResultLabel->setText("The setting " + names[i] + " was not found as a setting name.");
            ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
            return;
        }
    }
    activeSettings.clear();
    settingValues.clear();
    for (int i = 0; i < settingNamesDuplicate.size(); i++)
    {
        auto it = std::find(names.begin(), names.end(), settingNamesDuplicate[i]);
        if (it != names.end())
        {
            int index = it - names.begin();
            activeSettings.push_back(true);
            settingValues.push_back(results[index]);
        }
        else
        {
            activeSettings.push_back(false);
            settingValues.push_back(0);
        }
    }
    ui->validationResultLabel->setText(QString::number(names.size()) + " settings ready to be saved correctly.");
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}
