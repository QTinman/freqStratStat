#include "settingsdialog.h"
#include "ui_settingsdialog.h"

settingsDialog::settingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::settingsDialog)
{
    ui->setupUi(this);
    QStringList servers=loadsettings("servers").toStringList();
    ui->apikey->setText(loadsettings("apikey").toString());
    ui->markedfrom->setDate(loadsettings("markedfrom").toDate());
    if (ui->markedfrom->date() < QDate::currentDate().addDays(-2000)) ui->markedfrom->setDate(QDate::currentDate().addDays(-200));
    for(auto & a : servers) ui->servers->append(a);
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

    QSettings appsettings("QTinman",appgroup);
    appsettings.beginGroup(appgroup);
    appsettings.setValue("servers", QVariant::fromValue(servers));

    appsettings.endGroup();
    savesettings("tradelimits",ui->tradelimits->value());
    savesettings("apikey",ui->apikey->text());
    savesettings("markedfrom",ui->markedfrom->date());
}
