#include "ClipboardService.h"
#include <QGuiApplication>
#include <QDateTime>

ClipboardService::ClipboardService(QObject *parent) : QAbstractListModel(parent)
{
    m_clipboard = QGuiApplication::clipboard();
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
    if (role == TextRole) return entry.text;
    if (role == TimeRole) return entry.time;
    return {};
}
QHash<int, QByteArray> ClipboardService::roleNames() const
{
    return {{TextRole, "text"}, {TimeRole, "time"}};
}
void ClipboardService::captureClipboard()
{
    const QString text = currentText();
    emit currentTextChanged();
    if (text.isEmpty()) return;
    if (!m_history.isEmpty() && m_history.first().text == text) return;
    if (m_history.size() >= MaxEntries) {
        beginRemoveRows({}, m_history.size() - 1, m_history.size() - 1);
        m_history.removeLast();
        endRemoveRows();
    }
    beginInsertRows({}, 0, 0);
    m_history.prepend({text, QDateTime::currentDateTime().toString(QStringLiteral("hh:mm"))});
    endInsertRows();
    emit countChanged();
    emit clipboardChanged(text);
}
void ClipboardService::copyText(const QString &text)
{
    if (m_clipboard) m_clipboard->setText(text, QClipboard::Clipboard);
}
void ClipboardService::restore(int row)
{
    if (row >= 0 && row < m_history.size()) copyText(m_history.at(row).text);
}
void ClipboardService::clearHistory()
{
    if (m_history.isEmpty()) return;
    beginResetModel(); m_history.clear(); endResetModel();
    emit countChanged();
}
