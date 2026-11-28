#include "AudioService.h"
#include <QProcess>
#include <QRegularExpression>
#include <QVariantMap>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>
#include <QStandardPaths>

AudioService::AudioService(QObject *parent) : QObject(parent)
{
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(10000); // fallback for servers without pactl subscribe
    connect(m_refreshTimer, &QTimer::timeout, this, &AudioService::refresh);
    m_refreshTimer->start();
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(350);
    connect(m_debounceTimer, &QTimer::timeout, this, &AudioService::refresh);
    startDeviceMonitor();
    refresh();
}

bool AudioService::run(const QString &program, const QStringList &arguments, QString *output) const
{
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForStarted(1200) || !process.waitForFinished(2500)) {
        process.kill();
        process.waitForFinished(200);
        return false;
    }
    if (output) *output = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

void AudioService::startDeviceMonitor()
{
    const QString pactl = QStandardPaths::findExecutable(QStringLiteral("pactl"));
    if (pactl.isEmpty()) return;
    m_subscription = new QProcess(this);
    connect(m_subscription, &QProcess::readyReadStandardOutput, this, [this]() {
        const QByteArray events = m_subscription->readAllStandardOutput();
        if (events.contains(" on sink") || events.contains(" on source") ||
            events.contains(" on server") || events.contains(" on sink-input"))
            m_debounceTimer->start();
    });
    connect(m_subscription, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        if (m_subscription && m_subscription->state() == QProcess::NotRunning)
            m_subscription->deleteLater();
        m_subscription = nullptr;
    });
    m_subscription->start(pactl, {QStringLiteral("subscribe")});
}

void AudioService::refreshDevices()
{
    m_outputs.clear(); m_inputs.clear();
    QString result;
    if (run(QStringLiteral("pactl"), {QStringLiteral("list"), QStringLiteral("short"), QStringLiteral("sinks")}, &result)) {
        for (const QString &line : result.split('\n', Qt::SkipEmptyParts)) {
            const auto fields = line.split('\t');
            if (fields.size() >= 2)
                m_outputs.append(QVariantMap{{QStringLiteral("id"), fields.at(0)},
                                             {QStringLiteral("name"), fields.at(1)}});
        }
    }
    if (run(QStringLiteral("pactl"), {QStringLiteral("list"), QStringLiteral("short"), QStringLiteral("sources")}, &result)) {
        for (const QString &line : result.split('\n', Qt::SkipEmptyParts)) {
            const auto fields = line.split('\t');
            if (fields.size() >= 2 && !fields.at(1).endsWith(QStringLiteral(".monitor")))
                m_inputs.append(QVariantMap{{QStringLiteral("id"), fields.at(0)},
                                            {QStringLiteral("name"), fields.at(1)}});
        }
    }
    if (run(QStringLiteral("pactl"), {QStringLiteral("get-default-sink")}, &result))
        m_selectedOutput = result;
    if (run(QStringLiteral("pactl"), {QStringLiteral("get-default-source")}, &result))
        m_selectedInput = result;
}

