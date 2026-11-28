#include "FileService.h"
#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>
#include <QRegularExpression>

FileService::FileService(QObject *parent) : QAbstractListModel(parent), m_currentPath(QDir::homePath())
{
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &FileService::refresh);
    refresh();
}
bool FileService::isInsideHome(const QString &path) const
{
    const QString home = QDir::cleanPath(QDir::homePath());
    const QString clean = QDir::cleanPath(path);
    return clean == home || clean.startsWith(home + QDir::separator());
}
int FileService::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}
QVariant FileService::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) return {};
    const QFileInfo info = m_entries.at(index.row());
    switch (role) {
    case NameRole: return info.fileName();
    case PathRole: return info.absoluteFilePath();
    case IsDirectoryRole: return info.isDir();
    case SizeRole: return info.isDir() ? QStringLiteral("—") : QString::number(info.size());
    case ModifiedRole: return info.lastModified().toString(QStringLiteral("yyyy-MM-dd hh:mm"));
    default: return {};
    }
}
QHash<int, QByteArray> FileService::roleNames() const
{
    return {{NameRole, "name"}, {PathRole, "path"}, {IsDirectoryRole, "isDirectory"},
            {SizeRole, "size"}, {ModifiedRole, "modified"}};
}
void FileService::refresh()
{
    const QDir dir(m_currentPath);
    QFileInfoList next = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Readable,
                                           QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);
    beginResetModel(); m_entries = next; endResetModel();
    const QStringList watched = m_watcher.directories();
    if (!watched.contains(m_currentPath)) m_watcher.addPath(m_currentPath);
    emit countChanged();
}
bool FileService::openDirectory(const QString &path)
{
    const QFileInfo info(path);
    const QString canonical = info.canonicalFilePath();
    if (!info.exists() || !info.isDir() || canonical.isEmpty() || !isInsideHome(canonical)) {
        emit operationFailed(QStringLiteral("open-directory"), QStringLiteral("Only directories inside the user's home folder are currently available."));
        return false;
    }
    if (m_watcher.directories().contains(m_currentPath)) m_watcher.removePath(m_currentPath);
    m_currentPath = canonical; refresh(); emit directoryChanged(); return true;
}
bool FileService::goUp()
{
    const QString home = QDir::cleanPath(QDir::homePath());
    if (m_currentPath == home) return false;
    return openDirectory(QDir(m_currentPath).absoluteFilePath(QStringLiteral("..")));
}
bool FileService::createDirectory(const QString &name)
{
    if (name.isEmpty() || name == QStringLiteral(".") || name == QStringLiteral("..") ||
        name.contains('/') || name.contains('\\') || name.contains(QRegularExpression(QStringLiteral("[\\x00-\\x1F]")))) {
        emit operationFailed(QStringLiteral("create-folder"), QStringLiteral("Enter a valid folder name."));
        return false;
    }
    if (!QDir(m_currentPath).mkdir(name)) {
        emit operationFailed(QStringLiteral("create-folder"), QStringLiteral("The folder could not be created."));
        return false;
    }
    refresh(); return true;
}
bool FileService::renameEntry(const QString &oldName, const QString &newName)
{
    if (oldName.isEmpty() || newName.isEmpty() || oldName.contains('/') || newName.contains('/') ||
        oldName == QStringLiteral("..") || newName == QStringLiteral("..")) {
        emit operationFailed(QStringLiteral("rename"), QStringLiteral("Enter valid names."));
        return false;
    }
    const QString source = QDir(m_currentPath).absoluteFilePath(oldName);
    const QString target = QDir(m_currentPath).absoluteFilePath(newName);
    if (!isInsideHome(source) || !isInsideHome(target) || QFileInfo::exists(target) ||
        !QFile::rename(source, target)) {
        emit operationFailed(QStringLiteral("rename"), QStringLiteral("The item could not be renamed."));
        return false;
    }
    refresh(); return true;
}
bool FileService::openFile(const QString &path)
{
    const QFileInfo info(path);
    const QString canonical = info.canonicalFilePath();
    if (!info.isFile() || canonical.isEmpty() || !isInsideHome(canonical) ||
        !QDesktopServices::openUrl(QUrl::fromLocalFile(canonical))) {
        emit operationFailed(QStringLiteral("open-file"), QStringLiteral("The file could not be opened by a registered desktop application."));
        return false;
    }
    return true;
}
