#include "WaydroidRuntimeAdapter.h"

#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QVariantMap>

namespace {

bool validPackageName(const QString &packageName)
{
    static const QRegularExpression pattern(
        QStringLiteral("^[A-Za-z_][A-Za-z0-9_]*(\\.[A-Za-z_][A-Za-z0-9_]*)+$"));
    return pattern.match(packageName).hasMatch();
}

QString processErrorText(QProcess *process)
{
    QString message = QString::fromLocal8Bit(process->readAllStandardError()).trimmed();
    if (message.isEmpty())
        message = QString::fromLocal8Bit(process->readAllStandardOutput()).trimmed();
    if (message.isEmpty())
        message = process->errorString();
    return message;
}

} // namespace

WaydroidRuntimeAdapter::WaydroidRuntimeAdapter(QObject *parent)
    : AndroidRuntimeAdapter(parent),
      m_executable(QStandardPaths::findExecutable(QStringLiteral("waydroid")))
{
    if (m_executable.isEmpty())
        setRuntimeState(QStringLiteral("unavailable"));
    else {
        setRuntimeState(QStringLiteral("available"));
        refreshApplications();
    }
}

bool WaydroidRuntimeAdapter::ensureAvailable(QString *error) const
{
    if (isAvailable())
        return true;

    if (error)
        *error = QStringLiteral("Waydroid was not found in the executable search path.");
    return false;
}

void WaydroidRuntimeAdapter::setRuntimeState(const QString &state)
{
    if (m_runtimeState == state)
        return;
    m_runtimeState = state;
    emit runtimeStateChanged();
}

bool WaydroidRuntimeAdapter::runCommand(
    const QString &operation,
    const QStringList &arguments,
    const QString &target,
    const SuccessHandler &onSuccess)
{
    QString error;
    if (!ensureAvailable(&error)) {
        emit operationFailed(operation, error);
        emit operationFailedForTarget(operation, target, error);
        return false;
    }

    auto *process = new QProcess(this);
    m_processes.insert(process);

    connect(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, [this, process, operation, target, onSuccess]
            (int exitCode, QProcess::ExitStatus exitStatus) {
        m_processes.remove(process);
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            const QByteArray output = process->readAllStandardOutput();
            if (onSuccess)
                onSuccess(output);
            emit operationFinished(operation, target);
        } else {
            if (operation == QStringLiteral("session-start") ||
                operation == QStringLiteral("session-stop"))
                setRuntimeState(QStringLiteral("error"));
            const QString reason = processErrorText(process);
            emit operationFailed(operation, reason);
            emit operationFailedForTarget(operation, target, reason);
        }
        process->deleteLater();
    });

    connect(process, &QProcess::errorOccurred, this,
            [this, process, operation, target](QProcess::ProcessError processError) {
        if (processError != QProcess::FailedToStart)
            return;
        m_processes.remove(process);
        if (operation == QStringLiteral("session-start") ||
            operation == QStringLiteral("session-stop"))
            setRuntimeState(QStringLiteral("error"));
        emit operationFailed(operation, process->errorString());
        emit operationFailedForTarget(operation, target, process->errorString());
        process->deleteLater();
    });

    process->start(m_executable, arguments);
    return true;
}

QVariantList WaydroidRuntimeAdapter::parseApplicationList(const QByteArray &output)
{
    QVariantList result;
    QVariantMap current;
    bool readingCategories = false;

    const auto appendCurrent = [&]() {
        if (!current.value(QStringLiteral("packageName")).toString().isEmpty() &&
            !current.value(QStringLiteral("name")).toString().isEmpty())
            result.append(current);
    };

    const QString text = QString::fromUtf8(output);
    const QStringList lines = text.split(QLatin1Char('\n'));
    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.startsWith(QStringLiteral("Name:"))) {
            appendCurrent();
            current = {};
            current.insert(QStringLiteral("name"),
                           line.mid(QStringLiteral("Name:").size()).trimmed());
            current.insert(QStringLiteral("categories"), QStringList{});
            readingCategories = false;
        } else if (line.startsWith(QStringLiteral("packageName:"))) {
            current.insert(QStringLiteral("packageName"),
                           line.mid(QStringLiteral("packageName:").size()).trimmed());
            readingCategories = false;
        } else if (line == QStringLiteral("categories:")) {
            readingCategories = true;
        } else if (readingCategories && !line.isEmpty()) {
            QStringList categories =
                current.value(QStringLiteral("categories")).toStringList();
            categories.append(line);
            current.insert(QStringLiteral("categories"), categories);
        }
    }
    appendCurrent();
    return result;
}

