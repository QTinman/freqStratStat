#ifndef MAINWINDOW_H
#define MAINWINDOW_H

//#include <QMainWindow>
#include <QtCore>
#include <QtGui>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include "settingsdialog.h"
#include "relationdialog.h"
#include "OHLCData.h"

// Global data shared with relationDialog (will be refactored in future)
extern QString appgroup, strat, firsttrade;
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
    double readmarket(QString open_date, QString last_date);
    void market2table(QByteArray rawtable);
    void strat2table(QByteArray rawtable);

public slots:
    void replyFinished (QNetworkReply *reply);


private slots:
    void reload_model();
    void on_settings_clicked();
    void relation();
    void on_coffeecup_clicked();
    void onInitializationTimerComplete();

private:
    Ui::MainWindow *ui;
    QStandardItemModel *model;
    QNetworkAccessManager *manager;

    // Data members (previously global variables)
    QString m_timeframe;
    QDateTime m_marketDate;
    QStringList m_modelDataList;
    QStringList m_servers;
    OHLCDataContainer m_marketData;  // Replaces flat QStringList markets

    int m_columns;
    int m_runOnce;
    int m_candles;
    int m_candlesPerDay;
    int m_errors;
    long m_marketStartDay;
    long m_marketDays;
};
#endif // MAINWINDOW_H
