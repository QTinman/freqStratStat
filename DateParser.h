#ifndef DATEPARSER_H
#define DATEPARSER_H

#include <QDateTime>
#include <QString>

/**
 * Utility class for parsing date strings from FreqTrade API.
 * Eliminates repetitive date parsing code using QString::mid() operations.
 */
class DateParser
{
public:
    /**
     * Parse FreqTrade date string format: "YYYY-MM-DD HH:MM:SS"
     * @param dateString The date string to parse
     * @return QDateTime object, or null QDateTime if parsing fails
     */
    static QDateTime parseFreqTradeDate(const QString& dateString) {
        if (dateString.length() < 16) {
            return QDateTime();
        }

        // Use Qt's built-in parser for better performance and maintainability
        return QDateTime::fromString(dateString, "yyyy-MM-dd hh:mm:ss");
    }

    /**
     * Alternative parser using mid() operations for backward compatibility
     * if the fromString approach has issues
     */
    static QDateTime parseFreqTradeDateLegacy(const QString& dateString) {
        if (dateString.length() < 16) {
            return QDateTime();
        }

        int year = dateString.mid(0, 4).toInt();
        int month = dateString.mid(5, 2).toInt();
        int day = dateString.mid(8, 2).toInt();
        int hour = dateString.mid(11, 2).toInt();
        int minute = dateString.mid(14, 2).toInt();

        return QDateTime(QDate(year, month, day), QTime(hour, minute, 0));
    }

    /**
     * Format QDateTime to FreqTrade display format
     */
    static QString formatDisplayDate(const QDateTime& dateTime) {
        return dateTime.toString("d MMM - hh:mm");
    }

    /**
     * Parse and format in one step
     */
    static QString parseAndFormat(const QString& dateString) {
        QDateTime dt = parseFreqTradeDate(dateString);
        return dt.isValid() ? formatDisplayDate(dt) : QString();
    }
};

#endif // DATEPARSER_H
