#include "cepresetexportdialog.h"
#include "ui_cepresetexportdialog.h"
#include <QClipboard>

QStringList titles;
std::vector<std::vector<float>> values;
std::vector<QStringList> settingNames;

cepresetexportdialog::cepresetexportdialog(QWidget *parent, QStringList *presetTitles, std::vector<std::vector<float>> *presetValues, std::vector<QStringList> *presetSettingNames) :
    QDialog(parent),
    ui(new Ui::cepresetexportdialog)
{
    titles = *presetTitles;
    values = *presetValues;
    settingNames = *presetSettingNames;
    ui->setupUi(this);
    ui->presetSelectorBox->addItems(*presetTitles);
}
cepresetexportdialog::~cepresetexportdialog()
{
    delete ui;
}
void cepresetexportdialog::on_presetSelectorBox_currentIndexChanged(int index)
{
    QString s = "local settings = {";
    for(int i = 0; i < values[index].size(); i++){
        if(i != 0) s.append(",");
        s.append(QString("'%1', %2").arg(settingNames[index][i], QString::number(values[index][i])));
    }
    s.append("}");
    ui->exportText->setPlainText(s);
    ui->clipBoardButton->setEnabled(true);
}
void cepresetexportdialog::on_clipBoardButton_clicked()
{
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(ui->exportText->toPlainText());
    ui->clipBoardButton->setEnabled(false);
}
