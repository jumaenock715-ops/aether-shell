#pragma once
#include <QAbstractListModel>
#include <QFileSystemWatcher>
#include <QDir>
#include <QFileInfo>
#include <QVariantList>

class QTimer;

class FileService final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString currentPath READ currentPath NOTIFY directoryChanged)
    Q_PROPERTY(QString homePath READ homePath CONSTANT)
    Q_PROPERTY(QString androidSharedPath READ androidSharedPath NOTIFY volumesChanged)
    Q_PROPERTY(QVariantList volumes READ volumes NOTIFY volumesChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    enum Role { NameRole = Qt::UserRole + 1, PathRole, IsDirectoryRole, SizeRole, ModifiedRole };
    Q_ENUM(Role)
    explicit FileService(QObject *parent = nullptr);
    QString currentPath() const { return m_currentPath; }
    QString homePath() const { return QDir::homePath(); }
    QString androidSharedPath() const { return m_androidSharedPath; }
    QVariantList volumes() const { return m_volumes; }
    int count() const { return m_entries.size(); }
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE bool openDirectory(const QString &path);
    Q_INVOKABLE bool goUp();
    Q_INVOKABLE bool createDirectory(const QString &name);
    Q_INVOKABLE bool renameEntry(const QString &oldName, const QString &newName);
    Q_INVOKABLE bool trashEntry(const QString &path);
    Q_INVOKABLE bool openFile(const QString &path);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void refreshVolumes();
signals:
    void directoryChanged();
    void countChanged();
    void volumesChanged();
    void operationFailed(const QString &operation, const QString &reason);
private:
    bool isAllowedPath(const QString &path) const;
    bool isInside(const QString &path, const QString &root) const;
    QString m_currentPath;
    QString m_androidSharedPath;
    QVariantList m_volumes;
    QStringList m_allowedVolumeRoots;
    QFileSystemWatcher m_watcher;
    QFileInfoList m_entries;
    QTimer *m_volumeTimer = nullptr;
};
