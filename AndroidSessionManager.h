#pragma once

#include <QAbstractListModel>
#include <QPointer>
#include <QVariantList>
#include <QVector>

class AndroidRuntimeAdapter;

// Tracks Android launch requests without pretending that the Waydroid CLI
// process is the lifetime of the Android activity. Waydroid currently gives
// this layer command completion, not authoritative per-activity lifecycle
// events, so activityState remains "unknown" until a real event bridge exists.
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
        DetailRole
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
    };

    int indexOf(const QString &packageName) const;
    void updateRecord(int row, const QString &state, const QString &detail);

    QPointer<AndroidRuntimeAdapter> m_adapter;
    QVector<SessionRecord> m_records;
};