void WaydroidRuntimeAdapter::refreshApplications()
{
    if (!isAvailable())
        return;

    runCommand(QStringLiteral("list"),
               {QStringLiteral("app"), QStringLiteral("list")},
               QString(),
               [this](const QByteArray &output) {
        const QVariantList next = parseApplicationList(output);
        if (m_applications == next)
            return;
        m_applications = next;
        emit applicationsChanged();
    });
}

bool WaydroidRuntimeAdapter::startRuntime(QString *error)
{
    if (!ensureAvailable(error))
        return false;

    setRuntimeState(QStringLiteral("starting"));
    const bool accepted = runCommand(
        QStringLiteral("session-start"),
        {QStringLiteral("session"), QStringLiteral("start")},
        QString(),
        [this](const QByteArray &) {
            setRuntimeState(QStringLiteral("running"));
            refreshApplications();
        });
    if (!accepted)
        setRuntimeState(QStringLiteral("available"));
    return accepted;
}

bool WaydroidRuntimeAdapter::stopRuntime(QString *error)
{
    if (!ensureAvailable(error))
        return false;

    setRuntimeState(QStringLiteral("stopping"));
    const bool accepted = runCommand(
        QStringLiteral("session-stop"),
        {QStringLiteral("session"), QStringLiteral("stop")},
        QString(),
        [this](const QByteArray &) {
            setRuntimeState(QStringLiteral("stopped"));
        });
    if (!accepted)
        setRuntimeState(QStringLiteral("available"));
    return accepted;
}

bool WaydroidRuntimeAdapter::launchPackage(const QString &packageName,
                                            QString *error)
{
    if (!validPackageName(packageName)) {
        if (error)
            *error = QStringLiteral("The Android package identifier is invalid.");
        return false;
    }
    if (!ensureAvailable(error))
        return false;

    // Do not issue overlapping session-start/session-stop commands. A launch
    // can be retried once the in-flight runtime transition has completed.
    if (m_runtimeState == QStringLiteral("starting") ||
        m_runtimeState == QStringLiteral("stopping")) {
        if (error)
            *error = QStringLiteral("The Android session is changing state. Retry when it is ready.");
        return false;
    }

    // Starting an Android app from the launcher should not require the user
    // to open Settings first. Bring up the user session, then dispatch the app.
    if (m_runtimeState != QStringLiteral("running")) {
        setRuntimeState(QStringLiteral("starting"));
        return runCommand(
            QStringLiteral("session-start"),
            {QStringLiteral("session"), QStringLiteral("start")},
            packageName,
            [this, packageName](const QByteArray &) {
                setRuntimeState(QStringLiteral("running"));
                refreshApplications();
                runCommand(QStringLiteral("launch"),
                           {QStringLiteral("app"), QStringLiteral("launch"), packageName},
                           packageName);
            });
    }

    return runCommand(QStringLiteral("launch"),
                      {QStringLiteral("app"), QStringLiteral("launch"), packageName},
                      packageName);
}

bool WaydroidRuntimeAdapter::installApk(const QString &apkPath, QString *error)
{
    const QFileInfo fileInfo(apkPath);
    const QString canonicalPath = fileInfo.canonicalFilePath();
    if (canonicalPath.isEmpty() || !fileInfo.isFile() || !fileInfo.isReadable() ||
        fileInfo.suffix().compare(QStringLiteral("apk"), Qt::CaseInsensitive) != 0) {
        if (error)
            *error = QStringLiteral("Select an existing, readable APK file.");
        return false;
    }
    if (!ensureAvailable(error))
        return false;

    return runCommand(QStringLiteral("install"),
                      {QStringLiteral("app"), QStringLiteral("install"), canonicalPath},
                      canonicalPath,
                      [this](const QByteArray &) { refreshApplications(); });
}

bool WaydroidRuntimeAdapter::removePackage(const QString &packageName,
                                            QString *error)
{
    if (!validPackageName(packageName)) {
        if (error)
            *error = QStringLiteral("The Android package identifier is invalid.");
        return false;
    }
    if (!ensureAvailable(error))
        return false;

    return runCommand(QStringLiteral("remove"),
                      {QStringLiteral("app"), QStringLiteral("remove"), packageName},
                      packageName,
                      [this](const QByteArray &) { refreshApplications(); });
}
