#pragma once

#include <QAbstractListModel>
#include <QVariantMap>

class NotificationService final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *items READ items CONSTANT)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        TitleRole = Qt::UserRole + 1,
        BodyRole,
        TimeRole
    };
    Q_ENUM(Role)

    explicit NotificationService(QObject *parent = nullptr);

    QAbstractItemModel *items();
    int count() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void push(const QString &title, const QString &body);
    Q_INVOKABLE void clear();

signals:
    void countChanged();
    void pushed(const QString &title, const QString &body);

private:
    QList<QVariantMap> m_notifications;
};
