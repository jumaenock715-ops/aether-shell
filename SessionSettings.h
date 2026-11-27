#pragma once

#include <QObject>
#include <QColor>

class SessionSettings final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool darkMode READ darkMode NOTIFY darkModeChanged)
    Q_PROPERTY(QColor accentColor READ accentColor NOTIFY accentColorChanged)

public:
    explicit SessionSettings(QObject *parent = nullptr);

    bool darkMode() const;
    QColor accentColor() const;

    Q_INVOKABLE bool setDarkMode(bool enabled);
    Q_INVOKABLE bool setAccentColor(const QString &color);

signals:
    void darkModeChanged();
    void accentColorChanged();

private:
    bool m_darkMode = true;
    QColor m_accentColor = QColor(QStringLiteral("#6D4AFF"));
};
