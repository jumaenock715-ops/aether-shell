#include "WaydroidBridgeService.h"
#include "NotificationService.h"
#include "ClipboardService.h"
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace {
const QString kBridgeRelative = QStringLiteral("Android/data/org.aetheros.bridge/files");
}

WaydroidBridgeService::WaydroidBridgeService(NotificationService *notifications,
                                             ClipboardService *clipboard,
                                             QObject *parent)
    : QObject(parent), m_notifications(notifications), m_clipboard(clipboard)
{
    if (m_clipboard) {
        connect(m_clipboard, &ClipboardService::clipboardChanged,
                this, &WaydroidBridgeService::writeHostClipboard);
    }
    m_timer = new QTimer(this);
    m_timer->setInterval(900);
    connect(m_timer, &QTimer::timeout, this, &WaydroidBridgeService::poll);
    m_timer->start();
    poll();
}

void WaydroidBridgeService::setStatus(const QString &status)
{
    if (m_status == status && m_available == (status == QStringLiteral("connected"))) return;
    m_status = status;
    m_available = status == QStringLiteral("connected");
    emit changed();
}

void WaydroidBridgeService::discoverBridgeDirectory()
{
    QStringList roots;
    const QString userRoot = QDir::homePath() + QStringLiteral("/.local/share/waydroid/data/media/0");
    const QString systemRoot = QStringLiteral("/var/lib/waydroid/data/media/0");
    if (QFileInfo(userRoot).isDir()) roots.append(userRoot);
    if (QFileInfo(systemRoot).isDir()) roots.append(systemRoot);

    for (const QString &root : roots) {
        const QString candidate = QDir(root).filePath(kBridgeRelative);
        if (QFileInfo(candidate).isDir() && QFileInfo(candidate).isReadable()) {
            if (m_bridgeDirectory != candidate) {
                m_bridgeDirectory = candidate;
                m_notificationOffset = 0;
                m_lastHostClipboard.clear();
            }
            setStatus(QStringLiteral("connected"));
            return;
        }
    }
    m_bridgeDirectory.clear();
    setStatus(QStringLiteral("waiting-for-android-companion"));
}

void WaydroidBridgeService::poll()
{
    discoverBridgeDirectory();
    if (m_bridgeDirectory.isEmpty()) return;
    processNotificationEvents();
    processClipboard();
}

void WaydroidBridgeService::processNotificationEvents()
{
    const QString path = QDir(m_bridgeDirectory).filePath(QStringLiteral("notifications.ndjson"));
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;
    if (file.size() < m_notificationOffset) m_notificationOffset = 0;
    if (!file.seek(m_notificationOffset)) return;
    const QByteArray pending = file.readAll();
    const qsizetype completeLength = pending.lastIndexOf('\n') + 1;
    if (completeLength <= 0) return;
    const QByteArray complete = pending.left(completeLength);
    m_notificationOffset += completeLength;

    for (const QByteArray &line : complete.split('\n')) {
        if (line.trimmed().isEmpty()) continue;
        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(line, &error);
        if (error.error != QJsonParseError::NoError || !document.isObject()) continue;
        const QJsonObject event = document.object();
        const QString key = event.value(QStringLiteral("key")).toString();
        const QString packageName = event.value(QStringLiteral("packageName")).toString();
        if (key.isEmpty() || packageName.isEmpty()) continue;
        if (event.value(QStringLiteral("event")).toString() == QStringLiteral("removed")) {
            const uint id = m_notificationIds.take(key);
            if (id) m_notifications->closeNotification(id, 2);
            continue;
        }
        const QString title = event.value(QStringLiteral("title")).toString().left(512);
        const QString body = event.value(QStringLiteral("text")).toString().left(4096);
        if (title.isEmpty() && body.isEmpty()) continue;
        const uint replaces = m_notificationIds.value(key, 0);
        const uint id = m_notifications->postDesktopNotification(
            packageName, replaces, QString(), title.isEmpty() ? packageName : title,
            body, {}, {}, -1);
        m_notificationIds.insert(key, id);
        ++m_forwardedNotifications;
        emit changed();
    }
}

void WaydroidBridgeService::writeHostClipboard(const QString &text)
{
    if (m_bridgeDirectory.isEmpty() || text.isEmpty()) return;
    const QByteArray bytes = text.toUtf8();
    if (bytes == m_lastHostClipboard) return;
    QSaveFile file(QDir(m_bridgeDirectory).filePath(QStringLiteral("clipboard-to-android.txt")));
    if (!file.open(QIODevice::WriteOnly)) return;
    if (file.write(bytes) != bytes.size() || !file.commit()) return;
    m_lastHostClipboard = bytes;
}

void WaydroidBridgeService::processClipboard()
{
    const QString path = QDir(m_bridgeDirectory).filePath(QStringLiteral("clipboard-from-android.txt"));
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;
    const QByteArray bytes = file.readAll();
    file.close();
    QFile::remove(path);
    if (bytes.isEmpty() || bytes.size() > 1024 * 1024 || bytes == m_lastHostClipboard) return;
    const QString text = QString::fromUtf8(bytes);
    if (text.isEmpty() || !m_clipboard || text == m_clipboard->currentText()) return;
    m_lastHostClipboard = bytes;
    m_clipboard->copyText(text);
}
