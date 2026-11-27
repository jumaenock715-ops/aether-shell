#include "SessionManager.h"

#include <QFileInfo>

SessionManager::SessionManager(QObject *parent)
    : QAbstractListModel(parent)
{
}

SessionManager::~SessionManager()
{
    for (const Instance &instance : m_instances) {
        if (instance.process) {
            instance.process->disconnect(this);
            instance.process->kill();
            instance.process->waitForFinished(1000);
        }
    }
}

int SessionManager::count() const
{
    return m_instances.size();
}

int SessionManager::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_instances.size();
}

QVariant SessionManager::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_instances.size())
        return {};

    const Instance &instance = m_instances.at(index.row());
    switch (role) {
    case DesktopIdRole: return instance.desktopId;
    case NameRole: return instance.name;
    case ProcessIdRole: return instance.process ? instance.process->processId() : 0;
    case StateRole: return instance.state;
    default: return {};
    }
}

QHash<int, QByteArray> SessionManager::roleNames() const
{
    return {
        {DesktopIdRole, QByteArrayLiteral("desktopId")},
        {NameRole, QByteArrayLiteral("name")},
        {ProcessIdRole, QByteArrayLiteral("processId")},
        {StateRole, QByteArrayLiteral("state")}
    };
}

int SessionManager::indexOf(QProcess *process) const
{
    for (int i = 0; i < m_instances.size(); ++i) {
        if (m_instances.at(i).process == process)
            return i;
    }
    return -1;
}

bool SessionManager::launch(const QString &desktopId,
                            const QString &name,
                            const QString &program,
                            const QStringList &arguments)
{
    if (desktopId.trimmed().isEmpty() || name.trimmed().isEmpty() ||
        program.trimmed().isEmpty()) {
        emit applicationFailed(desktopId, QStringLiteral("Invalid application launch request."));
        return false;
    }

    const QFileInfo executableInfo(program);
    if (executableInfo.isAbsolute() &&
        (!executableInfo.exists() || !executableInfo.isExecutable())) {
        emit applicationFailed(desktopId, QStringLiteral("Application executable is unavailable."));
        return false;
    }

    auto *process = new QProcess(this);
    process->setProgram(program);
    process->setArguments(arguments);

    const int row = m_instances.size();
    beginInsertRows(QModelIndex(), row, row);
    m_instances.append({process, desktopId, name, QStringLiteral("starting")});
    endInsertRows();
    emit countChanged();

    connect(process, &QProcess::started, this, [this, process, desktopId]() {
        const int row = indexOf(process);
        if (row < 0)
            return;
        m_instances[row].state = QStringLiteral("running");
        emit dataChanged(index(row, 0), index(row, 0), {StateRole, ProcessIdRole});
        emit applicationStarted(desktopId, process->processId());
    });

    connect(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, [this, process, desktopId](int exitCode, QProcess::ExitStatus status) {
        emit applicationExited(desktopId, exitCode, status);
        removeProcess(process);
    });

    connect(process, &QProcess::errorOccurred, this,
            [this, process, desktopId](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart)
            return;
        const QString reason = process->errorString();
        emit applicationFailed(desktopId, reason);
        removeProcess(process);
    });

    process->start();
    return true;
}

void SessionManager::removeProcess(QProcess *process)
{
    const int row = indexOf(process);
    if (row < 0)
        return;

    beginRemoveRows(QModelIndex(), row, row);
    m_instances.removeAt(row);
    endRemoveRows();
    emit countChanged();
    process->deleteLater();
}

void SessionManager::terminate(const QString &desktopId)
{
    for (const Instance &instance : m_instances) {
        if (instance.desktopId == desktopId && instance.process &&
            instance.process->state() != QProcess::NotRunning)
            instance.process->terminate();
    }
}

void SessionManager::terminateAll()
{
    for (const Instance &instance : m_instances) {
        if (instance.process && instance.process->state() != QProcess::NotRunning)
            instance.process->terminate();
    }
}
