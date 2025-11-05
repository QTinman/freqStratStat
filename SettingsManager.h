#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QSettings>
#include <QVariant>
#include <QString>

/**
 * Singleton utility class for managing application settings.
 * Eliminates code duplication across MainWindow, RelationDialog, and SettingsDialog.
 */
class SettingsManager
{
public:
    // Get singleton instance
    static SettingsManager& instance() {
        static SettingsManager instance;
        return instance;
    }

    // Load a setting value
    QVariant loadSetting(const QString& key, const QString& appGroup = "stratreader") {
        QSettings settings("QTinman", appGroup);
        settings.beginGroup(appGroup);
        QVariant value = settings.value(key);
        settings.endGroup();
        return value;
    }

    // Save a setting value
    void saveSetting(const QString& key, const QVariant& value, const QString& appGroup = "stratreader") {
        QSettings settings("QTinman", appGroup);
        settings.beginGroup(appGroup);
        settings.setValue(key, value);
        settings.endGroup();
    }

    // Prevent copying
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

private:
    SettingsManager() {}
};

#endif // SETTINGSMANAGER_H
