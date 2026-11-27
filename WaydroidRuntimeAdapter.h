#pragma once

#include "AndroidRuntimeAdapter.h"

#include <QByteArray>
#include <QSet>
#include <QHash>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QVariantList>

#include <functional>

class QProcess;
class WaylandWindowTracker;

// Waydroid-backed Android runtime provider. All CLI operations are launched
// asynchronously with argument lists; no shell command string is constructed.
class WaydroidRuntimeAdapter final : public AndroidRuntimeAdapter
{
    Q_OBJECT
    Q_PROPERTY(QString runtimeState READ runtimeState NOTIFY runtimeStateChanged)
    Q_PROPERTY(bool available READ isAvailable CONSTANT)

signals:
    void unassociatedWindowChanged(const QString &appId, const QString &windowId,
                                   const QString &title);

public:
    explicit WaydroidRuntimeAdapter(QObject *parent = nullptr);
    ~WaydroidRuntimeAdapter() override = default;

    QVariantList applications() const override { return m_applications; }
    bool isAvailable() const override { return !m_executable.isEmpty(); }
    QString runtimeState() const override { return m_runtimeState; }

    bool startRuntime(QString *error = nullptr) override;
    bool stopRuntime(QString *error = nullptr) override;
    bool launchPackage(const QString &packageName,
                       QString *error = nullptr) override;
    bool installApk(const QString &apkPath,
                    QString *error = nullptr) override;
    bool removePackage(const QString &packageName,
                       QString *error = nullptr) override;

    Q_INVOKABLE void refreshApplications();

    // Register only when a trusted runtime integration has established the
    // app_id -> package relationship. The package must be installed.
    bool registerWindowPackageMapping(const QString &windowId,
                                      const QString &packageName,
                                    QString *error = nullptr);
    void clearWindowPackageMapping(const QString &windowId);
    void setWindowTracker(WaylandWindowTracker *tracker);
    Q_INVOKABLE bool requestStartRuntime() { return startRuntime(nullptr); }
    Q_INVOKABLE bool requestStopRuntime() { return stopRuntime(nullptr); }

private:
    using SuccessHandler = std::function<void(const QByteArray &)>;
    struct ObservedWindow {
        QString appId;
        QString title;
    };

    bool ensureAvailable(QString *error) const;
    bool runCommand(const QString &operation,
                    const QStringList &arguments,
                    const QString &target,
                    const SuccessHandler &onSuccess = {});
    static QVariantList parseApplicationList(const QByteArray &output);
    void setRuntimeState(const QString &state);
    bool isInstalledPackage(const QString &packageName) const;
    QString mappedPackageForWindow(const QString &windowId) const;
    void handleWindowChanged(const QString &appId, const QString &windowId,
                             const QString &title, const QString &eventState);
    void associateObservedWindow(const QString &windowId,
                                 const ObservedWindow &window);

    QString m_executable;
    QString m_runtimeState = QStringLiteral("unavailable");
    QVariantList m_applications;
    QSet<QProcess *> m_processes;
    QPointer<WaylandWindowTracker> m_windowTracker;
    QHash<QString, QString> m_windowPackageMappings;
    QHash<QString, ObservedWindow> m_observedWindows;
    QHash<QString, QString> m_windowOwners;
};
