#pragma once
#include <QAbstractListModel>
#include <QClipboard>
#include <QPointer>
#include <QImage>
#include <QUrl>

class QMimeData;

class ClipboardService final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString currentText READ currentText NOTIFY currentTextChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    enum Role { TextRole = Qt::UserRole + 1, TimeRole, KindRole, PreviewRole, ImageUrlRole, UrlsRole };
    Q_ENUM(Role)
    explicit ClipboardService(QObject *parent = nullptr);
    QString currentText() const;
    int count() const { return m_history.size(); }
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE void copyText(const QString &text);
    Q_INVOKABLE void copyUrls(const QStringList &urls);
    Q_INVOKABLE void restore(int row);
    Q_INVOKABLE void clearHistory();
signals:
    void currentTextChanged();
    void countChanged();
    void clipboardChanged(const QString &text);
private slots:
    void captureClipboard();
private:
    struct Entry {
        QString text;
        QString time;
        QString kind;
        QString imagePath;
        QImage image;
        QList<QUrl> urls;
    };
    QList<Entry> m_history;
    QPointer<QClipboard> m_clipboard;
    QString m_cacheDirectory;
    static constexpr int MaxEntries = 30;
};
