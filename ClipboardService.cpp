#include "ClipboardService.h"
#include <QGuiApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMimeData>
#include <QPixmap>
#include <QStandardPaths>

ClipboardService::ClipboardService(QObject *parent) : QAbstractListModel(parent)
{
    m_clipboard = QGuiApplication::clipboard();
    m_cacheDirectory = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                       + QStringLiteral("/clipboard");
    QDir().mkpath(m_cacheDirectory);
    if (m_clipboard)
        connect(m_clipboard, &QClipboard::dataChanged, this, &ClipboardService::captureClipboard);
}
QString ClipboardService::currentText() const
{
    return m_clipboard ? m_clipboard->text(QClipboard::Clipboard) : QString();
}
int ClipboardService::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_history.size();
}
QVariant ClipboardService::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_history.size()) return {};
    const Entry &entry = m_history.at(index.row());
    switch (role) {
    case TextRole: return entry.text;
    case TimeRole: return entry.time;
    case KindRole: return entry.kind;
    case PreviewRole: return entry.text;
    case ImageUrlRole: return entry.imagePath.isEmpty() ? QString() : QUrl::fromLocalFile(entry.imagePath).toString();
    case UrlsRole: {
        QStringList result;
        for (const QUrl &url : entry.urls) result.append(url.toString());
        return result;
    }
    default: return {};
    }
}
QHash<int, QByteArray> ClipboardService::roleNames() const
{
    return {{TextRole, "text"}, {TimeRole, "time"}, {KindRole, "kind"},
            {PreviewRole, "preview"}, {ImageUrlRole, "imageUrl"}, {UrlsRole, "urls"}};
}
void ClipboardService::captureClipboard()
{
    if (!m_clipboard) return;
    const QMimeData *mime = m_clipboard->mimeData(QClipboard::Clipboard);
    if (!mime) return;
    Entry entry;
    entry.time = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm"));
    if (mime->hasImage()) {
        entry.image = qvariant_cast<QImage>(mime->imageData());
        if (entry.image.isNull())
            entry.image = qvariant_cast<QPixmap>(mime->imageData()).toImage();
        if (entry.image.isNull()) return;
        entry.kind = QStringLiteral("image");
        entry.text = QStringLiteral("Copied image");
        const QString filename = QStringLiteral("clip-%1.png")
            .arg(QDateTime::currentMSecsSinceEpoch());
        entry.imagePath = m_cacheDirectory + QLatin1Char('/') + filename;
        if (!entry.image.save(entry.imagePath, "PNG")) entry.imagePath.clear();
    } else if (mime->hasUrls()) {
        entry.urls = mime->urls();
        entry.kind = QStringLiteral("files");
        QStringList names;
        for (const QUrl &url : entry.urls)
            names.append(url.isLocalFile() ? QFileInfo(url.toLocalFile()).fileName() : url.toDisplayString());
        entry.text = names.join(QStringLiteral(", "));
    } else if (mime->hasText()) {
        entry.text = mime->text();
        if (entry.text.isEmpty()) return;
        entry.kind = QStringLiteral("text");
    } else {
        return;
    }

    if (!m_history.isEmpty()) {
        const Entry &first = m_history.first();
        if (first.kind == entry.kind && first.text == entry.text &&
            (entry.kind != QStringLiteral("image") || first.image == entry.image))
            return;
    }
    if (m_history.size() >= MaxEntries) {
        beginRemoveRows({}, m_history.size() - 1, m_history.size() - 1);
        const Entry removed = m_history.takeLast();
        if (!removed.imagePath.isEmpty()) QFile::remove(removed.imagePath);
        endRemoveRows();
    }
    beginInsertRows({}, 0, 0);
    m_history.prepend(entry);
    endInsertRows();
    emit countChanged();
    emit currentTextChanged();
    emit clipboardChanged(entry.kind == QStringLiteral("text") ? entry.text : QString());
}
void ClipboardService::copyText(const QString &text)
{
    if (m_clipboard) m_clipboard->setText(text, QClipboard::Clipboard);
}
void ClipboardService::copyUrls(const QStringList &urls)
{
    if (!m_clipboard) return;
    QList<QUrl> parsed;
    for (const QString &value : urls) {
        const QUrl url = QUrl::fromUserInput(value);
        if (url.isValid()) parsed.append(url);
    }
    if (parsed.isEmpty()) return;
    auto *mime = new QMimeData;
    mime->setUrls(parsed);
    m_clipboard->setMimeData(mime, QClipboard::Clipboard);
}
void ClipboardService::restore(int row)
{
    if (!m_clipboard || row < 0 || row >= m_history.size()) return;
    const Entry &entry = m_history.at(row);
    if (entry.kind == QStringLiteral("image")) m_clipboard->setImage(entry.image, QClipboard::Clipboard);
    else if (entry.kind == QStringLiteral("files")) {
        auto *mime = new QMimeData;
        mime->setUrls(entry.urls);
        m_clipboard->setMimeData(mime, QClipboard::Clipboard);
    } else copyText(entry.text);
}
void ClipboardService::clearHistory()
{
    if (m_history.isEmpty()) return;
    for (const Entry &entry : m_history)
        if (!entry.imagePath.isEmpty()) QFile::remove(entry.imagePath);
    beginResetModel(); m_history.clear(); endResetModel();
    emit countChanged();
}
