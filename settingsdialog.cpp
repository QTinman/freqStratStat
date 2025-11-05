#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "SettingsManager.h"

settingsDialog::settingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::settingsDialog)
{
    ui->setupUi(this);
    SettingsManager& settings = SettingsManager::instance();
    QStringList servers = settings.loadSetting("servers").toStringList();
    ui->apikey->setText(settings.loadSetting("apikey").toString());
    ui->markedfrom->setDate(settings.loadSetting("markedfrom").toDate());
    if (ui->markedfrom->date() < QDate::currentDate().addDays(-2000)) ui->markedfrom->setDate(QDate::currentDate().addDays(-200));
    for(auto & a : servers) ui->servers->append(a);
    ui->tradelimits->setValue(settings.loadSetting("tradelimits").toInt());
    if (ui->tradelimits->value() == 0) ui->tradelimits->setValue(500);
}

settingsDialog::~settingsDialog()
{
    delete ui;
}

void settingsDialog::on_buttonBox_accepted()
{
    SettingsManager& settings = SettingsManager::instance();
    QStringList servers = ui->servers->toPlainText().split("\n");

    settings.saveSetting("servers", servers);
    settings.saveSetting("tradelimits", ui->tradelimits->value());
    settings.saveSetting("apikey", ui->apikey->text());
    settings.saveSetting("markedfrom", ui->markedfrom->date());
}
