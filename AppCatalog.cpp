#include "AppCatalog.h"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QTextStream>
#include <algorithm>

AppCatalog::AppCatalog(QObject *parent) : QObject(parent) {}

static QString stripFieldCodes(QString exec)
{
    exec.replace("%%", "\x01");
    exec.remove(QRegularExpression("%[fFuUdDnNickvm]"));
    exec.replace("\x01", "%");
    return exec.trimmed();
}

void AppCatalog::reload()
{
    QSet<QString> seen;
    QVector<QVariantMap> found;
    m_exec.clear();

    const QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    for (const QString &dirPath : dirs) {
        QDir dir(dirPath);
        const QStringList files = dir.entryList({"*.desktop"}, QDir::Files);
        for (const QString &fileName : files) {
            const QString desktopId = fileName;
            if (seen.contains(desktopId)) continue;   // earlier dirs take priority

            QFile f(dir.filePath(fileName));
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

            QTextStream in(&f);
            bool inEntry = false;
            QString name, icon, exec, type;
            bool hidden = false;
            while (!in.atEnd()) {
                const QString line = in.readLine().trimmed();
                if (line.startsWith('[')) { inEntry = (line == "[Desktop Entry]"); continue; }
                if (!inEntry || line.isEmpty() || line.startsWith('#')) continue;
                const int eq = line.indexOf('=');
                if (eq <= 0) continue;
                const QString key = line.left(eq).trimmed();
                const QString val = line.mid(eq + 1).trimmed();
                if (key == "Name") name = val;
                else if (key == "Icon") icon = val;
                else if (key == "Exec") exec = val;
                else if (key == "Type") type = val;
                else if ((key == "NoDisplay" || key == "Hidden" || key == "Terminal") && val == "true") hidden = true;
            }

            seen.insert(desktopId);
            if (type != "Application" || hidden || name.isEmpty() || exec.isEmpty()) continue;

            QVariantMap e;
            e["appId"] = desktopId;
            e["desktopId"] = desktopId;
            e["name"] = name;
            e["icon"] = icon;
            e["kind"] = "system";
            e["tint"] = QColor::fromHsl(qHash(name) % 360, 150, 120).name();
            found.append(e);
            m_exec.insert(desktopId, stripFieldCodes(exec));
        }
    }

    std::sort(found.begin(), found.end(), [](const QVariantMap &a, const QVariantMap &b) {
        return a["name"].toString().compare(b["name"].toString(), Qt::CaseInsensitive) < 0;
    });

    m_apps.clear();
    for (const QVariantMap &e : std::as_const(found)) m_apps.append(e);
    emit appsChanged();
}

bool AppCatalog::launch(const QString &desktopId)
{
    const QString exec = m_exec.value(desktopId);
    if (exec.isEmpty()) return false;
    QStringList parts = QProcess::splitCommand(exec);
    if (parts.isEmpty()) return false;
    const QString program = parts.takeFirst();
    return QProcess::startDetached(program, parts);
}

QPixmap IconProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    const int s = requestedSize.isValid() && requestedSize.width() > 0 ? requestedSize.width() : 128;
    const QIcon icon = id.startsWith('/') ? QIcon(id) : QIcon::fromTheme(id);
    if (icon.isNull()) return QPixmap();   // QML falls back to the letter tile
    const QPixmap pm = icon.pixmap(QSize(s, s));
    if (size) *size = pm.size();
    return pm;
}
