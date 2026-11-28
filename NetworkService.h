#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

class QDBusServiceWatcher;

class NetworkService final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(bool wifiEnabled READ wifiEnabled NOTIFY wifiEnabledChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString activeSsid READ activeSsid NOTIFY activeSsidChanged)
    Q_PROPERTY(QVariantList accessPoints READ accessPoints NOTIFY accessPointsChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit NetworkService(QObject *parent = nullptr);

    bool available() const { return m_available; }
    bool wifiEnabled() const { return m_wifiEnabled; }
    bool scanning() const { return m_scanning; }
    QString status() const { return m_status; }
    QString activeSsid() const { return m_activeSsid; }
    QVariantList accessPoints() const { return m_accessPoints; }
    QString lastError() const { return m_lastError; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setWifiEnabled(bool enabled);
    Q_INVOKABLE void requestScan();
    Q_INVOKABLE void connectToNetwork(const QString &ssid, const QString &password = QString());
    Q_INVOKABLE void disconnectFromNetwork();

signals:
    void availableChanged();
    void wifiEnabledChanged();
    void scanningChanged();
    void statusChanged();
    void activeSsidChanged();
    void accessPointsChanged();
    void lastErrorChanged();
    void operationFinished(bool success, const QString &message);

private:
    void setError(const QString &message);
    void setScanning(bool scanning);
    void setStatus(const QString &status);
    void updateActiveConnection();
    void updateAccessPoints();

    bool m_available = false;
    bool m_wifiEnabled = false;
    bool m_scanning = false;
    QString m_status = QStringLiteral("Unavailable");
    QString m_activeSsid;
    QString m_lastError;
    QVariantList m_accessPoints;
    QDBusServiceWatcher *m_watcher = nullptr;
};
