#include "ApplicationService.h"
#include "SessionManager.h"
#include "AndroidRuntimeAdapter.h"

#include <QColor>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QVariantMap>
#include <algorithm>

namespace {

QString desktopIdForPath(const QString &root, const QString &filePath)
{
    QString relative = QDir(root).relativeFilePath(filePath);
    if (relative.startsWith("../") || relative == "..")
        return {};
    relative.replace('/', '-');
    return relative;
}

bool desktopEntryVisible(const QSettings &settings)
{
    if (settings.value("Hidden", false).toBool() ||
        settings.value("NoDisplay", false).toBool())
        return false;

    const QStringList onlyShowIn =
        settings.value("OnlyShowIn").toString().split(';', Qt::SkipEmptyParts);
    const QStringList notShowIn =
        settings.value("NotShowIn").toString().split(';', Qt::SkipEmptyParts);
    const QString currentDesktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP");
    const QStringList desktops = currentDesktop.split(':', Qt::SkipEmptyParts);

    if (!onlyShowIn.isEmpty()) {
        bool matches = false;
        for (const QString &desktop : desktops)
            matches = matches || onlyShowIn.contains(desktop);
        if (!matches)
            return false;
    }

    for (const QString &desktop : desktops) {
        if (notShowIn.contains(desktop))
            return false;
    }
    return true;
}

// Expand desktop-entry field codes for a launch without file/URL context.
// Unknown field codes are rejected instead of being passed to a shell.
bool buildLaunchCommand(const QString &exec, const QString &name,
                        const QString &icon, const QString &desktopFile,
                        QString *program, QStringList *arguments)
{
    const QStringList tokens = QProcess::splitCommand(exec);
    if (tokens.isEmpty())
        return false;

    QStringList expanded;
    for (QString token : tokens) {
        if (token == "%i") {
            if (!icon.isEmpty())
                expanded << "--icon" << icon;
            continue;
        }

        if (token == "%f" || token == "%F" ||
            token == "%u" || token == "%U")
            continue;

        token.replace("%%", "\x01");
        token.replace("%c", name);
        token.replace("%k", desktopFile);
        token.remove(QRegularExpression("%[dDnNvm]"));

        if (QRegularExpression("%[A-Za-z]").match(token).hasMatch())
            return false;

        token.replace("\x01", "%");
        if (!token.isEmpty())
            expanded.append(token);
    }

    if (expanded.isEmpty() || expanded.first().isEmpty())
        return false;

    *program = expanded.takeFirst();
    *arguments = expanded;
    return true;
}

} // namespace

ApplicationService::ApplicationService(QObject *parent)
    : QObject(parent)
{
}

void ApplicationService::setSessionManager(SessionManager *sessionManager)
{
    if (m_sessionManager == sessionManager)
        return;

    if (m_sessionManager)
        disconnect(m_sessionManager, nullptr, this, nullptr);

    m_sessionManager = sessionManager;
    if (!m_sessionManager)
        return;

    connect(m_sessionManager, &SessionManager::applicationStarted,
            this, [this](const QString &desktopId, qint64) {
        emit launchStarted(desktopId);
    });
    connect(m_sessionManager, &SessionManager::applicationFailed,
            this, &ApplicationService::launchFailed);
}

void ApplicationService::setAndroidRuntimeAdapter(AndroidRuntimeAdapter *adapter)
{
    if (m_androidAdapter == adapter)
        return;

    if (m_androidAdapter)
        disconnect(m_androidAdapter, nullptr, this, nullptr);

    m_androidAdapter = adapter;
    if (m_androidAdapter) {
        connect(m_androidAdapter, &AndroidRuntimeAdapter::applicationsChanged,
                this, &ApplicationService::reload);
        connect(m_androidAdapter, &AndroidRuntimeAdapter::runtimeStateChanged,
                this, &ApplicationService::appsChanged);
        connect(m_androidAdapter, &AndroidRuntimeAdapter::operationFailed,
                this, [this](const QString &operation, const QString &reason) {
            emit launchFailed(operation, reason);
        });
    }
    reload();
}