void AudioService::refreshStreams()
{
    m_streams.clear();
    QString json;
    if (!run(QStringLiteral("pactl"), {QStringLiteral("-f"), QStringLiteral("json"),
                                      QStringLiteral("list"), QStringLiteral("sink-inputs")}, &json))
        return;
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !document.isArray()) return;
    const QRegularExpression percentPattern(QStringLiteral("(\\d+(?:\\.\\d+)?)%"));
    for (const QJsonValue &value : document.array()) {
        const QJsonObject stream = value.toObject();
        const QJsonObject properties = stream.value(QStringLiteral("properties")).toObject();
        const QString appName = properties.value(QStringLiteral("application.name")).toString();
        const QString mediaName = properties.value(QStringLiteral("media.name")).toString();
        const QString binary = properties.value(QStringLiteral("application.process.binary")).toString();
        const QString name = !appName.isEmpty() ? appName :
                             (!mediaName.isEmpty() ? mediaName : QStringLiteral("Audio stream"));
        const QJsonObject volume = stream.value(QStringLiteral("volume")).toObject();
        QString volumeText;
        const QJsonObject channel = volume.value(QStringLiteral("front-left")).toObject();
        volumeText = channel.value(QStringLiteral("value_percent")).toString();
        const auto match = percentPattern.match(volumeText);
        const int percent = match.hasMatch() ? qBound(0, qRound(match.captured(1).toDouble()), 150) : 100;
        const uint id = stream.value(QStringLiteral("index")).toVariant().toUInt();
        if (!id) continue;
        const bool android = binary.contains(QStringLiteral("waydroid"), Qt::CaseInsensitive) ||
                             appName.contains(QStringLiteral("waydroid"), Qt::CaseInsensitive);
        m_streams.append(QVariantMap{
            {QStringLiteral("id"), id}, {QStringLiteral("name"), name},
            {QStringLiteral("binary"), binary}, {QStringLiteral("volume"), percent},
            {QStringLiteral("muted"), stream.value(QStringLiteral("mute")).toBool()},
            {QStringLiteral("android"), android},
            {QStringLiteral("provider"), android ? QStringLiteral("Waydroid audio session")
                                                  : QStringLiteral("Linux application")}
        });
    }
}

void AudioService::refresh()
{
    QString output;
    m_available = false;
    m_inputVolume = 0;
    m_inputMuted = false;
    if (run(QStringLiteral("wpctl"), {QStringLiteral("get-volume"), QStringLiteral("@DEFAULT_AUDIO_SINK@")}, &output)) {
        const auto match = QRegularExpression(QStringLiteral("Volume:\\s*([0-9.]+)(?:\\s+\\[MUTED\\])?")).match(output);
        if (match.hasMatch()) {
            m_volume = qBound(0, qRound(match.captured(1).toDouble() * 100.0), 100);
            m_muted = output.contains(QStringLiteral("[MUTED]"));
            m_available = true;
        }
    } else if (run(QStringLiteral("pactl"), {QStringLiteral("get-sink-volume"), QStringLiteral("@DEFAULT_SINK@")}, &output)) {
        const auto match = QRegularExpression(QStringLiteral("(\\d+)%")).match(output);
        if (match.hasMatch()) { m_volume = qBound(0, match.captured(1).toInt(), 100); m_available = true; }
        if (run(QStringLiteral("pactl"), {QStringLiteral("get-sink-mute"), QStringLiteral("@DEFAULT_SINK@")}, &output))
            m_muted = output.contains(QStringLiteral("yes"));
    }
    if (run(QStringLiteral("wpctl"), {QStringLiteral("get-volume"), QStringLiteral("@DEFAULT_AUDIO_SOURCE@")}, &output)) {
        const auto inputMatch = QRegularExpression(QStringLiteral("Volume:\\s*([0-9.]+)")).match(output);
        if (inputMatch.hasMatch()) m_inputVolume = qBound(0, qRound(inputMatch.captured(1).toDouble() * 100.0), 100);
        m_inputMuted = output.contains(QStringLiteral("[MUTED]"));
    } else if (run(QStringLiteral("pactl"), {QStringLiteral("get-source-volume"), QStringLiteral("@DEFAULT_SOURCE@")}, &output)) {
        const auto inputMatch = QRegularExpression(QStringLiteral("(\\d+)%")).match(output);
        if (inputMatch.hasMatch()) m_inputVolume = qBound(0, inputMatch.captured(1).toInt(), 100);
        if (run(QStringLiteral("pactl"), {QStringLiteral("get-source-mute"), QStringLiteral("@DEFAULT_SOURCE@")}, &output))
            m_inputMuted = output.contains(QStringLiteral("yes"));
    }
    refreshDevices();
    refreshStreams();
    emit changed();
}

