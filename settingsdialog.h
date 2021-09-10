#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QtCore>
#include "mainwindow.h"

namespace Ui {
class settingsDialog;
}

class settingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit settingsDialog(QWidget *parent = nullptr);
    QVariant loadsettings(QString settings);
    void savesettings(QString settings, QVariant attr);
    ~settingsDialog();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::settingsDialog *ui;
};

#endif // SETTINGSDIALOG_H
