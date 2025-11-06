#include "relationdialog.h"
#include "ui_relationdialog.h"
#include "mainwindow.h"
#include "Delegates.h"
#include "DateParser.h"
#include "SettingsManager.h"
#include <QPrinter>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextTable>
#include <QTextTableFormat>
#include <QTextCharFormat>
#include <QTextBlockFormat>
#include <QAbstractTextDocumentLayout>

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
    printer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);

    // Create QTextDocument for better font handling
    QTextDocument document;
    QTextCursor cursor(&document);

    // Set up character formats
    QTextCharFormat titleFormat;
    titleFormat.setFontPointSize(18);
    titleFormat.setFontWeight(QFont::Bold);

    QTextCharFormat normalFormat;
    normalFormat.setFontPointSize(10);

    QTextCharFormat headerFormat;
    headerFormat.setFontPointSize(10);
    headerFormat.setFontWeight(QFont::Bold);
    headerFormat.setBackground(QBrush(QColor(220, 220, 220)));

    QTextCharFormat totalFormat;
    totalFormat.setFontPointSize(10);
    totalFormat.setFontWeight(QFont::Bold);

    // Add title
    QTextBlockFormat centerFormat;
    centerFormat.setAlignment(Qt::AlignCenter);
    cursor.setBlockFormat(centerFormat);
    cursor.insertText("Trade Details - Strategy: " + strat, titleFormat);
    cursor.insertBlock();

    // Add generation timestamp
    QTextBlockFormat leftFormat;
    leftFormat.setAlignment(Qt::AlignLeft);
    cursor.setBlockFormat(leftFormat);
    cursor.insertText("Generated: " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"), normalFormat);
    cursor.insertBlock();
    cursor.insertBlock(); // Empty line

    // Create table
    int colCount = model->columnCount();
    int rowCount = model->rowCount();

    QTextTableFormat tableFormat;
    tableFormat.setCellPadding(4);
    tableFormat.setCellSpacing(0);
    tableFormat.setBorder(1);
    tableFormat.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
    tableFormat.setWidth(QTextLength(QTextLength::PercentageLength, 100));
    tableFormat.setAlignment(Qt::AlignCenter);

    // Set column width constraints (as percentages)
    QVector<QTextLength> columnWidths;
    columnWidths << QTextLength(QTextLength::PercentageLength, 14)  // Date open
                 << QTextLength(QTextLength::PercentageLength, 14)  // Date closed
                 << QTextLength(QTextLength::PercentageLength, 11)  // Pair
                 << QTextLength(QTextLength::PercentageLength, 11)  // Enter tag
                 << QTextLength(QTextLength::PercentageLength, 13)  // Exit reason
                 << QTextLength(QTextLength::PercentageLength, 11)  // Stake
                 << QTextLength(QTextLength::PercentageLength, 13)  // Profit%
                 << QTextLength(QTextLength::PercentageLength, 13); // Profit
    tableFormat.setColumnWidthConstraints(columnWidths);

    QTextTable *table = cursor.insertTable(rowCount + 1, colCount, tableFormat); // +1 for header

    // Insert headers
    for (int col = 0; col < colCount; col++) {
        QTextTableCell cell = table->cellAt(0, col);
        QTextCursor cellCursor = cell.firstCursorPosition();
        cellCursor.setBlockFormat(centerFormat);
        QString headerText = model->headerData(col, Qt::Horizontal).toString();
        cellCursor.insertText(headerText, headerFormat);
    }

    // Insert data rows
    for (int row = 0; row < rowCount; row++) {
        bool isTotalRow = (row == rowCount - 1);
        QTextCharFormat cellFormat = isTotalRow ? totalFormat : normalFormat;

        for (int col = 0; col < colCount; col++) {
            QTextTableCell cell = table->cellAt(row + 1, col); // +1 because of header row
            QTextCursor cellCursor = cell.firstCursorPosition();
            cellCursor.setBlockFormat(centerFormat);

            QModelIndex index = model->index(row, col);
            QString cellText = model->data(index, Qt::DisplayRole).toString();

            cellCursor.insertText(cellText, cellFormat);

            // Add subtle background for alternating rows (except header and total)
            if (!isTotalRow && row % 2 == 0) {
                QTextTableCellFormat cellBgFormat;
                cellBgFormat.setBackground(QBrush(QColor(248, 248, 248)));
                cell.setFormat(cellBgFormat);
            }
        }
    }

    // Print the document to PDF
    document.print(&printer);

    // Show success message
    QMessageBox::information(this, tr("Export Successful"),
                           tr("Trades exported successfully to:\n%1").arg(fileName));
}
