#pragma once

#include <QObject>
#include <QHash>
#include <QStringList>
#include <QVariantList>
#include <QPointer>

class AndroidRuntimeAdapter;

class SessionManager;

// Session-level application service.
// Discovers XDG desktop entries and launches applications without a shell.
class ApplicationService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList apps READ apps NOTIFY appsChanged)

public:
    explicit ApplicationService(QObject *parent = nullptr);

    QVariantList apps() const { return m_apps; }

    void setSessionManager(SessionManager *sessionManager);
    void setAndroidRuntimeAdapter(AndroidRuntimeAdapter *adapter);

    Q_INVOKABLE void reload();
    Q_INVOKABLE bool launch(const QString &appId);

signals:
    void appsChanged();
    void launchStarted(const QString &desktopId);
    void launchFailed(const QString &desktopId, const QString &reason);

private:
    struct LaunchEntry {
        QString program;
        QStringList arguments;
        QString name;
    };

    QVariantList m_apps;
    QHash<QString, LaunchEntry> m_launchEntries;
    SessionManager *m_sessionManager = nullptr;
    QPointer<AndroidRuntimeAdapter> m_androidAdapter;
};