bool AudioService::setVolume(int percent)
{
    percent = qBound(0, percent, 100);
    const QString value = QString::number(percent / 100.0, 'f', 2);
    bool ok = run(QStringLiteral("wpctl"), {QStringLiteral("set-volume"), QStringLiteral("@DEFAULT_AUDIO_SINK@"), value});
    if (!ok) ok = run(QStringLiteral("pactl"), {QStringLiteral("set-sink-volume"), QStringLiteral("@DEFAULT_SINK@"), QString::number(percent) + QStringLiteral("%")});
    if (!ok) { emit operationFailed(QStringLiteral("set-volume"), QStringLiteral("Neither wpctl nor pactl could change the default output volume.")); return false; }
    refresh(); return true;
}
bool AudioService::setMuted(bool muted)
{
    const QString state = muted ? QStringLiteral("1") : QStringLiteral("0");
    bool ok = run(QStringLiteral("wpctl"), {QStringLiteral("set-mute"), QStringLiteral("@DEFAULT_AUDIO_SINK@"), state});
    if (!ok) ok = run(QStringLiteral("pactl"), {QStringLiteral("set-sink-mute"), QStringLiteral("@DEFAULT_SINK@"), state});
    if (!ok) { emit operationFailed(QStringLiteral("set-mute"), QStringLiteral("Could not change output mute state.")); return false; }
    refresh(); return true;
}
bool AudioService::setInputVolume(int percent)
{
    percent = qBound(0, percent, 100);
    const QString value = QString::number(percent / 100.0, 'f', 2);
    bool ok = run(QStringLiteral("wpctl"), {QStringLiteral("set-volume"), QStringLiteral("@DEFAULT_AUDIO_SOURCE@"), value});
    if (!ok) ok = run(QStringLiteral("pactl"), {QStringLiteral("set-source-volume"), QStringLiteral("@DEFAULT_SOURCE@"), QString::number(percent) + QStringLiteral("%")});
    if (!ok) { emit operationFailed(QStringLiteral("set-input-volume"), QStringLiteral("Could not change microphone level.")); return false; }
    refresh(); return true;
}
bool AudioService::setInputMuted(bool muted)
{
    const QString state = muted ? QStringLiteral("1") : QStringLiteral("0");
    bool ok = run(QStringLiteral("wpctl"), {QStringLiteral("set-mute"), QStringLiteral("@DEFAULT_AUDIO_SOURCE@"), state});
    if (!ok) ok = run(QStringLiteral("pactl"), {QStringLiteral("set-source-mute"), QStringLiteral("@DEFAULT_SOURCE@"), state});
    if (!ok) { emit operationFailed(QStringLiteral("set-input-mute"), QStringLiteral("Could not change microphone mute state.")); return false; }
    refresh(); return true;
}
bool AudioService::selectOutput(const QString &name)
{
    if (name.isEmpty() || !run(QStringLiteral("pactl"), {QStringLiteral("set-default-sink"), name})) {
        emit operationFailed(QStringLiteral("select-output"), QStringLiteral("Could not select that output device.")); return false;
    }
    refresh(); return true;
}
bool AudioService::selectInput(const QString &name)
{
    if (name.isEmpty() || !run(QStringLiteral("pactl"), {QStringLiteral("set-default-source"), name})) {
        emit operationFailed(QStringLiteral("select-input"), QStringLiteral("Could not select that input device.")); return false;
    }
    refresh(); return true;
}
bool AudioService::setStreamVolume(uint streamId, int percent)
{
    percent = qBound(0, percent, 150);
    if (!run(QStringLiteral("pactl"), {QStringLiteral("set-sink-input-volume"),
            QString::number(streamId), QString::number(percent) + QStringLiteral("%")})) {
        emit operationFailed(QStringLiteral("set-stream-volume"), QStringLiteral("The audio stream disappeared or could not be controlled.")); return false;
    }
    refresh(); return true;
}
bool AudioService::setStreamMuted(uint streamId, bool muted)
{
    if (!run(QStringLiteral("pactl"), {QStringLiteral("set-sink-input-mute"),
            QString::number(streamId), muted ? QStringLiteral("1") : QStringLiteral("0")})) {
        emit operationFailed(QStringLiteral("set-stream-mute"), QStringLiteral("The audio stream disappeared or could not be controlled.")); return false;
    }
    refresh(); return true;
}
