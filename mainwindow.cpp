// Enable "enable_openapi": true, in apiserver config.json
// Go to http://127.0.0.1:8080/docs#/ to get api key after login, it should be the same for all instances on one ip.

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QStyledItemDelegate>
#include <QMessageBox>
#include <QAbstractButton>
#include <QPushButton>

class DateDelegate: public QStyledItemDelegate{
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QString displayText(const QVariant &value, const QLocale &locale) const{
        return locale.toString(value.toDateTime(), "d MMM - hh:mm");
    }
};

QString appgroup="stratreader",strat,timeframe;
QDateTime marketdate;
QStringList modeldatalist, servers, markets, trademodel;
int colums=7,limit=500;
int runonce=0,candles, candlesprday, errors;
long marketstartday,marketdays=41;



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
    //connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(reload_model()));
    //connect(this, SIGNAL (processfinished(const QString &)), this, SLOT(reload_model()));
    connect(ui->relation, SIGNAL(clicked()), this, SLOT(relation()));

    servers.append("Select server");
    servers.append(loadsettings("servers").toStringList());
    QStringList keys=loadsettings("keys").toStringList();
    if (keys.isEmpty()) savesettings("keys","API key here");
    int markedfrom=QDate::currentDate().dayOfYear()-loadsettings("markedfrom").toDate().dayOfYear();
    QDateTime marketage = QDateTime(QDate::currentDate().addDays(-markedfrom),QTime::currentTime());
    loadmarket(marketage);
    delay(1000);
    runonce=0;
    setGeometry(loadsettings("position").toRect());
    ui->servers->addItems(servers);
    //ui->servers->setCurrentIndex(1);
    this->setWindowTitle("Strategy Statistics");
    connect(ui->servers, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [=](int index){ combo_refresh(index); });

    QStringList modellist = initializemodel();
    //ui->tableView->setItemDelegateForColumn(2,  new DateDelegate);
    model = new QStandardItemModel(modellist.length()/colums,colums,this);
    model->setRowCount(modellist.length()/colums);
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
    ui->tableView->sortByColumn(0,Qt::AscendingOrder);

}


QStringList MainWindow::initializemodel()
{
    QStringList model;
    strat_download();
    model=modeldatalist;
    return model;
}

void MainWindow::strat_download()
{
    QString server=ui->servers->currentText();
    if (server != "Select server") {
        int limits=loadsettings("tradelimits").toInt();
        QUrl url = QUrl(QString("http://"+server+"/api/v1/trades?limit="+QString::number(limits)));
        this->setWindowTitle("Strategy Statistics - Active server "+server);
        int key=ui->servers->currentIndex()-1;
        QStringList keys=loadsettings("keys").toStringList();
        QString arg="Basic "+keys[key];
        QNetworkRequest request;
        manager->connectToHost(ui->servers->currentText().mid(0,ui->servers->currentText().indexOf(":")),ui->servers->currentText().mid(ui->servers->currentText().indexOf(":")+1,4).toInt());
        request.setRawHeader(QByteArray("Authorization"), arg.toUtf8());
        request.setHeader(QNetworkRequest::ContentTypeHeader,QString("application/json"));
        request.setUrl(url);
        if (runonce < 1) manager->get(request);
    }
}

void MainWindow::market_download()
{
    QDateTime cdt = QDateTime::currentDateTime();
    long startdate=marketdate.msecsTo(cdt);
    marketdays=startdate/86400000;
    marketstartday=QDateTime::currentMSecsSinceEpoch()-(startdate);
    timeframe = gettimeframe();
    candles=marketdays*candlesprday;
    QUrl url = QUrl(QString("https://www.binance.com/api/v3/klines?symbol=BTCUSDT&interval="+timeframe+"&limit="+QString::number(candles))+"&startTime="+QString::number(marketstartday));
    QNetworkRequest request(url);
    if (runonce < 1) manager->get(request);
}

