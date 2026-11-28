#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QByteArray>
#include <QTimer>

class QProcess;

class LinuxHardwareService final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool bluetoothAvailable READ bluetoothAvailable NOTIFY bluetoothChanged)
    Q_PROPERTY(bool bluetoothPowered READ bluetoothPowered NOTIFY bluetoothChanged)
    Q_PROPERTY(bool bluetoothScanning READ bluetoothScanning NOTIFY bluetoothScanningChanged)
    Q_PROPERTY(bool bluetoothPairing READ bluetoothPairing NOTIFY bluetoothPairingChanged)
    Q_PROPERTY(QString bluetoothPairingAddress READ bluetoothPairingAddress NOTIFY bluetoothPairingChanged)
    Q_PROPERTY(QString bluetoothPairingPrompt READ bluetoothPairingPrompt NOTIFY bluetoothPairingChanged)
    Q_PROPERTY(QString bluetoothPairingPasskey READ bluetoothPairingPasskey NOTIFY bluetoothPairingChanged)
    Q_PROPERTY(QStringList bluetoothControllers READ bluetoothControllers NOTIFY bluetoothDevicesChanged)
    Q_PROPERTY(QStringList bluetoothDevices READ bluetoothDevices NOTIFY bluetoothDevicesChanged)
    Q_PROPERTY(QVariantList bluetoothDeviceDetails READ bluetoothDeviceDetails NOTIFY bluetoothDevicesChanged)
    Q_PROPERTY(QStringList batteryDevices READ batteryDevices NOTIFY batteryChanged)
    Q_PROPERTY(int batteryPercent READ batteryPercent NOTIFY batteryChanged)
    Q_PROPERTY(bool batteryPresent READ batteryPresent NOTIFY batteryChanged)
    Q_PROPERTY(QString batteryStatus READ batteryStatus NOTIFY batteryChanged)
    Q_PROPERTY(int batteryHealth READ batteryHealth NOTIFY batteryChanged)
    Q_PROPERTY(int brightness READ brightness NOTIFY brightnessChanged)
    Q_PROPERTY(int volume READ volume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ muted NOTIFY volumeChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit LinuxHardwareService(QObject *parent = nullptr);
    bool bluetoothAvailable() const { return m_bluetoothAvailable; }
    bool bluetoothPowered() const { return m_bluetoothPowered; }
    bool bluetoothScanning() const { return m_bluetoothScanning; }
    bool bluetoothPairing() const { return m_bluetoothPairing; }
    QString bluetoothPairingAddress() const { return m_bluetoothPairingAddress; }
    QString bluetoothPairingPrompt() const { return m_bluetoothPairingPrompt; }
    QString bluetoothPairingPasskey() const { return m_bluetoothPairingPasskey; }
    QStringList bluetoothControllers() const { return m_bluetoothControllers; }
    QStringList bluetoothDevices() const { return m_bluetoothDevices; }
    QVariantList bluetoothDeviceDetails() const { return m_bluetoothDeviceDetails; }
    QStringList batteryDevices() const { return m_batteryDevices; }
    int batteryPercent() const { return m_batteryPercent; }
    bool batteryPresent() const { return m_batteryPresent; }
    QString batteryStatus() const { return m_batteryStatus; }
    int batteryHealth() const { return m_batteryHealth; }
    int brightness() const { return m_brightness; }
    int volume() const { return m_volume; }
    bool muted() const { return m_muted; }
    QString lastError() const { return m_lastError; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setBluetoothPowered(bool enabled);
    Q_INVOKABLE void scanBluetooth();
    Q_INVOKABLE void pairBluetooth(const QString &address);
    Q_INVOKABLE void respondBluetoothPairing(const QString &pin);
    Q_INVOKABLE void confirmBluetoothPairing(bool accepted);
    Q_INVOKABLE void cancelBluetoothPairing();
    Q_INVOKABLE void connectBluetooth(const QString &address);
    Q_INVOKABLE void disconnectBluetooth(const QString &address);
    Q_INVOKABLE void trustBluetooth(const QString &address, bool trusted = true);
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
    void bluetoothScanningChanged();
    void bluetoothPairingChanged();
    void batteryChanged();
    void brightnessChanged();
    void volumeChanged();
    void lastErrorChanged();
    void operationFinished(bool success, const QString &message);

private:
    bool run(const QString &program, const QStringList &arguments,
             const QString &successMessage, int timeoutMs = 8000);
    bool validBluetoothAddress(const QString &address) const;
    void setBluetoothScanning(bool scanning);
    void setBluetoothPairingPrompt(const QString &prompt, const QString &passkey = {});
    void processBluetoothPairingOutput(const QString &output);
    void finishBluetoothPairing(bool success, const QString &message);
    void setError(const QString &message);
    bool m_bluetoothAvailable = false;
    bool m_bluetoothPowered = false;
    bool m_bluetoothScanning = false;
    QProcess *m_bluetoothScanProcess = nullptr;
    QProcess *m_bluetoothPairingProcess = nullptr;
    QTimer *m_bluetoothPairingTimeout = nullptr;
    QByteArray m_bluetoothPairingBuffer;
    bool m_bluetoothPairing = false;
    bool m_bluetoothPairingSucceeded = false;
    QString m_bluetoothPairingAddress;
    QString m_bluetoothPairingPrompt;
    QString m_bluetoothPairingPasskey;
    QStringList m_bluetoothControllers;
    QStringList m_bluetoothDevices;
    QVariantList m_bluetoothDeviceDetails;
    QStringList m_batteryDevices;
    int m_batteryPercent = 0;
    bool m_batteryPresent = false;
    QString m_batteryStatus = QStringLiteral("Unknown");
    int m_batteryHealth = -1;
    int m_brightness = -1;
    int m_volume = -1;
    bool m_muted = false;
    QString m_lastError;
};
