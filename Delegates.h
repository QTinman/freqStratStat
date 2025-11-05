#ifndef DELEGATES_H
#define DELEGATES_H

#include <QStyledItemDelegate>
#include <QLocale>
#include <QDateTime>

/**
 * Custom delegate for displaying QDateTime values in table views.
 * Formats dates as "d MMM - hh:mm" (e.g., "5 Nov - 14:30")
 *
 * This class was previously duplicated in mainwindow.cpp and relationdialog.cpp.
 */
class DateDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QString displayText(const QVariant &value, const QLocale &locale) const override {
        if (value.type() == QVariant::DateTime) {
            return locale.toString(value.toDateTime(), "d MMM - hh:mm");
        }
        return QStyledItemDelegate::displayText(value, locale);
    }
};

#endif // DELEGATES_H
