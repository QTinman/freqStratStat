// Enable "enable_openapi": true, in apiserver config.json
// Go to http://127.0.0.1:8080/docs#/ to get api key after login, it should be the same for all instances on one ip.

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "Delegates.h"
#include "DateParser.h"
#include "SettingsManager.h"
#include <QMessageBox>
#include <QAbstractButton>
#include <QPushButton>
#include <QTimer>

// Global data shared with relationDialog (consider refactoring in future)
QString appgroup = "stratreader";
QString strat;
QString firsttrade;
QStringList trademodel;



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_columns(7)
    , m_runOnce(0)
    , m_candles(0)
    , m_candlesPerDay(0)
    , m_errors(0)
    , m_marketStartDay(0)
    , m_marketDays(41)
{
    ui->setupUi(this);
    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    connect(ui->relation, SIGNAL(clicked()), this, SLOT(relation()));

    SettingsManager& settings = SettingsManager::instance();
    m_servers.append("Select server");
    m_servers.append(settings.loadSetting("servers").toStringList());

    QString apikey = settings.loadSetting("apikey").toString();
    if (apikey.isEmpty()) {
        ui->messages->setText("FreqUI API key missing, please enter one in settings.");
    }

    int markedfrom = QDate::currentDate().dayOfYear() - settings.loadSetting("markedfrom").toDate().dayOfYear();
    QDateTime marketage = QDateTime(QDate::currentDate().addDays(-markedfrom), QTime::currentTime());
    loadmarket(marketage);

    // Use QTimer instead of blocking delay
    QTimer::singleShot(1000, this, SLOT(onInitializationTimerComplete()));

    setGeometry(settings.loadSetting("position").toRect());
    ui->servers->addItems(m_servers);
    this->setWindowTitle("Strategy Statistics");
    connect(ui->servers, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [=](int index){ combo_refresh(index); });

    QStringList modellist = initializemodel();
    model = new QStandardItemModel(modellist.length() / m_columns, m_columns, this);
    model->setRowCount(modellist.length() / m_columns);
    model->setHeaderData(0, Qt::Horizontal, "Strat", Qt::DisplayRole);
    model->setHeaderData(1, Qt::Horizontal, "Trades", Qt::DisplayRole);
    model->setHeaderData(2, Qt::Horizontal, "First trade", Qt::DisplayRole);
    model->setHeaderData(3, Qt::Horizontal, "Avr. %Profit", Qt::DisplayRole);
    model->setHeaderData(4, Qt::Horizontal, "Avr. Profit", Qt::DisplayRole);
    model->setHeaderData(5, Qt::Horizontal, "Market change", Qt::DisplayRole);
    model->setHeaderData(6, Qt::Horizontal, "Change today", Qt::DisplayRole);
    ui->tableView->setModel(model);
    ui->tableView->setSortingEnabled(true);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView->sortByColumn(0, Qt::AscendingOrder);
    ui->tableView->setItemDelegateForColumn(2, new DateDelegate(this));
}


void MainWindow::onInitializationTimerComplete()
{
    m_runOnce = 0;
}

QStringList MainWindow::initializemodel()
{
    QStringList model;
    strat_download();
    model = m_modelDataList;
    return model;
}

void MainWindow::strat_download()
{
    QString server = ui->servers->currentText();
    if (server != "Select server") {
        SettingsManager& settings = SettingsManager::instance();
        int limits = settings.loadSetting("tradelimits").toInt();
        QUrl url = QUrl(QString("http://" + server + "/api/v1/trades?limit=" + QString::number(limits)));
        this->setWindowTitle("Strategy Statistics - Active server " + server);
        QString apikey = settings.loadSetting("apikey").toString();
        QString arg = "Basic " + apikey;
        QNetworkRequest request;
        manager->connectToHost(ui->servers->currentText().mid(0, ui->servers->currentText().indexOf(":")),
                               ui->servers->currentText().mid(ui->servers->currentText().indexOf(":") + 1, 4).toInt());
        request.setRawHeader(QByteArray("Authorization"), arg.toUtf8());
        request.setHeader(QNetworkRequest::ContentTypeHeader, QString("application/json"));
        request.setUrl(url);
        if (m_runOnce < 1) manager->get(request);
    }
}

void MainWindow::market_download()
{
    QDateTime cdt = QDateTime::currentDateTime();
    long startdate = m_marketDate.msecsTo(cdt);
    m_marketDays = startdate / 86400000;
    m_marketStartDay = QDateTime::currentMSecsSinceEpoch() - startdate;
    m_timeframe = gettimeframe();
    m_candles = m_marketDays * m_candlesPerDay;
    QUrl url = QUrl(QString("https://www.binance.com/api/v3/klines?symbol=BTCUSDT&interval=" + m_timeframe +
                           "&limit=" + QString::number(m_candles)) + "&startTime=" + QString::number(m_marketStartDay));
    QNetworkRequest request(url);
    if (m_runOnce < 1) manager->get(request);
}