void ApplicationService::reload()
{
    QHash<QString, LaunchEntry> nextLaunchEntries;
    QHash<QString, QVariantMap> selectedEntries;

    const QStringList applicationDirs =
        QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);

    // The first occurrence of a desktop ID wins, including Hidden=true entries
    // which intentionally mask lower-priority copies.
    for (const QString &rootPath : applicationDirs) {
        const QDir root(rootPath);
        if (!root.exists())
            continue;

        QDirIterator it(rootPath, {"*.desktop"},
                        QDir::Files | QDir::Readable,
                        QDirIterator::Subdirectories);

        while (it.hasNext()) {
            const QString filePath = it.next();
            const QString desktopId = desktopIdForPath(rootPath, filePath);
            if (desktopId.isEmpty() || selectedEntries.contains(desktopId))
                continue;

            // Reserve the ID before parsing so a higher-priority Hidden entry
            // also masks any lower-priority entry with the same desktop ID.
            selectedEntries.insert(desktopId, {});

            QSettings desktopFile(filePath, QSettings::IniFormat);
            desktopFile.setFallbacksEnabled(false);
            desktopFile.beginGroup("Desktop Entry");

            const QString type = desktopFile.value("Type").toString();
            const QString name = desktopFile.value("Name").toString();
            const QString exec = desktopFile.value("Exec").toString();
            const QString icon = desktopFile.value("Icon").toString();
            const QString tryExec = desktopFile.value("TryExec").toString();
            const bool terminal = desktopFile.value("Terminal", false).toBool();

            if (type != "Application" || name.isEmpty() || exec.isEmpty() ||
                terminal || !desktopEntryVisible(desktopFile))
                continue;

            if (!tryExec.isEmpty()) {
                const QFileInfo info(tryExec);
                const bool exists = info.isAbsolute()
                    ? (info.exists() && info.isExecutable())
                    : !QStandardPaths::findExecutable(tryExec).isEmpty();
                if (!exists)
                    continue;
            }

            QString program;
            QStringList arguments;
            if (!buildLaunchCommand(exec, name, icon, filePath,
                                    &program, &arguments))
                continue;

            const QFileInfo programInfo(program);
            const bool programExists = programInfo.isAbsolute()
                ? (programInfo.exists() && programInfo.isExecutable())
                : !QStandardPaths::findExecutable(program).isEmpty();
            if (!programExists)
                continue;

            QVariantMap app;
            app["appId"] = QStringLiteral("linux:") + desktopId;
            app["desktopId"] = desktopId;
            app["provider"] = QStringLiteral("linux");
            app["name"] = name;
            app["genericName"] = desktopFile.value("GenericName").toString();
            app["comment"] = desktopFile.value("Comment").toString();
            app["icon"] = icon;
            app["categories"] = desktopFile.value("Categories").toString()
                                    .split(';', Qt::SkipEmptyParts);
            app["desktopFile"] = filePath;
            app["kind"] = "system";
            app["tint"] = QColor::fromHsl(qHash(name) % 360, 150, 120).name();

            LaunchEntry launchEntry;
            launchEntry.program = program;
            launchEntry.arguments = arguments;
            launchEntry.name = name;
            nextLaunchEntries.insert(desktopId, launchEntry);
            selectedEntries[desktopId] = app;
        }
    }

    QVector<QVariantMap> sorted;
    sorted.reserve(selectedEntries.size());
    for (auto it = selectedEntries.cbegin(); it != selectedEntries.cend(); ++it) {
        if (it.value().value("kind").toString() == "system")
            sorted.append(it.value());
    }

    std::sort(sorted.begin(), sorted.end(),
              [](const QVariantMap &a, const QVariantMap &b) {
        return a.value("name").toString().compare(
                   b.value("name").toString(), Qt::CaseInsensitive) < 0;
    });

    QVariantList nextApps;
    nextApps.reserve(sorted.size());
    for (const QVariantMap &entry : sorted)
        nextApps.append(entry);

    // Android records are normalized at the provider boundary and merged into
    // the same catalogue. The provider is optional until a runtime is selected.
    if (m_androidAdapter && m_androidAdapter->isAvailable()) {
        const QVariantList androidApps = m_androidAdapter->applications();
        for (const QVariant &value : androidApps) {
            const QVariantMap source = value.toMap();
            const QString packageName = source.value("packageName").toString().trimmed();
            const QString name = source.value("name").toString().trimmed();
            if (packageName.isEmpty() || name.isEmpty())
                continue;

            QVariantMap app;
            app["appId"] = QStringLiteral("android:") + packageName;
            app["packageName"] = packageName;
            app["provider"] = QStringLiteral("android");
            app["kind"] = QStringLiteral("android");
            app["name"] = name;
            app["icon"] = source.value("icon");
            app["categories"] = source.value("categories");
            app["tint"] = QColor::fromHsl(qHash(packageName) % 360, 150, 120).name();
            nextApps.append(app);
        }
    }

    m_launchEntries = std::move(nextLaunchEntries);
    m_apps = std::move(nextApps);
    emit appsChanged();
}

bool ApplicationService::launch(const QString &appId)
{
    if (appId.startsWith(QStringLiteral("android:"))) {
        const QString packageName = appId.mid(QStringLiteral("android:").size());
        if (!m_androidAdapter || !m_androidAdapter->isAvailable()) {
            emit launchFailed(appId, QStringLiteral("The Android runtime is unavailable."));
            return false;
        }
        QString error;
        if (!m_androidAdapter->launchPackage(packageName, &error)) {
            emit launchFailed(appId, error.isEmpty()
                ? QStringLiteral("The Android application could not be launched.") : error);
            return false;
        }
        emit launchStarted(appId);
        return true;
    }

    const QString desktopId = appId.startsWith(QStringLiteral("linux:"))
        ? appId.mid(QStringLiteral("linux:").size()) : appId;
    const auto it = m_launchEntries.constFind(desktopId);
    if (it == m_launchEntries.cend()) {
        emit launchFailed(desktopId, QStringLiteral("Application is not available."));
        return false;
    }

    if (!m_sessionManager) {
        emit launchFailed(desktopId,
                          QStringLiteral("The session manager is not available."));
        return false;
    }

    return m_sessionManager->launch(desktopId, it->name,
                                    it->program, it->arguments);
}
