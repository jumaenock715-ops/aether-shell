#pragma once
#include <QAbstractListModel>
#include <QFileSystemWatcher>
#include <QDir>
#include <QFileInfo>

class FileService final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString currentPath READ currentPath NOTIFY directoryChanged)
    Q_PROPERTY(QString homePath READ homePath CONSTANT)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    enum Role { NameRole = Qt::UserRole + 1, PathRole, IsDirectoryRole, SizeRole, ModifiedRole };
    Q_ENUM(Role)
    explicit FileService(QObject *parent = nullptr);
    QString currentPath() const { return m_currentPath; }
    QString homePath() const { return QDir::homePath(); }
    int count() const { return m_entries.size(); }
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE bool openDirectory(const QString &path);
    Q_INVOKABLE bool goUp();
    Q_INVOKABLE bool createDirectory(const QString &name);
    Q_INVOKABLE bool renameEntry(const QString &oldName, const QString &newName);
    Q_INVOKABLE bool trashEntry(const QString &path);
    Q_INVOKABLE bool openFile(const QString &path);
    Q_INVOKABLE void refresh();
signals:
    void directoryChanged();
    void countChanged();
    void operationFailed(const QString &operation, const QString &reason);
private:
    bool isInsideHome(const QString &path) const;
    QString m_currentPath;
    QFileSystemWatcher m_watcher;
    QFileInfoList m_entries;
};