void MainWindow::loadmarket(QDateTime date)
{
    m_marketDate = date;
    market_download();
}

double MainWindow::readmarket(QString last_date, QString open_date)
{ // "2021-06-30 14:39:06"   "2021-06-23 16:26:06"
    QDateTime cdt = QDateTime::currentDateTime();
    int candle = m_timeframe.mid(0, 1).toInt() * 3600000;

    // Use DateParser to parse dates
    QDateTime openDateTime = DateParser::parseFreqTradeDate(open_date);
    QDateTime lastDateTime = DateParser::parseFreqTradeDate(last_date);

    long startdate = openDateTime.msecsTo(cdt);
    long enddate = lastDateTime.msecsTo(cdt);
    startdate = QDateTime::currentMSecsSinceEpoch() - startdate;
    enddate = QDateTime::currentMSecsSinceEpoch() - enddate;

    // Use OHLCDataContainer's calculatePriceChange method
    double change = m_marketData.calculatePriceChange(startdate, enddate, candle);
    return change;
}

void MainWindow::replyFinished (QNetworkReply *reply)
{
    if(reply->error())
    {
        QByteArray rawtable = reply->readAll();
        qDebug() << "Error: " << reply->error() <<
        ", Message: " << reply->errorString() <<
        ", Code: " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() <<
        ", Description: " << rawtable;
        ui->messages->setText("Error: " + reply->errorString());
        m_errors++;
    }
    else
    {
        m_errors = 0;
        reply->deleteLater();
        QByteArray rawtable = reply->readAll();
        if (rawtable.mid(2, 5) == "trade") {
            strat2table(rawtable);
            ui->messages->clear();
        }
        else if (!rawtable.isEmpty()) market2table(rawtable);
    }
}

void MainWindow::market2table(QByteArray rawtable)
{
    QJsonParseError parserError;
    QJsonDocument cryptolist = QJsonDocument::fromJson(rawtable, &parserError);

    // Clear existing market data
    m_marketData.clear();

    for (int i = 0; i < m_candles; i++)
    {
        qint64 epocdate = static_cast<qint64>(cryptolist[i][0].toDouble()); // 0=date
        double open = cryptolist[i][1].toString().toDouble();   // 1=open
        double high = cryptolist[i][2].toString().toDouble();   // 2=high
        double low = cryptolist[i][3].toString().toDouble();    // 3=low
        double close = cryptolist[i][4].toString().toDouble();  // 4=close 5=volume

        if (epocdate > 0) {
            m_marketData.addCandle(epocdate, open, high, low, close);
        }
    }

    QDate cd = QDate::currentDate();
    int today = (QDateTime::currentDateTime().currentMSecsSinceEpoch() + m_marketStartDay) / 86400000;
    cd = cd.addDays(-(m_candles / m_marketDays) - today);
    m_runOnce++;
}

