#pragma once

#include <QAbstractListModel>
#include <QProcess>
#include <QVector>

class SessionManager final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        DesktopIdRole = Qt::UserRole + 1,
        NameRole,
        ProcessIdRole,
        StateRole
    };
    Q_ENUM(Role)

    explicit SessionManager(QObject *parent = nullptr);
    ~SessionManager() override;

    int count() const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE bool launch(const QString &desktopId,
                            const QString &name,
                            const QString &program,
                            const QStringList &arguments);
    Q_INVOKABLE void terminate(const QString &desktopId);
    Q_INVOKABLE void terminateAll();

signals:
    void countChanged();
    void applicationStarted(const QString &desktopId, qint64 processId);
    void applicationExited(const QString &desktopId, int exitCode, QProcess::ExitStatus status);
    void applicationFailed(const QString &desktopId, const QString &reason);

private:
    struct Instance {
        QProcess *process = nullptr;
        QString desktopId;
        QString name;
        QString state;
    };

    int indexOf(QProcess *process) const;
    void removeProcess(QProcess *process);
    QVector<Instance> m_instances;
};
