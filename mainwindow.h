#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore>
#include <QtGui>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include "settingsdialog.h"
#include "relationdialog.h"

extern QString appgroup, strat;
extern QStringList trademodel;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
signals:
    void processfinished(const QString &);
public:
    MainWindow(QWidget *parent = nullptr);
    QStringList initializemodel();
    ~MainWindow();
    QJsonArray ReadJson(const QByteArray &bytes);
    void createdb(QString table);
    void market_download();
    void strat_download();
    void combo_refresh(int index);
    void add_data(QString column1,QString column2);
    void loadmarket(QDateTime date);
    QString gettimeframe();
    QVariant loadsettings(QString settings);
    void savesettings(QString settings, QVariant attr);
    double readmarket(QString open_date, QString last_date);
    void market2table(QByteArray rawtable);
    void strat2table(QByteArray rawtable);

public slots:
    void replyFinished (QNetworkReply *reply);


private slots:
    void reload_model();
    void on_settings_clicked();
    void relation ();
    void delay(int msec);

private:
    Ui::MainWindow *ui;
    QStandardItemModel *model;
    QNetworkAccessManager *manager;
};
#endif // MAINWINDOW_H
