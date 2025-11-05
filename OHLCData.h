#ifndef OHLCDATA_H
#define OHLCDATA_H

#include <QVector>
#include <QString>

/**
 * OHLC (Open, High, Low, Close) candlestick data structure.
 * Replaces the inefficient flat QStringList storage for market data.
 */
struct OHLCCandle {
    qint64 timestamp;    // Unix timestamp in milliseconds
    double open;
    double high;
    double low;
    double close;

    OHLCCandle() : timestamp(0), open(0), high(0), low(0), close(0) {}

    OHLCCandle(qint64 ts, double o, double h, double l, double c)
        : timestamp(ts), open(o), high(h), low(l), close(c) {}

    OHLCCandle(const QString& tsStr, const QString& oStr, const QString& hStr,
               const QString& lStr, const QString& cStr)
        : timestamp(tsStr.toLongLong()),
          open(oStr.toDouble()),
          high(hStr.toDouble()),
          low(lStr.toDouble()),
          close(cStr.toDouble()) {}

    bool isValid() const {
        return timestamp > 0;
    }
};

/**
 * Container for OHLC market data with utility functions.
 */
class OHLCDataContainer {
public:
    OHLCDataContainer() {}

    void addCandle(const OHLCCandle& candle) {
        candles.append(candle);
    }

    void addCandle(qint64 timestamp, double open, double high, double low, double close) {
        candles.append(OHLCCandle(timestamp, open, high, low, close));
    }

    void clear() {
        candles.clear();
    }

    int count() const {
        return candles.count();
    }

    bool isEmpty() const {
        return candles.isEmpty();
    }

    const OHLCCandle& at(int index) const {
        return candles.at(index);
    }

    /**
     * Find candle that contains the given timestamp.
     * Returns index or -1 if not found.
     */
    int findCandleIndex(qint64 timestamp, qint64 candleDuration) const {
        for (int i = 0; i < candles.count(); ++i) {
            if (timestamp >= candles[i].timestamp - candleDuration &&
                timestamp <= candles[i].timestamp + candleDuration) {
                return i;
            }
        }
        return -1;
    }

    /**
     * Calculate price change between two timestamps.
     * Returns percentage change, or 0 if data not available.
     */
    double calculatePriceChange(qint64 startTimestamp, qint64 endTimestamp, qint64 candleDuration) const {
        int startIdx = findCandleIndex(startTimestamp, candleDuration);
        int endIdx = findCandleIndex(endTimestamp, candleDuration);

        if (startIdx == -1 || endIdx == -1) {
            return 0.0;
        }

        double startPrice = candles[startIdx].close;
        double endPrice = candles[endIdx].close;

        if (startPrice == 0) {
            return 0.0;
        }

        return ((endPrice - startPrice) / startPrice) * 100.0;
    }

    /**
     * Iterator support for range-based for loops
     */
    QVector<OHLCCandle>::const_iterator begin() const {
        return candles.constBegin();
    }

    QVector<OHLCCandle>::const_iterator end() const {
        return candles.constEnd();
    }

private:
    QVector<OHLCCandle> candles;
};

#endif // OHLCDATA_H
