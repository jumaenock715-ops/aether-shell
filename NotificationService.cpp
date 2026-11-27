#include "NotificationService.h"

#include <QDateTime>

NotificationService::NotificationService(QObject *parent)
    : QAbstractListModel(parent)
{
}

QAbstractItemModel *NotificationService::items()
{
    return this;
}

int NotificationService::count() const
{
    return m_notifications.size();
}

int NotificationService::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_notifications.size();
}

QVariant NotificationService::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_notifications.size())
        return {};

    const QVariantMap &notification = m_notifications.at(index.row());

    switch (role) {
    case TitleRole:
        return notification.value(QStringLiteral("title"));
    case BodyRole:
        return notification.value(QStringLiteral("body"));
    case TimeRole:
        return notification.value(QStringLiteral("time"));
    default:
        return {};
    }
}

QHash<int, QByteArray> NotificationService::roleNames() const
{
    return {
        {TitleRole, QByteArrayLiteral("title")},
        {BodyRole, QByteArrayLiteral("body")},
        {TimeRole, QByteArrayLiteral("time")}
    };
}

void NotificationService::push(const QString &title, const QString &body)
{
    const int insertionRow = 0;
    beginInsertRows(QModelIndex(), insertionRow, insertionRow);
    m_notifications.prepend({
        {QStringLiteral("title"), title},
        {QStringLiteral("body"), body},
        {QStringLiteral("time"), QDateTime::currentDateTime().toString(QStringLiteral("hh:mm"))}
    });
    endInsertRows();

    emit countChanged();
    emit pushed(title, body);
}

void NotificationService::clear()
{
    if (m_notifications.isEmpty())
        return;

    beginResetModel();
    m_notifications.clear();
    endResetModel();

    emit countChanged();
}
