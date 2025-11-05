#include "relationdialog.h"
#include "ui_relationdialog.h"
#include "mainwindow.h"
#include "Delegates.h"
#include "DateParser.h"
#include "SettingsManager.h"
#include <QPrinter>
#include <QPainter>
#include <QFileDialog>
#include <QMessageBox>
#include <QPageSize>
#include <QPageLayout>

int tablecolumns=9;
relationDialog::relationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::relationDialog)
{
    ui->setupUi(this);
    ui->label->setText("Strategy:"+strat);
    model = new QStandardItemModel(trademodel.length()/(tablecolumns-1),tablecolumns-1,this);
    //model->setRowCount(trademodel.length()/tablecolumns);
    setGeometry(SettingsManager::instance().loadSetting("relationdialog").toRect());
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
    SettingsManager::instance().saveSetting("relationdialog", this->geometry());
    delete ui;
}

void relationDialog::load_model()
{
    int row=0,i=0,col, rows=0;
    double cash_total=0, pr_total=0, avrstake=0;
    QModelIndex index;
    QDateTime tradedate;
    QDate trade = DateParser::parseFreqTradeDate(firsttrade).date();
    while (i < trademodel.length()-1) {
        if (strat == trademodel[i]) rows++;
         i++;
    }
    i=0;
    model->setRowCount(rows+1);
    bool add=false;
    while (i < trademodel.length()-1) {
       for (col=0;col<tablecolumns;col++) {
         // Only access model columns that exist (0 to tablecolumns-2, i.e., 0-7 for 9 fields)
         if (col < tablecolumns - 1) {
             index=model->index(row,col,QModelIndex());
         }

         if (col == 0) {
             QDate datetrade = DateParser::parseFreqTradeDate(trademodel[i+1]).date();
             //qDebug() << ui->datefrom->date() << " " << datetrade << " " << ui->dateto->date();
             if (strat == trademodel[i] && trade <= datetrade) add=true;
             else if (strat == trademodel[i]) model->removeRow(1);
         }
         if (col==3 && add) {
            // When col=3, i points to pair (index 3), so i+4=profit_pct(7), i+5=profit_abs(8)
            pr_total+=trademodel[i+4].toDouble();
            cash_total+=trademodel[i+5].toDouble();
         }
         if (col <= 5 && add) {
             tradedate = DateParser::parseFreqTradeDate(trademodel[i+1]);
             if (col == 0) model->setData(index,tradedate);
             if (col == 1) model->setData(index,tradedate);
             if (col == 2 || col == 3 || col == 4 || col == 5) model->setData(index,trademodel[i+1]);
             if (col == 5) avrstake+=trademodel[i+1].toDouble();
         }
         if (col >= 6 && col < 8 && add) model->setData(index,trademodel[i+1].toDouble());

         // Only set alignment if within model column bounds
         if (col < tablecolumns - 1) {
             model->setData(index, Qt::AlignCenter, Qt::TextAlignmentRole);
         }
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

void relationDialog::on_savePdfButton_clicked()
{
    // Open file dialog to choose save location
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Trades to PDF"),
        QDir::homePath() + "/trades_" + strat + "_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".pdf",
        tr("PDF Files (*.pdf)"));

    if (fileName.isEmpty()) {
        return; // User cancelled
    }

    // Create printer object for PDF
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageOrientation(QPageLayout::Landscape);

    // Create painter
    QPainter painter;
    if (!painter.begin(&printer)) {
        QMessageBox::warning(this, tr("Export Error"),
                           tr("Failed to create PDF file."));
        return;
    }

    // Set up fonts
    QFont titleFont("Arial", 16, QFont::Bold);
    QFont headerFont("Arial", 10, QFont::Bold);
    QFont dataFont("Arial", 9);

    // Calculate page dimensions
    int pageWidth = printer.pageRect(QPrinter::DevicePixel).width();
    int pageHeight = printer.pageRect(QPrinter::DevicePixel).height();
    int margin = 50;
    int yPos = margin;

    // Draw title
    painter.setFont(titleFont);
    QString title = "Trade Details - Strategy: " + strat;
    painter.drawText(margin, yPos, title);
    yPos += 60;

    // Draw date range
    painter.setFont(dataFont);
    painter.drawText(margin, yPos, "Generated: " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    yPos += 40;

    // Get column count and row count
    int colCount = model->columnCount();
    int rowCount = model->rowCount();

    // Calculate column widths (distribute available space)
    int availableWidth = pageWidth - 2 * margin;
    QVector<int> columnWidths;

    // Define relative widths for each column
    QVector<double> relativeWidths = {1.0, 1.0, 0.8, 0.8, 0.8, 0.7, 0.8, 0.8}; // Adjust based on content
    double totalRelative = 0;
    for (double w : relativeWidths) totalRelative += w;

    for (int i = 0; i < colCount; i++) {
        columnWidths.append(static_cast<int>(availableWidth * relativeWidths[i] / totalRelative));
    }

    // Draw table header
    painter.setFont(headerFont);
    int xPos = margin;
    yPos += 10;

    // Draw header background
    painter.fillRect(margin, yPos - 5, availableWidth, 30, QColor(200, 200, 200));

    for (int col = 0; col < colCount; col++) {
        QString headerText = model->headerData(col, Qt::Horizontal).toString();
        QRect headerRect(xPos, yPos, columnWidths[col], 25);
        painter.drawText(headerRect, Qt::AlignCenter | Qt::TextWordWrap, headerText);
        xPos += columnWidths[col];
    }
    yPos += 35;

    // Draw table data
    painter.setFont(dataFont);
    int rowHeight = 25;

    for (int row = 0; row < rowCount; row++) {
        // Check if we need a new page
        if (yPos + rowHeight > pageHeight - margin) {
            printer.newPage();
            yPos = margin;

            // Redraw header on new page
            painter.setFont(headerFont);
            painter.fillRect(margin, yPos - 5, availableWidth, 30, QColor(200, 200, 200));
            xPos = margin;
            for (int col = 0; col < colCount; col++) {
                QString headerText = model->headerData(col, Qt::Horizontal).toString();
                QRect headerRect(xPos, yPos, columnWidths[col], 25);
                painter.drawText(headerRect, Qt::AlignCenter | Qt::TextWordWrap, headerText);
                xPos += columnWidths[col];
            }
            yPos += 35;
            painter.setFont(dataFont);
        }

        // Draw row background (alternate colors)
        if (row % 2 == 0) {
            painter.fillRect(margin, yPos - 5, availableWidth, rowHeight, QColor(245, 245, 245));
        }

        // Check if this is the totals row (last row)
        bool isTotalRow = (row == rowCount - 1);
        if (isTotalRow) {
            painter.setFont(headerFont);
        }

        xPos = margin;
        for (int col = 0; col < colCount; col++) {
            QModelIndex index = model->index(row, col);
            QString cellText = model->data(index, Qt::DisplayRole).toString();

            QRect cellRect(xPos, yPos, columnWidths[col] - 5, rowHeight);
            painter.drawText(cellRect, Qt::AlignCenter | Qt::TextWordWrap, cellText);
            xPos += columnWidths[col];
        }

        if (isTotalRow) {
            painter.setFont(dataFont);
        }

        yPos += rowHeight;
    }

    // Draw footer
    yPos = pageHeight - margin + 20;
    painter.setFont(dataFont);
    QString footer = "Generated by freqStratStat - Page 1";
    painter.drawText(margin, yPos, footer);

    painter.end();

    // Show success message
    QMessageBox::information(this, tr("Export Successful"),
                           tr("Trades exported successfully to:\n%1").arg(fileName));
}