QVariant MainWindow::loadsettings(QString settings)
{
    QVariant returnvar;
    QSettings appsettings("QTinman",appgroup);
    appsettings.beginGroup(appgroup);
    returnvar = appsettings.value(settings);
    appsettings.endGroup();
    return returnvar;
}

void MainWindow::savesettings(QString settings, QVariant attr)
{
    QSettings appsettings("QTinman",appgroup);
    appsettings.beginGroup(appgroup);
    appsettings.setValue(settings,QVariant::fromValue(attr));
    appsettings.endGroup();
}

void MainWindow::loadmarket(QDateTime date)
{
    marketdate=date;
    market_download();
}

double MainWindow::readmarket(QString last_date, QString open_date)
{ // "2021-06-30 14:39:06"   "2021-06-23 16:26:06"
    QDateTime cdt = QDateTime::currentDateTime();
    double firstprice=0,lastprice=0;
    double change=0;
    int counter=0, candle=timeframe.mid(0,1).toInt()*3600000;
    long startdate = QDateTime(QDate(open_date.mid(0,4).toInt(),open_date.mid(5,2).toInt(),open_date.mid(8,2).toInt()),QTime(open_date.mid(11,2).toInt(),open_date.mid(14,2).toInt(),0)).msecsTo(cdt);
    long enddate = QDateTime(QDate(last_date.mid(0,4).toInt(),last_date.mid(5,2).toInt(),last_date.mid(8,2).toInt()),QTime(last_date.mid(11,2).toInt(),last_date.mid(14,2).toInt(),0)).msecsTo(cdt);
    startdate=QDateTime::currentMSecsSinceEpoch()-startdate;
    enddate=QDateTime::currentMSecsSinceEpoch()-enddate;
    for (int r=0;r<markets.count();r+=5 ) {
      if (startdate < markets[r].toDouble()+candle && enddate > markets[r].toDouble()-candle) {
         if (counter==0) firstprice = markets[r+1].toDouble();
         lastprice = markets[r+4].toDouble();
         counter++;
      }
   }

   change=(lastprice/firstprice*100)-100;
   return change;
}

void MainWindow::replyFinished (QNetworkReply *reply)
{
    if(reply->error())
    {
        QByteArray rawtable=reply->readAll();
       //qDebug() << "ERROR! " << reply->errorString();
        qDebug() << "Error: " << reply->error() <<
        ", Message: " << reply->errorString() <<
        ", Code: " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() <<
        ", Description: " << rawtable;
        ui->messages->setText("Error: " + reply->errorString());
        errors++;
    }
    else
    {
        errors=0;
        reply->deleteLater();
        ui->messages->clear();
        QByteArray rawtable=reply->readAll();
        if (rawtable.mid(2,5) == "trade") strat2table(rawtable);
        else if (!rawtable.isEmpty()) market2table(rawtable);
    }
}

void MainWindow::market2table(QByteArray rawtable)
{
    QJsonParseError parserError;
    QJsonDocument cryptolist = QJsonDocument::fromJson(rawtable, &parserError);
    double low,high,open,close;
    for (int i=0;i<candles;i++)
    {
        double epocdate = cryptolist[i][0].toDouble(); // 0=date
        QString Qopen = cryptolist[i][1].toString(); // 1=open
        QString Qhigh = cryptolist[i][2].toString(); // 2=high
        QString Qlow = cryptolist[i][3].toString(); // 3=low
        QString Qclose = cryptolist[i][4].toString(); // 4=close 5=volume
        low = Qlow.toDouble();
        high = Qhigh.toDouble();
        open = Qopen.toDouble();
        close = Qclose.toDouble();
        if (epocdate > 0) {
        markets << QString::number(epocdate) << QString::number(open) << QString::number(high) << QString::number(low) << QString::number(close);
        }
    }
    QDate cd = QDate::currentDate();
    int today=(QDateTime::currentDateTime().currentMSecsSinceEpoch()+marketstartday)/86400000;
    cd = cd.addDays(-(candles/marketdays)-today);
    runonce++;
}

