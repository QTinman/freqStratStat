#ifndef RELATIONDIALOG_H
#define RELATIONDIALOG_H

#include <QDialog>
#include <QMainWindow>
#include <QtCore>
#include <QtGui>

namespace Ui {
class relationDialog;
}

class relationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit relationDialog(QWidget *parent = nullptr);
    void load_model();
    QVariant loadsettings(QString settings);
    void savesettings(QString settings, QVariant attr);
    ~relationDialog();

private slots:


private:
    Ui::relationDialog *ui;
    QStandardItemModel *model;
};

#endif // RELATIONDIALOG_H
