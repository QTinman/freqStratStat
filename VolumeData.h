#ifndef VOLUMEDATA_H
#define VOLUMEDATA_H

#include <QVector>
#include <QMap>
#include <QString>
#include <QDateTime>

/**
 * Structure to hold OHLCV candle data with volume information
 */
struct VolumeCandle {
    qint64 timestamp;    // Unix timestamp in milliseconds
    double open;
    double high;
    double low;
    double close;
    double volume;

    VolumeCandle() : timestamp(0), open(0), high(0), low(0), close(0), volume(0) {}

    VolumeCandle(qint64 ts, double o, double h, double l, double c, double v)
        : timestamp(ts), open(o), high(h), low(l), close(c), volume(v) {}

    bool isValid() const {
        return timestamp > 0 && volume >= 0;
    }
};

/**
 * Container for volume candle data with utility functions
 */
class VolumeDataContainer {
public:
    VolumeDataContainer() {}

    void addCandle(const VolumeCandle& candle) {
        candles.append(candle);
    }

    void addCandle(qint64 timestamp, double open, double high, double low, double close, double volume) {
        candles.append(VolumeCandle(timestamp, open, high, low, close, volume));
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

    const VolumeCandle& at(int index) const {
        return candles.at(index);
    }

    /**
     * Find the candle that contains the given timestamp
     * @param timestamp - Unix timestamp in milliseconds
     * @param timeframeMs - Timeframe in milliseconds (e.g., 300000 for 5m)
     * @return Volume at that timestamp, or 0 if not found
     */
    double getVolumeAtTimestamp(qint64 timestamp, qint64 timeframeMs) const {
        for (const VolumeCandle& candle : candles) {
            // Check if timestamp falls within this candle's timeframe
            if (timestamp >= candle.timestamp &&
                timestamp < candle.timestamp + timeframeMs) {
                return candle.volume;
            }
        }
        return 0.0; // Not found
    }

    /**
     * Calculate average volume over last N candles
     * @param lookback - Number of candles to average (default 100)
     * @return Average volume
     */
    double calculateAverageVolume(int lookback = 100) const {
        if (candles.isEmpty()) return 0.0;

        int count = qMin(lookback, candles.count());
        double total = 0.0;

        // Calculate from most recent candles
        for (int i = candles.count() - count; i < candles.count(); i++) {
            total += candles[i].volume;
        }

        return count > 0 ? total / count : 0.0;
    }

    /**
     * Get volume at specific timestamp and return volume ratio
     * @param timestamp - Unix timestamp in milliseconds
     * @param timeframeMs - Timeframe in milliseconds
     * @param lookback - Lookback period for average calculation
     * @return Volume ratio (volume / average), or 0 if not found
     */
    double getVolumeRatio(qint64 timestamp, qint64 timeframeMs, int lookback = 100) const {
        double volume = getVolumeAtTimestamp(timestamp, timeframeMs);
        if (volume == 0.0) return 0.0;

        double avgVolume = calculateAverageVolume(lookback);
        if (avgVolume == 0.0) return 0.0;

        return volume / avgVolume;
    }

    /**
     * Iterator support for range-based for loops
     */
    QVector<VolumeCandle>::const_iterator begin() const {
        return candles.constBegin();
    }

    QVector<VolumeCandle>::const_iterator end() const {
        return candles.constEnd();
    }

private:
    QVector<VolumeCandle> candles;
};

/**
 * Cache for volume data by pair and timeframe
 * Key format: "PAIR_TIMEFRAME" (e.g., "BTC/USDT_5m")
 */
class VolumeDataCache {
public:
    static VolumeDataCache& instance() {
        static VolumeDataCache instance;
        return instance;
    }

    void addData(const QString& pair, const QString& timeframe, const VolumeDataContainer& data) {
        QString key = makeCacheKey(pair, timeframe);
        cache[key] = data;
        cacheTimestamps[key] = QDateTime::currentMSecsSinceEpoch();
    }

    bool hasData(const QString& pair, const QString& timeframe, qint64 maxAgeMs = 300000) const {
        QString key = makeCacheKey(pair, timeframe);
        if (!cache.contains(key)) return false;

        // Check if cache is still valid (default 5 minutes)
        qint64 age = QDateTime::currentMSecsSinceEpoch() - cacheTimestamps[key];
        return age < maxAgeMs;
    }

    VolumeDataContainer getData(const QString& pair, const QString& timeframe) const {
        QString key = makeCacheKey(pair, timeframe);
        return cache.value(key, VolumeDataContainer());
    }

    void clear() {
        cache.clear();
        cacheTimestamps.clear();
    }

    void clearPair(const QString& pair, const QString& timeframe) {
        QString key = makeCacheKey(pair, timeframe);
        cache.remove(key);
        cacheTimestamps.remove(key);
    }

    // Prevent copying
    VolumeDataCache(const VolumeDataCache&) = delete;
    VolumeDataCache& operator=(const VolumeDataCache&) = delete;

private:
    VolumeDataCache() {}

    QString makeCacheKey(const QString& pair, const QString& timeframe) const {
        return pair + "_" + timeframe;
    }

    QMap<QString, VolumeDataContainer> cache;
    QMap<QString, qint64> cacheTimestamps;
};

#endif // VOLUMEDATA_H
