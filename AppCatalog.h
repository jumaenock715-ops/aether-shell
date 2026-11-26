#pragma once
#include <QObject>
#include <QVariantList>
#include <QQuickImageProvider>

// Discovers installed applications from XDG .desktop files and launches them.
class AppCatalog : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList apps READ apps NOTIFY appsChanged)

public:
    explicit AppCatalog(QObject *parent = nullptr);

    QVariantList apps() const { return m_apps; }

    Q_INVOKABLE void reload();
    Q_INVOKABLE bool launch(const QString &desktopId);

signals:
    void appsChanged();

private:
    struct Entry { QString id, name, icon, exec; };
    QVariantList m_apps;
    QHash<QString, QString> m_exec;   // desktop id -> cleaned Exec line
};

// Serves icons by theme name or absolute path: image://aethericon/<name>
class IconProvider : public QQuickImageProvider
{
public:
    IconProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;
};
