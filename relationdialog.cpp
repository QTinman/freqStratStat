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

int tablecolumns=8;
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
    model->setHeaderData(3, Qt::Horizontal, "Sell reason", Qt::DisplayRole);
    model->setHeaderData(4, Qt::Horizontal, "Stake", Qt::DisplayRole);
    model->setHeaderData(5, Qt::Horizontal, "Profit%", Qt::DisplayRole);
    model->setHeaderData(6, Qt::Horizontal, "Profit", Qt::DisplayRole);
    ui->tableView->setModel(model);
    ui->tableView->setSortingEnabled(true);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView->sortByColumn(0,Qt::AscendingOrder);
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
         if (col == 0) if (strat == trademodel[i]) add=true;
         if (col==2 && add) {
            pr_total+=trademodel[i+4].toDouble();
            cash_total+=trademodel[i+5].toDouble();
         }
         if (col <= 4 && add) {
             tradedate=QDateTime(QDate(trademodel[i+1].mid(0,4).toInt(),trademodel[i+1].mid(5,2).toInt(),trademodel[i+1].mid(8,2).toInt()),QTime(trademodel[i+1].mid(11,2).toInt(),trademodel[i+1].mid(14,2).toInt(),0));
             if (col == 0) model->setData(index,tradedate);
             if (col == 1) model->setData(index,tradedate);
             if (col == 2 || col == 3 || col == 4) model->setData(index,trademodel[i+1]);
             if (col == 4) avrstake+=trademodel[i+1].toDouble();
             ui->tableView->setItemDelegateForColumn(0,  new DateDelegate);
             ui->tableView->setItemDelegateForColumn(1,  new DateDelegate);
         }
         if (col >= 5 && col < 7 && add) model->setData(index,trademodel[i+1].toDouble());
         model->setData(index, Qt::AlignCenter, Qt::TextAlignmentRole);
         i++;

         }

       if (add) row++;
       add=false;
    }
    index=model->index(rows,2,QModelIndex());
    model->setData(index,"Total:");
    model->setData(index, Qt::AlignCenter, Qt::TextAlignmentRole);
    index=model->index(rows,5,QModelIndex());
    model->setData(index,QLocale(QLocale::English).toString(pr_total,'F',2)+"% of "+QLocale(QLocale::English).toString(avrstake/row,'F',2));
    model->setData(index, Qt::AlignCenter, Qt::TextAlignmentRole);
    index=model->index(rows,6,QModelIndex());
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
