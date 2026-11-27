#include "AndroidSessionManager.h"
#include "AndroidRuntimeAdapter.h"

#include <QDateTime>
#include <QVariantMap>

AndroidSessionManager::AndroidSessionManager(AndroidRuntimeAdapter *adapter,
                                             QObject *parent)
    : QAbstractListModel(parent),
      m_adapter(adapter)
{
    if (!m_adapter)
        return;

    connect(m_adapter, &AndroidRuntimeAdapter::operationFinished,
            this, [this](const QString &operation, const QString &target) {
        if (operation != QStringLiteral("launch") || target.isEmpty())
            return;
        const int row = indexOf(target);
        if (row < 0)
            return;
        updateRecord(row, QStringLiteral("launch-command-completed"),
                     QStringLiteral("Waydroid completed the launch command. "
                                    "The Android activity state is not exposed."));
        emit launchCommandCompleted(target);
    });

    connect(m_adapter, &AndroidRuntimeAdapter::operationFailed,
            this, [this](const QString &operation, const QString &reason) {
        if (operation != QStringLiteral("launch") &&
            operation != QStringLiteral("session-start"))
            return;

        // A session-start failure is associated with a package only when the
        // adapter supplied that package as the operation target. The current
        // adapter's generic operationFailed signal does not include a target,
        // so package-specific launch-start failures are surfaced by the
        // ApplicationService as well; don't guess which record failed here.
        if (operation == QStringLiteral("launch")) {
            // The adapter reports operation and reason but not target on
            // failure. Mark pending launch records as failed rather than
            // inventing a package association.
            for (int i = 0; i < m_records.size(); ++i) {
                if (m_records[i].state == QStringLiteral("launch-requested"))
                    updateRecord(i, QStringLiteral("launch-failed"), reason);
            }
        }
    });

    connect(m_adapter, &AndroidRuntimeAdapter::runtimeStateChanged,
            this, [this]() {
        if (!m_adapter || m_adapter->runtimeState() != QStringLiteral("stopped"))
            return;
        for (int i = 0; i < m_records.size(); ++i) {
            if (m_records[i].state != QStringLiteral("launch-failed"))
                updateRecord(i, QStringLiteral("runtime-stopped"),
                             QStringLiteral("The Android user session was stopped. "
                                            "Per-activity state is not available."));
        }
    });
}

int AndroidSessionManager::count() const
{
    return m_records.size();
}

int AndroidSessionManager::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_records.size();
}

QVariant AndroidSessionManager::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_records.size())
        return {};

    const SessionRecord &record = m_records.at(index.row());
    switch (role) {
    case PackageNameRole: return record.packageName;
    case NameRole: return record.name;
    case StateRole: return record.state;
    case ActivityStateRole: return record.activityState;
    case LastRequestedRole: return record.lastRequested;
    case DetailRole: return record.detail;
    default: return {};
    }
}

QHash<int, QByteArray> AndroidSessionManager::roleNames() const
{
    return {
        {PackageNameRole, QByteArrayLiteral("packageName")},
        {NameRole, QByteArrayLiteral("name")},
        {StateRole, QByteArrayLiteral("state")},
        {ActivityStateRole, QByteArrayLiteral("activityState")},
        {LastRequestedRole, QByteArrayLiteral("lastRequested")},
        {DetailRole, QByteArrayLiteral("detail")}
    };
}

int AndroidSessionManager::indexOf(const QString &packageName) const
{
    for (int i = 0; i < m_records.size(); ++i) {
        if (m_records.at(i).packageName == packageName)
            return i;
    }
    return -1;
}

void AndroidSessionManager::updateRecord(int row, const QString &state,
                                         const QString &detail)
{
    if (row < 0 || row >= m_records.size())
        return;
    m_records[row].state = state;
    m_records[row].detail = detail;
    emit dataChanged(index(row, 0), index(row, 0),
                     {StateRole, ActivityStateRole, DetailRole});
}

bool AndroidSessionManager::launch(const QString &packageName, const QString &name)
{
    const QString package = packageName.trimmed();
    const QString displayName = name.trimmed();
    if (package.isEmpty() || displayName.isEmpty() || !m_adapter ||
        !m_adapter->isAvailable()) {
        emit launchFailed(package,
                          QStringLiteral("The Android runtime or application is unavailable."));
        return false;
    }

    int row = indexOf(package);
    if (row < 0) {
        row = m_records.size();
        beginInsertRows(QModelIndex(), row, row);
        m_records.append({package, displayName, QStringLiteral("launch-requested"),
                          QStringLiteral("unknown"),
                          QDateTime::currentDateTime().toString(Qt::ISODate),
                          QStringLiteral("Launch request queued; activity state is not yet known.")});
        endInsertRows();
        emit countChanged();
    } else {
        m_records[row].name = displayName;
        m_records[row].lastRequested = QDateTime::currentDateTime().toString(Qt::ISODate);
        updateRecord(row, QStringLiteral("launch-requested"),
                     QStringLiteral("Launch request queued; activity state is not yet known."));
        emit dataChanged(index(row, 0), index(row, 0), {NameRole, LastRequestedRole});
    }

    emit launchRequested(package);
    QString error;
    if (!m_adapter->launchPackage(package, &error)) {
        const QString reason = error.isEmpty()
            ? QStringLiteral("The Android launch request was rejected.") : error;
        updateRecord(row, QStringLiteral("launch-failed"), reason);
        emit launchFailed(package, reason);
        return false;
    }
    return true;
}

void AndroidSessionManager::clearHistory()
{
    if (m_records.isEmpty())
        return;
    beginResetModel();
    m_records.clear();
    endResetModel();
    emit countChanged();
}
