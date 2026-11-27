#pragma once

#include <QObject>
#include <QVariantList>
#include <QString>

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
};
