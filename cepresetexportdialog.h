#ifndef CEPRESETEXPORTDIALOG_H
#define CEPRESETEXPORTDIALOG_H

#include <QDialog>

namespace Ui {
class cepresetexportdialog;
}

class cepresetexportdialog : public QDialog
{
    Q_OBJECT

public:
    explicit cepresetexportdialog(QWidget *parent = nullptr, QStringList *presetTitles = {}, std::vector<std::vector<float>> *presetValues = {}, std::vector<QStringList> *presetSettingNames = {});
    ~cepresetexportdialog();

private slots:
    void on_presetSelectorBox_currentIndexChanged(int index);

    void on_clipBoardButton_clicked();

private:
    Ui::cepresetexportdialog *ui;
};

#endif // CEPRESETEXPORTDIALOG_H
