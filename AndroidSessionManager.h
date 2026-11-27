#pragma once

#include <QAbstractListModel>
#include <QPointer>
#include <QSet>
#include <QStringList>
#include <QVariantList>
#include <QVector>

class AndroidRuntimeAdapter;

// Tracks Android launch requests separately from native Linux processes.
// Activity state is populated only by explicit runtime lifecycle events.
// Window state is a separate view derived only from compositor window events.
class AndroidSessionManager final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        PackageNameRole = Qt::UserRole + 1,
        NameRole,
        StateRole,
        ActivityStateRole,
        LastRequestedRole,
        DetailRole,
        WindowStateRole,
        WindowCountRole,
        ActiveWindowCountRole
    };
    Q_ENUM(Role)

    explicit AndroidSessionManager(AndroidRuntimeAdapter *adapter,
                                   QObject *parent = nullptr);

    int count() const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE bool launch(const QString &packageName, const QString &name);
    Q_INVOKABLE void clearHistory();

signals:
    void countChanged();
    void launchRequested(const QString &packageName);
    void launchCommandCompleted(const QString &packageName);
    void launchFailed(const QString &packageName, const QString &reason);

private:
    struct SessionRecord {
        QString packageName;
        QString name;
        QString state;
        QString activityState = QStringLiteral("unknown");
        QString lastRequested;
        QString detail;
        QString windowState = QStringLiteral("unknown");
        QSet<QString> windows;
        QSet<QString> activeWindows;
        bool activationKnown = false;
    };

    int indexOf(const QString &packageName) const;
    void updateRecord(int row, const QString &state, const QString &detail);
    void updateWindowRecord(int row, const QString &windowState);

    QPointer<AndroidRuntimeAdapter> m_adapter;
    QVector<SessionRecord> m_records;
};
