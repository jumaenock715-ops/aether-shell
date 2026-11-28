#pragma once
#include <QObject>
#include <QVariantList>
#include <QStringList>

class AudioService final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(int volume READ volume NOTIFY changed)
    Q_PROPERTY(bool muted READ muted NOTIFY changed)
    Q_PROPERTY(int inputVolume READ inputVolume NOTIFY changed)
    Q_PROPERTY(bool inputMuted READ inputMuted NOTIFY changed)
    Q_PROPERTY(QVariantList outputs READ outputs NOTIFY changed)
    Q_PROPERTY(QVariantList inputs READ inputs NOTIFY changed)
    Q_PROPERTY(QString selectedOutput READ selectedOutput NOTIFY changed)
    Q_PROPERTY(QString selectedInput READ selectedInput NOTIFY changed)
public:
    explicit AudioService(QObject *parent = nullptr);
    bool available() const { return m_available; }
    int volume() const { return m_volume; }
    bool muted() const { return m_muted; }
    int inputVolume() const { return m_inputVolume; }
    bool inputMuted() const { return m_inputMuted; }
    QVariantList outputs() const { return m_outputs; }
    QVariantList inputs() const { return m_inputs; }
    QString selectedOutput() const { return m_selectedOutput; }
    QString selectedInput() const { return m_selectedInput; }
    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool setVolume(int percent);
    Q_INVOKABLE bool setMuted(bool muted);
    Q_INVOKABLE bool setInputVolume(int percent);
    Q_INVOKABLE bool setInputMuted(bool muted);
    Q_INVOKABLE bool selectOutput(const QString &name);
    Q_INVOKABLE bool selectInput(const QString &name);
signals:
    void changed();
    void operationFailed(const QString &operation, const QString &reason);
private:
    bool run(const QString &program, const QStringList &arguments, QString *output = nullptr) const;
    void refreshDevices();
    bool m_available = false;
    int m_volume = 0;
    bool m_muted = false;
    int m_inputVolume = 0;
    bool m_inputMuted = false;
    QVariantList m_outputs;
    QVariantList m_inputs;
    QString m_selectedOutput;
    QString m_selectedInput;
};
