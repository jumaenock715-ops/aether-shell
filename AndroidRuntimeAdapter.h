#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

// Provider boundary for Android userspace runtimes (Waydroid is the first
// candidate). Implementations own runtime-specific commands and package data.
// The shell and ApplicationService must not execute runtime commands directly.
class AndroidRuntimeAdapter : public QObject
{
    Q_OBJECT

public:
    explicit AndroidRuntimeAdapter(QObject *parent = nullptr)
        : QObject(parent) {}
    ~AndroidRuntimeAdapter() override = default;

    // Each returned map describes one installed app:
    // packageName, name, icon (optional), categories (optional).
    virtual QVariantList applications() const = 0;
    virtual bool isAvailable() const = 0;
    virtual QString runtimeState() const = 0;
    virtual bool startRuntime(QString *error = nullptr) = 0;
    virtual bool stopRuntime(QString *error = nullptr) = 0;

    // Returns whether the request was accepted; errors explain rejection.
    virtual bool launchPackage(const QString &packageName,
                               QString *error = nullptr) = 0;
    virtual bool installApk(const QString &apkPath,
                            QString *error = nullptr) = 0;
    virtual bool removePackage(const QString &packageName,
                               QString *error = nullptr) = 0;

signals:
    void applicationsChanged();
    void runtimeStateChanged();
    void operationFailed(const QString &operation, const QString &reason);
    void operationFailedForTarget(const QString &operation, const QString &target,
                                  const QString &reason);
    void operationFinished(const QString &operation, const QString &target);

    // Optional provider event contract. A runtime/window bridge may emit
    // started, foreground, background, stopped, or crashed. A provider that
    // cannot observe real activity state must not emit guessed transitions.
    void applicationLifecycleChanged(const QString &packageName,
                                     const QString &lifecycleState);
};
