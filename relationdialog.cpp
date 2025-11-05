#include "relationdialog.h"
#include "ui_relationdialog.h"
#include "mainwindow.h"
#include <QStyledItemDelegate>

class DateDelegate: public QStyledItemDelegate{
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QString displayText(const QVariant &value, const QLocale &locale) const{
        return locale.toString(value.toDateTime(), "d MMM - hh:mm");
    }
};

int tablecolumns=9;
relationDialog::relationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::relationDialog)
{
    ui->setupUi(this);
    ui->label->setText("Strategy:"+strat);
    model = new QStandardItemModel(trademodel.length()/(tablecolumns-1),tablecolumns-1,this);
    //model->setRowCount(trademodel.length()/tablecolumns);
    setGeometry(loadsettings("relationdialog").toRect());
    load_model();
    model->setHeaderData(0, Qt::Horizontal, "Date open", Qt::DisplayRole);
    model->setHeaderData(1, Qt::Horizontal, "Date closed", Qt::DisplayRole);
    model->setHeaderData(2, Qt::Horizontal, "Pair", Qt::DisplayRole);
    model->setHeaderData(3, Qt::Horizontal, "Enter tag", Qt::DisplayRole);
    model->setHeaderData(4, Qt::Horizontal, "Exit reason", Qt::DisplayRole);
    model->setHeaderData(5, Qt::Horizontal, "Stake", Qt::DisplayRole);
    model->setHeaderData(6, Qt::Horizontal, "Profit%", Qt::DisplayRole);
    model->setHeaderData(7, Qt::Horizontal, "Profit", Qt::DisplayRole);
    ui->tableView->setModel(model);
    ui->tableView->setSortingEnabled(true);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView->sortByColumn(0,Qt::AscendingOrder);
    ui->tableView->setItemDelegateForColumn(0, new DateDelegate(this));
    ui->tableView->setItemDelegateForColumn(1, new DateDelegate(this));
}

relationDialog::~relationDialog()
{
    savesettings("relationdialog",this->geometry());
    delete ui;
}

void relationDialog::load_model()
{
    int row=0,i=0,col, rows=0;
    double cash_total=0, pr_total=0, avrstake=0;
    QModelIndex index;
    QDateTime tradedate;
    QDate trade=QDate(firsttrade.mid(0,4).toInt(),firsttrade.mid(5,2).toInt(),firsttrade.mid(8,2).toInt());
    while (i < trademodel.length()-1) {
        if (strat == trademodel[i]) rows++;
         i++;
    }
    i=0;
    model->setRowCount(rows+1);
    bool add=false;
    while (i < trademodel.length()-1) {
       for (col=0;col<tablecolumns;col++) {
         index=model->index(row,col,QModelIndex());
         if (col == 0) {
             QDate datetrade=QDate(trademodel[i+1].mid(0,4).toInt(),trademodel[i+1].mid(5,2).toInt(),trademodel[i+1].mid(8,2).toInt());
             //qDebug() << ui->datefrom->date() << " " << datetrade << " " << ui->dateto->date();
             if (strat == trademodel[i] && trade <= datetrade) add=true;
             else if (strat == trademodel[i]) model->removeRow(1);
         }
         if (col==3 && add) {
            pr_total+=trademodel[i+5].toDouble();
            cash_total+=trademodel[i+6].toDouble();
         }
         if (col <= 5 && add) {
             tradedate=QDateTime(QDate(trademodel[i+1].mid(0,4).toInt(),trademodel[i+1].mid(5,2).toInt(),trademodel[i+1].mid(8,2).toInt()),QTime(trademodel[i+1].mid(11,2).toInt(),trademodel[i+1].mid(14,2).toInt(),0));
             if (col == 0) model->setData(index,tradedate);
             if (col == 1) model->setData(index,tradedate);
             if (col == 2 || col == 3 || col == 4 || col == 5) model->setData(index,trademodel[i+1]);
             if (col == 5) avrstake+=trademodel[i+1].toDouble();
         }
         if (col >= 6 && col < 8 && add) model->setData(index,trademodel[i+1].toDouble());
         model->setData(index, Qt::AlignCenter, Qt::TextAlignmentRole);
         i++;

         }

       if (add) row++;
       add=false;
    }
    index=model->index(row,2,QModelIndex());
    model->setData(index,"Total:");
    model->setData(index, Qt::AlignCenter, Qt::TextAlignmentRole);
    index=model->index(row,6,QModelIndex());
    model->setData(index,QLocale(QLocale::English).toString(pr_total,'F',2)+"% of "+QLocale(QLocale::English).toString(avrstake/row,'F',2));
    model->setData(index, Qt::AlignCenter, Qt::TextAlignmentRole);
    index=model->index(row,7,QModelIndex());
    model->setData(index,QString::number(cash_total));
    model->setData(index, Qt::AlignCenter, Qt::TextAlignmentRole);
}

QVariant relationDialog::loadsettings(QString settings)
{
    QVariant returnvar;
    QSettings appsettings("QTinman",appgroup);
    appsettings.beginGroup(appgroup);
    returnvar = appsettings.value(settings);
    appsettings.endGroup();
    return returnvar;
}

void relationDialog::savesettings(QString settings, QVariant attr)
{
    QSettings appsettings("QTinman",appgroup);
    appsettings.beginGroup(appgroup);
    appsettings.setValue(settings,QVariant::fromValue(attr));
    appsettings.endGroup();
}