void MainWindow::strat2table(QByteArray rawtable)
{
    QJsonArray jsonArray;
    QString close_date = "", start_date = "";
    double avr_profit = 0, avr_percent = 0, marketthen = 0, marketnow = 0;
    QDateTime tradestart, tradestartnow;
    QJsonParseError parserError;
    int average = 0, rows = 0, addedrows = 0;
    m_modelDataList.clear();
    trademodel.clear();
    strat = "";
    QJsonDocument cryptolist = QJsonDocument::fromJson(rawtable, &parserError);
    QJsonObject jsonObject = cryptolist.object();
    jsonArray = jsonObject["trades"].toArray();
    foreach (const QJsonValue & value, jsonArray) {
        QJsonObject data = value.toObject();
        rows++;
        if (rows == 1) {
            strat=data["strategy"].toString();
            avr_percent=data["close_profit_pct"].toDouble();
            avr_profit=data["profit_abs"].toDouble();
            start_date=data["open_date"].toString();
            average=1;
        }
        if (strat==data["strategy"].toString()) {
            avr_percent+=data["close_profit_pct"].toDouble();
            avr_profit+=data["profit_abs"].toDouble();
            close_date=data["close_date"].toString();
            average++;

        }
        if (strat!=data["strategy"].toString() || jsonArray.count() == rows) {
            tradestart = DateParser::parseFreqTradeDate(start_date);
            if (tradestart.isNull()) qDebug() << tradestart;
            QDateTime closeDateTime = DateParser::parseFreqTradeDate(close_date);
            qint64 tradetime = closeDateTime.msecsTo(tradestart);
            tradestartnow = QDateTime::currentDateTime().addMSecs(tradetime);
            m_modelDataList.append(strat);
            m_modelDataList.append(QString::number(average));
            if (!m_marketData.isEmpty()) marketthen = readmarket(close_date, start_date);
            if (!m_marketData.isEmpty()) marketnow = readmarket(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"), tradestartnow.toString("yyyy-MM-dd hh:mm:ss"));
            m_modelDataList.append(tradestart.toString("yyyy-MM-dd hh:mm:ss"));
            m_modelDataList.append(QLocale(QLocale::English).toString(avr_percent / average, 'F', 2));
            m_modelDataList.append(QLocale(QLocale::English).toString(avr_profit / average, 'F', 2));
            m_modelDataList.append(QLocale(QLocale::English).toString(marketthen, 'F', 2));
            m_modelDataList.append(QLocale(QLocale::English).toString(marketnow, 'F', 2));
            strat=data["strategy"].toString();
            avr_percent=data["close_profit_pct"].toDouble();
            avr_profit=data["profit_abs"].toDouble();
            start_date=data["open_date"].toString();
            addedrows++;
            average=1;
        }
        int trailingZeros = 2;
        if (data["stake_amount"].toDouble() < 0.001) trailingZeros = 7;
        if (data["stake_amount"].toDouble() < 0.1 && data["stake_amount"].toDouble() > 0.001) trailingZeros = 5;

        // Note: opendate and closedate variables removed - they were unused
        trademodel  << data["strategy"].toString() << data["open_date"].toString() << data["close_date"].toString() << data["pair"].toString()
                << data["enter_tag"].toString() << data["exit_reason"].toString()
                << QLocale(QLocale::English).toString(data["stake_amount"].toDouble(), 'F', trailingZeros)
                << QLocale(QLocale::English).toString(data["profit_pct"].toDouble(), 'F', 2)
                << QLocale(QLocale::English).toString(data["profit_abs"].toDouble(), 'F', trailingZeros); //
    }
    if (!m_modelDataList.isEmpty())
    if (m_modelDataList[(addedrows - 1) * 7] != strat) {
        tradestart = DateParser::parseFreqTradeDate(start_date);
        QDateTime closeDateTime = DateParser::parseFreqTradeDate(close_date);
        qint64 tradetime = closeDateTime.msecsTo(tradestart);
        tradestartnow = QDateTime::currentDateTime().addMSecs(tradetime);
        m_modelDataList.append(strat);
        m_modelDataList.append(QString::number(average));
        if (!m_marketData.isEmpty()) marketthen = readmarket(close_date, start_date);
        if (!m_marketData.isEmpty()) marketnow = readmarket(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"), tradestartnow.toString("yyyy-MM-dd hh:mm:ss"));
        m_modelDataList.append(tradestart.toString("yyyy-MM-dd hh:mm:ss"));
        m_modelDataList.append(QLocale(QLocale::English).toString(avr_percent / average, 'F', 2));
        m_modelDataList.append(QLocale(QLocale::English).toString(avr_profit / average, 'F', 2));
        m_modelDataList.append(QLocale(QLocale::English).toString(marketthen, 'F', 2));
        m_modelDataList.append(QLocale(QLocale::English).toString(marketnow, 'F', 2));
    }
    m_runOnce++;
    reload_model();
}


void MainWindow::reload_model()
{
    int row = 0, i = 0, col;
    QModelIndex index;
    QDateTime tradestart;
    QStringList modellist = initializemodel();
    model->setRowCount(modellist.length() / m_columns);
    while (i < modellist.length() - 1) {
       for (col = 0; col < m_columns; col++) {
         index = model->index(row, col, QModelIndex());
         if (col <= 1) model->setData(index, modellist[i]);
         if (col == 2) {
            tradestart = DateParser::parseFreqTradeDate(modellist[i]);
            model->setData(index, tradestart);
         }
         if (col >= 3) model->setData(index, modellist[i]);
         if (col >= 5) model->setData(index, modellist[i].toDouble());
         if (col >= 6) model->setData(index, modellist[i]);
         model->setData(index, Qt::AlignCenter, Qt::TextAlignmentRole);
         i++;
        }
      row++;
    }
}

QJsonArray MainWindow::ReadJson(const QByteArray &bytes)
{
    QJsonArray jsonArray;
    QJsonParseError parserError;
    QJsonDocument cryptolist = QJsonDocument::fromJson(bytes, &parserError);
    QJsonObject jsonObject = cryptolist.object();
    jsonArray = jsonObject["data"].toArray();
    return jsonArray;
}

MainWindow::~MainWindow()
{
    SettingsManager::instance().saveSetting("position", this->geometry());
    delete ui;
}



 /*   "trades": [
    {
      "trade_id": 1,
      "pair": "PERP/USD",
      "is_open": false,
      "exchange": "ftx",
      "amount": 5.8,
      "amount_requested": 5.8,
      "stake_amount": 55.44,
      "strategy": "CombinedBinHAndClucV6",
      "timeframe": 5,
      "fee_open": 0.0002,
      "fee_open_cost": 0.01098868,
      "fee_open_currency": "USD",
      "fee_close": 0.0002,
      "fee_close_cost": 0.011209080000000001,
      "fee_close_currency": "USD",
      "open_date": "2021-05-17 11:00:44",
      "open_timestamp": 1621249244934,
      "open_rate": 9.473,
      "open_rate_requested": 9.473,
      "open_trade_value": 54.95438868,
      "close_date": "2021-05-17 11:14:17",
      "close_timestamp": 1621250057346,
      "close_rate": 9.663,
      "close_rate_requested": 9.663,
      "close_profit": 0.01964906,
      "close_profit_pct": 1.96,
      "close_profit_abs": 1.07980224,
      "trade_duration_s": 812,
      "trade_duration": 13,
      "profit_ratio": 0.01964906,
      "profit_pct": 1.96,
      "profit_abs": 1.07980224,
      "sell_reason": "roi",
      "sell_order_status": "closed",
      "stop_loss_abs": 0.09663000000000009,
      "stop_loss_ratio": -0.99,
      "stop_loss_pct": -99,
      "stoploss_order_id": null,
      "stoploss_last_update": "2021-05-17 11:14:03",
      "stoploss_last_update_timestamp": 1621250043957,
      "initial_stop_loss_abs": 0.09473000000000009,
      "initial_stop_loss_ratio": -0.99,
      "initial_stop_loss_pct": -99,
      "min_rate": 9.424,
      "max_rate": 9.663,
      "open_order_id": null
    }
    */
void MainWindow::combo_refresh(int comboindex)
{
    m_runOnce = 0;
    reload_model();
}

void MainWindow::on_settings_clicked()
{
    settingsDialog settingsdialog;
    settingsdialog.setModal(true); // if nomodal is needed then create pointer inputdialog *datesearch; in mainwindow.h private section, then here use inputdialog = new datesearch(this); datesearch.show();
    settingsdialog.exec();
}

QString MainWindow::gettimeframe()
{
    QDateTime edt = QDateTime::currentDateTime();
    if (m_marketDays <= 41) {
        m_timeframe = "1h";
        m_candlesPerDay = 24;
    } else if (m_marketDays >= 42 && m_marketDays <= 82) {
        m_timeframe = "2h";
        m_candlesPerDay = 12;
    } else if (m_marketDays >= 83 && m_marketDays <= 164) {
        m_timeframe = "4h";
        m_candlesPerDay = 6;
    } else if (m_marketDays >= 165 && m_marketDays <= 329) {
        m_timeframe = "8h";
        m_candlesPerDay = 3;
    } else {
        m_timeframe = "1d";
        m_candlesPerDay = 1;
    }
    return m_timeframe;
}

void MainWindow::relation ()
{
    QModelIndexList indexes = ui->tableView->selectionModel()->selection().indexes();
    if (!indexes.isEmpty()) {
        ui->messages->clear();
        QModelIndex index = indexes.at(0);
        strat = model->data(index.sibling(index.row(),0)).toString();
        firsttrade = model->data(index.sibling(index.row(),2)).toString();
        relationDialog relationDialog;
        relationDialog.setModal(true);
        relationDialog.exec();
    } else ui->messages->setText("Select record");
}

void MainWindow::on_coffeecup_clicked()
{
    QMessageBox msgBox;
    QClipboard *clipboard=0;
    msgBox.setWindowTitle("Coffee");
    msgBox.setText("A coffee for creator \nBTC 1HJ5xJmePkfrYwixbZJaMUcXosiJhYRLbo\nETH/USDT 0x425c98102c43cd4d8e052Fd239B016dCb6CDa597\nAppreciated");
    QAbstractButton* pButtonYes = msgBox.addButton("Copy to clipboard", QMessageBox::YesRole);
    msgBox.exec();
    if (msgBox.clickedButton()==pButtonYes) {
        clipboard->setText("A coffee for creator \nBTC 1HJ5xJmePkfrYwixbZJaMUcXosiJhYRLbo\nETH/USDT 0x425c98102c43cd4d8e052Fd239B016dCb6CDa597\nAppreciated");
    }
}
