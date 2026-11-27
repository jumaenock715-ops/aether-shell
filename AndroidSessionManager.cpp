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
                                    "This does not confirm Android activity state."));
        emit launchCommandCompleted(target);
    });

    connect(m_adapter, &AndroidRuntimeAdapter::operationFailedForTarget,
            this, [this](const QString &operation, const QString &target,
                         const QString &reason) {
        if (target.isEmpty() ||
            (operation != QStringLiteral("launch") &&
             operation != QStringLiteral("session-start")))
            return;
        const int row = indexOf(target);
        if (row < 0)
            return;
        updateRecord(row, QStringLiteral("launch-failed"), reason);
        emit launchFailed(target, reason);
    });

    connect(m_adapter, &AndroidRuntimeAdapter::applicationLifecycleChanged,
            this, [this](const QString &packageName, const QString &lifecycleState) {
        const int row = indexOf(packageName);
        if (row < 0)
            return;

        QString sessionState;
        QString detail;
        if (lifecycleState == QStringLiteral("started") ||
            lifecycleState == QStringLiteral("foreground")) {
            sessionState = QStringLiteral("activity-running");
            detail = QStringLiteral("The Android provider reported an active activity.");
        } else if (lifecycleState == QStringLiteral("background")) {
            sessionState = QStringLiteral("activity-background");
            detail = QStringLiteral("The Android provider reported a background activity.");
        } else if (lifecycleState == QStringLiteral("stopped")) {
            sessionState = QStringLiteral("activity-stopped");
            detail = QStringLiteral("The Android provider reported that the activity stopped.");
        } else if (lifecycleState == QStringLiteral("crashed")) {
            sessionState = QStringLiteral("activity-crashed");
            detail = QStringLiteral("The Android provider reported an activity failure.");
        } else {
            return;
        }

        m_records[row].activityState = lifecycleState;
        updateRecord(row, sessionState, detail);
    });

    // These events describe compositor-visible windows, not Android process
    // lifetime. One package may own multiple windows at the same time.
    connect(m_adapter, &AndroidRuntimeAdapter::applicationWindowChanged,
            this, [this](const QString &packageName, const QString &windowId,
                         const QString &, const QString &eventState) {
        const int row = indexOf(packageName);
        if (row < 0 || windowId.isEmpty())
            return;

        SessionRecord &record = m_records[row];
        if (eventState == QStringLiteral("opened")) {
            record.windows.insert(windowId);
            record.activeWindows.remove(windowId);
        } else if (eventState == QStringLiteral("activated")) {
            record.windows.insert(windowId);
            record.activeWindows.insert(windowId);
        } else if (eventState == QStringLiteral("deactivated")) {
            if (record.windows.contains(windowId))
                record.activeWindows.remove(windowId);
        } else if (eventState == QStringLiteral("closed")) {
            record.windows.remove(windowId);
            record.activeWindows.remove(windowId);
        } else {
            return;
        }

        QString windowState = QStringLiteral("closed");
        if (!record.windows.isEmpty()) {
            windowState = record.activeWindows.isEmpty()
                ? QStringLiteral("background") : QStringLiteral("foreground");
        }
        updateWindowRecord(row, windowState);
    });

    connect(m_adapter, &AndroidRuntimeAdapter::runtimeStateChanged,
            this, [this]() {
        if (!m_adapter || m_adapter->runtimeState() != QStringLiteral("stopped"))
            return;
        for (int i = 0; i < m_records.size(); ++i) {
            if (m_records[i].state != QStringLiteral("launch-failed")) {
                m_records[i].activityState = QStringLiteral("unknown");
                m_records[i].windows.clear();
                m_records[i].activeWindows.clear();
                updateRecord(i, QStringLiteral("runtime-stopped"),
                             QStringLiteral("The Android user session was stopped. "
                                            "Per-activity state is not available."));
                updateWindowRecord(i, QStringLiteral("closed"));
            }
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
    case WindowStateRole: return record.windowState;
    case WindowCountRole: return record.windows.size();
    case ActiveWindowCountRole: return record.activeWindows.size();
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
        {DetailRole, QByteArrayLiteral("detail")},
        {WindowStateRole, QByteArrayLiteral("windowState")},
        {WindowCountRole, QByteArrayLiteral("windowCount")},
        {ActiveWindowCountRole, QByteArrayLiteral("activeWindowCount")}
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

void AndroidSessionManager::updateWindowRecord(int row, const QString &windowState)
{
    if (row < 0 || row >= m_records.size())
        return;
    m_records[row].windowState = windowState;
    emit dataChanged(index(row, 0), index(row, 0),
                     {WindowStateRole, WindowCountRole, ActiveWindowCountRole});
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
        SessionRecord record;
        record.packageName = package;
        record.name = displayName;
        record.state = QStringLiteral("launch-requested");
        record.lastRequested = QDateTime::currentDateTime().toString(Qt::ISODate);
        record.detail = QStringLiteral("Launch request queued; activity state is not yet known.");
        m_records.append(record);
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
