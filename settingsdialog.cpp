#include "settingsdialog.h"
#include "ui_settingsdialog.h"

settingsDialog::settingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::settingsDialog)
{
    ui->setupUi(this);
    QStringList servers=loadsettings("servers").toStringList();
    QStringList keys=loadsettings("keys").toStringList();
    for(auto & a : servers) ui->servers->append(a);
    for(auto & a : keys) ui->keys->append(a);
    ui->tradelimits->setValue(loadsettings("tradelimits").toInt());
    if (ui->tradelimits->value() == 0) ui->tradelimits->setValue(500);
}

QVariant settingsDialog::loadsettings(QString settings)
{
    QVariant returnvar;
    QSettings appsettings("QTinman",appgroup);
    appsettings.beginGroup(appgroup);
    returnvar = appsettings.value(settings);
    appsettings.endGroup();
    return returnvar;
}

void settingsDialog::savesettings(QString settings, QVariant attr)
{
    QSettings appsettings("QTinman",appgroup);
    appsettings.beginGroup(appgroup);
    appsettings.setValue(settings,QVariant::fromValue(attr));
    appsettings.endGroup();
}


settingsDialog::~settingsDialog()
{
    delete ui;
}

void settingsDialog::on_buttonBox_accepted()
{
    QStringList servers=ui->servers->toPlainText().split("\n");
    QStringList keys=ui->keys->toPlainText().split("\n");
    QSettings appsettings("QTinman",appgroup);
    appsettings.beginGroup(appgroup);
    appsettings.setValue("servers", QVariant::fromValue(servers));
    appsettings.setValue("keys", QVariant::fromValue(keys));
    appsettings.endGroup();
    savesettings("tradelimits",ui->tradelimits->value());
}