void MainWindow::strat2table(QByteArray rawtable)
{
    QJsonArray jsonArray;
    QString close_date="",start_date="";
    double avr_profit=0,avr_percent=0,marketthen=0,marketnow=0;
    QDateTime tradestart, tradestartnow;
    QJsonParseError parserError;
    int average=0,rows=0,addedrows=0;
    modeldatalist.clear();
    trademodel.clear();
    strat="";
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
            tradestart=QDateTime(QDate(start_date.mid(0,4).toInt(),start_date.mid(5,2).toInt(),start_date.mid(8,2).toInt()),QTime(start_date.mid(11,2).toInt(),start_date.mid(14,2).toInt(),0));
            qint64 tradetime=QDateTime(QDate(close_date.mid(0,4).toInt(),close_date.mid(5,2).toInt(),close_date.mid(8,2).toInt()),QTime(close_date.mid(11,2).toInt(),close_date.mid(14,2).toInt(),0)).msecsTo(tradestart);
            tradestartnow = QDateTime::currentDateTime().addMSecs(tradetime);
            modeldatalist.append(strat);
            modeldatalist.append(QString::number(average));
            if (!markets.isEmpty()) marketthen = readmarket(close_date,start_date);
            if (!markets.isEmpty()) marketnow = readmarket(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"),tradestartnow.toString("yyyy-MM-dd hh:mm:ss"));
            //modeldatalist.append(tradestart.toString("d MMM - hh:mm"));
            modeldatalist.append(tradestart.toString("yyyy-MM-dd hh:mm:ss"));
            modeldatalist.append(QLocale(QLocale::English).toString(avr_percent/average,'F',2));
            modeldatalist.append(QLocale(QLocale::English).toString(avr_profit/average,'F',2));
            modeldatalist.append(QLocale(QLocale::English).toString(marketthen,'F',2));
            modeldatalist.append(QLocale(QLocale::English).toString(marketnow,'F',2));
            strat=data["strategy"].toString();
            avr_percent=data["close_profit_pct"].toDouble();
            avr_profit=data["profit_abs"].toDouble();
            start_date=data["open_date"].toString();
            addedrows++;
            average=1;
        }
        int trailingZeros=2;
        if (data["stake_amount"].toDouble()<0.001) trailingZeros=7;
        if (data["stake_amount"].toDouble()<0.1&&data["stake_amount"].toDouble()>0.001) trailingZeros=5;
        QString opendate =  QDateTime(QDate(data["open_date"].toString().mid(0,4).toInt(),data["open_date"].toString().mid(5,2).toInt(),data["open_date"].toString().mid(8,2).toInt()),QTime(data["open_date"].toString().mid(11,2).toInt(),data["open_date"].toString().mid(14,2).toInt(),0)).toString("d MMM - hh:mm");
        QString closedate =  QDateTime(QDate(data["close_date"].toString().mid(0,4).toInt(),data["close_date"].toString().mid(5,2).toInt(),data["close_date"].toString().mid(8,2).toInt()),QTime(data["close_date"].toString().mid(11,2).toInt(),data["close_date"].toString().mid(14,2).toInt(),0)).toString("d MMM - hh:mm");
        trademodel  << data["strategy"].toString() << data["open_date"].toString() << data["close_date"].toString() << data["pair"].toString() << data["sell_reason"].toString()
                << QLocale(QLocale::English).toString(data["stake_amount"].toDouble(),'F',trailingZeros)
                << QLocale(QLocale::English).toString(data["profit_pct"].toDouble(),'F',2)
                << QLocale(QLocale::English).toString(data["profit_abs"].toDouble(),'F',trailingZeros); //
        //laststrat=strat;
    }

        //qDebug() << modeldatalist[(addedrows-1)*7] << " " << addedrows;
    if (!modeldatalist.isEmpty())
    if (modeldatalist[(addedrows-1)*7] != strat) {
        tradestart=QDateTime(QDate(start_date.mid(0,4).toInt(),start_date.mid(5,2).toInt(),start_date.mid(8,2).toInt()),QTime(start_date.mid(11,2).toInt(),start_date.mid(14,2).toInt(),0));
        qint64 tradetime=QDateTime(QDate(close_date.mid(0,4).toInt(),close_date.mid(5,2).toInt(),close_date.mid(8,2).toInt()),QTime(close_date.mid(11,2).toInt(),close_date.mid(14,2).toInt(),0)).msecsTo(tradestart);
        tradestartnow = QDateTime::currentDateTime().addMSecs(tradetime);
        modeldatalist.append(strat);
        modeldatalist.append(QString::number(average));
        if (!markets.isEmpty()) marketthen = readmarket(close_date,start_date);
        if (!markets.isEmpty()) marketnow = readmarket(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"),tradestartnow.toString("yyyy-MM-dd hh:mm:ss"));
        modeldatalist.append(tradestart.toString("d MMM - hh:mm"));
        modeldatalist.append(QLocale(QLocale::English).toString(avr_percent/average,'F',2));
        modeldatalist.append(QLocale(QLocale::English).toString(avr_profit/average,'F',2));
        modeldatalist.append(QLocale(QLocale::English).toString(marketthen,'F',2));
        modeldatalist.append(QLocale(QLocale::English).toString(marketnow,'F',2));
    }
    runonce++;
    reload_model();
}


