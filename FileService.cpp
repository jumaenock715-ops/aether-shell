#include "FileService.h"
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QRegularExpression>
#include <QProcess>
#include <QStorageInfo>
#include <QStandardPaths>
#include <QTimer>

FileService::FileService(QObject *parent) : QAbstractListModel(parent), m_currentPath(QDir::homePath())
{
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &FileService::refresh);
    refreshVolumes();
    m_volumeTimer = new QTimer(this);
    m_volumeTimer->setInterval(5000);
    connect(m_volumeTimer, &QTimer::timeout, this, &FileService::refreshVolumes);
    m_volumeTimer->start();
    refresh();
}
bool FileService::isInside(const QString &path, const QString &root) const
{
    const QString cleanPath = QDir::cleanPath(path);
    const QString cleanRoot = QDir::cleanPath(root);
    return cleanPath == cleanRoot || cleanPath.startsWith(cleanRoot + QDir::separator());
}
bool FileService::isAllowedPath(const QString &path) const
{
    const QString canonical = QFileInfo(path).canonicalFilePath();
    if (canonical.isEmpty()) return false;
    if (isInside(canonical, QDir::homePath())) return true;
    if (!m_androidSharedPath.isEmpty() && isInside(canonical, m_androidSharedPath)) return true;
    for (const QString &root : m_allowedVolumeRoots)
        if (isInside(canonical, root)) return true;
    return false;
}
void FileService::refreshVolumes()
{
    QVariantList next;
    QStringList roots;
    const QList<QStorageInfo> mounted = QStorageInfo::mountedVolumes();
    for (const QStorageInfo &storage : mounted) {
        if (!storage.isValid() || !storage.isReady() || storage.rootPath().isEmpty()) continue;
        const QString root = QDir::cleanPath(storage.rootPath());
        // Never expose the system root or arbitrary fixed host partitions here.
        // Qt 6.6 does not expose the kernel's removable flag through
        // QStorageInfo. Restrict this list to conventional user mount roots.
        const bool removable =
            root.startsWith(QStringLiteral("/media/")) ||
            root.startsWith(QStringLiteral("/run/media/")) ||
            root.startsWith(QStringLiteral("/mnt/"));
        if (!removable || root == QStringLiteral("/")) continue;
        roots.append(root);
        next.append(QVariantMap{
            {QStringLiteral("name"), storage.name().isEmpty() ? root : storage.name()},
            {QStringLiteral("path"), root},
            {QStringLiteral("total"), QVariant::fromValue<qlonglong>(storage.bytesTotal())},
            {QStringLiteral("available"), QVariant::fromValue<qlonglong>(storage.bytesAvailable())},
            {QStringLiteral("readOnly"), storage.isReadOnly()}
        });
    }

    QString androidPath;
    const QString userPath = QDir::homePath() + QStringLiteral("/.local/share/waydroid/data/media/0");
    const QString systemPath = QStringLiteral("/var/lib/waydroid/data/media/0");
    if (QFileInfo(userPath).isDir() && QFileInfo(userPath).isReadable())
        androidPath = QFileInfo(userPath).canonicalFilePath();
    else if (QFileInfo(systemPath).isDir() && QFileInfo(systemPath).isReadable())
        androidPath = QFileInfo(systemPath).canonicalFilePath();

    if (next != m_volumes || roots != m_allowedVolumeRoots || androidPath != m_androidSharedPath) {
        m_volumes = next;
        m_allowedVolumeRoots = roots;
        m_androidSharedPath = androidPath;
        emit volumesChanged();
        if (!isAllowedPath(m_currentPath)) {
            m_currentPath = QDir::homePath();
            emit directoryChanged();
            refresh();
        }
    }
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
    if (!info.exists() || !info.isDir() || canonical.isEmpty() || !isAllowedPath(canonical)) {
        emit operationFailed(QStringLiteral("open-directory"), QStringLiteral("Choose your home folder, an attached removable drive, or the detected Android shared-storage folder."));
        return false;
    }
    if (m_watcher.directories().contains(m_currentPath)) m_watcher.removePath(m_currentPath);
    m_currentPath = canonical; refresh(); emit directoryChanged(); return true;
}
bool FileService::goUp()
{
    const QString home = QDir::cleanPath(QDir::homePath());
    if (m_currentPath == home || (!m_androidSharedPath.isEmpty() && m_currentPath == m_androidSharedPath) ||
        m_allowedVolumeRoots.contains(m_currentPath)) return false;
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
        emit operationFailed(QStringLiteral("rename"), QStringLiteral("Enter valid names.")); return false;
    }
    const QString source = QFileInfo(QDir(m_currentPath).absoluteFilePath(oldName)).canonicalFilePath();
    const QString target = QDir(m_currentPath).absoluteFilePath(newName);
    const QString canonicalParent = QFileInfo(m_currentPath).canonicalFilePath();
    if (source.isEmpty() || !isAllowedPath(source) || canonicalParent.isEmpty() ||
        !isAllowedPath(canonicalParent) ||
        QFileInfo(target).absolutePath() != canonicalParent ||
        QFileInfo::exists(target) || !QFile::rename(source, target)) {
        emit operationFailed(QStringLiteral("rename"), QStringLiteral("The item could not be renamed.")); return false;
    }
    refresh(); return true;
}
bool FileService::trashEntry(const QString &path)
{
    const QFileInfo info(path);
    const QString canonical = info.canonicalFilePath();
    if (!info.exists() || canonical.isEmpty() || !isAllowedPath(canonical) ||
        canonical == QDir::cleanPath(QDir::homePath()) || canonical == m_androidSharedPath ||
        m_allowedVolumeRoots.contains(canonical)) {
        emit operationFailed(QStringLiteral("trash"), QStringLiteral("That item cannot be moved to the trash.")); return false;
    }
    const int result = QProcess::execute(QStringLiteral("gio"), {QStringLiteral("trash"), canonical});
    if (result != 0) {
        emit operationFailed(QStringLiteral("trash"), QStringLiteral("Could not move the item to the desktop trash. The gio utility may be unavailable.")); return false;
    }
    refresh(); return true;
}
bool FileService::openFile(const QString &path)
{
    const QFileInfo info(path);
    const QString canonical = info.canonicalFilePath();
    if (!info.isFile() || canonical.isEmpty() || !isAllowedPath(canonical) ||
        !QDesktopServices::openUrl(QUrl::fromLocalFile(canonical))) {
        emit operationFailed(QStringLiteral("open-file"), QStringLiteral("The file could not be opened by its registered MIME/default application.")); return false;
    }
    return true;
}
