#pragma once

#include "AndroidRuntimeAdapter.h"

#include <QByteArray>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariantList>

#include <functional>

class QProcess;

// Waydroid-backed Android runtime provider. All CLI operations are launched
// asynchronously with argument lists; no shell command string is constructed.
class WaydroidRuntimeAdapter final : public AndroidRuntimeAdapter
{
    Q_OBJECT
    Q_PROPERTY(QString runtimeState READ runtimeState NOTIFY runtimeStateChanged)
    Q_PROPERTY(bool available READ isAvailable CONSTANT)

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
    Q_INVOKABLE bool requestStartRuntime() { return startRuntime(nullptr); }
    Q_INVOKABLE bool requestStopRuntime() { return stopRuntime(nullptr); }

private:
    using SuccessHandler = std::function<void(const QByteArray &)>;

    bool ensureAvailable(QString *error) const;
    bool runCommand(const QString &operation,
                    const QStringList &arguments,
                    const QString &target,
                    const SuccessHandler &onSuccess = {});
    static QVariantList parseApplicationList(const QByteArray &output);
    void setRuntimeState(const QString &state);

    QString m_executable;
    QString m_runtimeState = QStringLiteral("unavailable");
    QVariantList m_applications;
    QSet<QProcess *> m_processes;
};
