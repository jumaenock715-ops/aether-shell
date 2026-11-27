#include "SessionSettings.h"

#include <QSettings>

namespace {
constexpr int SettingsSchemaVersion = 1;
const QColor DefaultAccent(QStringLiteral("#6D4AFF"));
}

SessionSettings::SessionSettings(QObject *parent)
    : QObject(parent)
{
    QSettings settings;
    const int schemaVersion = settings.value(QStringLiteral("session/schemaVersion"), 0).toInt();

    // Version 1 introduces persistent appearance preferences.
    if (schemaVersion < 1) {
        settings.setValue(QStringLiteral("appearance/darkMode"), true);
        settings.setValue(QStringLiteral("appearance/accentColor"), DefaultAccent.name(QColor::HexRgb));
        settings.setValue(QStringLiteral("session/schemaVersion"), SettingsSchemaVersion);
        settings.sync();
    }

    m_darkMode = settings.value(QStringLiteral("appearance/darkMode"), true).toBool();

    const QColor storedAccent(settings.value(
        QStringLiteral("appearance/accentColor"),
        DefaultAccent.name(QColor::HexRgb)).toString());

    m_accentColor = storedAccent.isValid() ? storedAccent : DefaultAccent;
}

bool SessionSettings::darkMode() const
{
    return m_darkMode;
}

QColor SessionSettings::accentColor() const
{
    return m_accentColor;
}

bool SessionSettings::setDarkMode(bool enabled)
{
    if (m_darkMode == enabled)
        return true;

    QSettings settings;
    settings.setValue(QStringLiteral("appearance/darkMode"), enabled);
    settings.sync();

    if (settings.status() != QSettings::NoError)
        return false;

    m_darkMode = enabled;
    emit darkModeChanged();
    return true;
}

bool SessionSettings::setAccentColor(const QString &color)
{
    const QColor candidate(color);
    if (!candidate.isValid())
        return false;

    if (m_accentColor == candidate)
        return true;

    QSettings settings;
    settings.setValue(QStringLiteral("appearance/accentColor"), candidate.name(QColor::HexRgb));
    settings.sync();

    if (settings.status() != QSettings::NoError)
        return false;

    m_accentColor = candidate;
    emit accentColorChanged();
    return true;
}