void MainWindow::reload_model()
{
    int row=0,i=0,col;
    QModelIndex index;
    QDateTime tradestart;
    QStringList modellist = initializemodel();
    model->setRowCount(modellist.length()/colums);
    // "yyyy-MM-dd hh:mm:ss"
    //qDebug() << modellist.length();
    while (i < modellist.length()-1) {
       for (col=0;col<colums;col++) {
         index=model->index(row,col,QModelIndex());
         if (col <= 1) model->setData(index,modellist[i]);
         if (col == 2){
            tradestart=QDateTime(QDate(modellist[i].mid(0,4).toInt(),modellist[i].mid(5,2).toInt(),modellist[i].mid(8,2).toInt()),QTime(modellist[i].mid(11,2).toInt(),modellist[i].mid(14,2).toInt(),0));
            model->setData(index,tradestart);
         }
         if (col >= 3) model->setData(index,modellist[i]);
         if (col >= 5) model->setData(index,modellist[i].toDouble());
         if (col >= 6) model->setData(index,modellist[i]);
         model->setData(index, Qt::AlignCenter, Qt::TextAlignmentRole);
         ui->tableView->setItemDelegateForColumn(2,  new DateDelegate);
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
    savesettings("position",this->geometry());
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
    },
    */
void MainWindow::combo_refresh(int comboindex)
{
    runonce=0;
    reload_model();
}

void MainWindow::on_settings_clicked()
{
    settingsDialog settingsdialog;
    //QObject::connect(&settingsdialog, SIGNAL(destroyed()), this, SLOT(reload_model()));
    settingsdialog.setModal(true); // if nomodal is needed then create pointer inputdialog *datesearch; in mainwindow.h private section, then here use inputdialog = new datesearch(this); datesearch.show();
    settingsdialog.exec();
}

QString MainWindow::gettimeframe()
{

    QDateTime edt=QDateTime::currentDateTime();
    if (marketdays <= 41) {
        timeframe = "1h";
        candlesprday=24;
    } else if (marketdays >= 42 && marketdays <= 82) {
        timeframe = "2h";
        candlesprday=12;
    } else if (marketdays >= 83 && marketdays <= 164) {
        timeframe = "4h";
        candlesprday=6;
    } else if (marketdays >= 165 && marketdays <= 329) {
        timeframe = "8h";
        candlesprday=3;
    } else {
        timeframe = "1d";
        candlesprday=1;
    }

    return timeframe;
}

void MainWindow::delay(int msec)
{
    QTime dieTime= QTime::currentTime().addMSecs(msec);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void MainWindow::relation ()
{
    QModelIndexList indexes = ui->tableView->selectionModel()->selection().indexes();
    if (!indexes.isEmpty()) {
        ui->messages->clear();
        QModelIndex index = indexes.at(0);
        strat = model->data(index.sibling(index.row(),0)).toString();
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
