#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>

class LinuxHardwareService final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool bluetoothAvailable READ bluetoothAvailable NOTIFY bluetoothChanged)
    Q_PROPERTY(bool bluetoothPowered READ bluetoothPowered NOTIFY bluetoothChanged)
    Q_PROPERTY(QStringList bluetoothControllers READ bluetoothControllers NOTIFY bluetoothDevicesChanged)
    Q_PROPERTY(QStringList bluetoothDevices READ bluetoothDevices NOTIFY bluetoothDevicesChanged)
    Q_PROPERTY(QVariantList bluetoothDeviceDetails READ bluetoothDeviceDetails NOTIFY bluetoothDevicesChanged)
    Q_PROPERTY(QStringList batteryDevices READ batteryDevices NOTIFY batteryChanged)
    Q_PROPERTY(int batteryPercent READ batteryPercent NOTIFY batteryChanged)
    Q_PROPERTY(bool batteryPresent READ batteryPresent NOTIFY batteryChanged)
    Q_PROPERTY(int brightness READ brightness NOTIFY brightnessChanged)
    Q_PROPERTY(int volume READ volume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ muted NOTIFY volumeChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit LinuxHardwareService(QObject *parent = nullptr);
    bool bluetoothAvailable() const { return m_bluetoothAvailable; }
    bool bluetoothPowered() const { return m_bluetoothPowered; }
    QStringList bluetoothControllers() const { return m_bluetoothControllers; }
    QStringList bluetoothDevices() const { return m_bluetoothDevices; }
    QVariantList bluetoothDeviceDetails() const { return m_bluetoothDeviceDetails; }
    QStringList batteryDevices() const { return m_batteryDevices; }
    int batteryPercent() const { return m_batteryPercent; }
    bool batteryPresent() const { return m_batteryPresent; }
    int brightness() const { return m_brightness; }
    int volume() const { return m_volume; }
    bool muted() const { return m_muted; }
    QString lastError() const { return m_lastError; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setBluetoothPowered(bool enabled);
    Q_INVOKABLE void scanBluetooth();
    Q_INVOKABLE void pairBluetooth(const QString &address);
    Q_INVOKABLE void connectBluetooth(const QString &address);
    Q_INVOKABLE void disconnectBluetooth(const QString &address);
    Q_INVOKABLE void removeBluetooth(const QString &address);
    Q_INVOKABLE void setBrightness(int percent);
    Q_INVOKABLE void setVolume(int percent);
    Q_INVOKABLE void setMuted(bool muted);
    Q_INVOKABLE void suspend();
    Q_INVOKABLE void restart();
    Q_INVOKABLE void shutdown();

signals:
    void bluetoothChanged();
    void bluetoothDevicesChanged();
    void batteryChanged();
    void brightnessChanged();
    void volumeChanged();
    void lastErrorChanged();
    void operationFinished(bool success, const QString &message);

private:
    bool run(const QString &program, const QStringList &arguments,
             const QString &successMessage);
    void setError(const QString &message);
    bool m_bluetoothAvailable = false;
    bool m_bluetoothPowered = false;
    QStringList m_bluetoothControllers;
    QStringList m_bluetoothDevices;
    QVariantList m_bluetoothDeviceDetails;
    QStringList m_batteryDevices;
    int m_batteryPercent = 0;
    bool m_batteryPresent = false;
    int m_brightness = -1;
    int m_volume = -1;
    bool m_muted = false;
    QString m_lastError;
};
