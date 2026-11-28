#pragma once
#include <QAbstractListModel>
#include <QClipboard>
#include <QPointer>

class ClipboardService final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString currentText READ currentText NOTIFY currentTextChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
public:
    enum Role { TextRole = Qt::UserRole + 1, TimeRole };
    Q_ENUM(Role)
    explicit ClipboardService(QObject *parent = nullptr);
    QString currentText() const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE void copyText(const QString &text);
    Q_INVOKABLE void restore(int row);
    Q_INVOKABLE void clearHistory();
signals:
    void currentTextChanged();
    void countChanged();
    void clipboardChanged(const QString &text);
private slots:
    void captureClipboard();
private:
    struct Entry { QString text; QString time; };
    QList<Entry> m_history;
    QPointer<QClipboard> m_clipboard;
    static constexpr int MaxEntries = 30;
};
