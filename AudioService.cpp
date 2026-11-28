#include "AudioService.h"
#include <QProcess>
#include <QRegularExpression>

AudioService::AudioService(QObject *parent) : QObject(parent) { refresh(); }

bool AudioService::run(const QString &program, const QStringList &arguments, QString *output) const
{
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForStarted(1200) || !process.waitForFinished(2500))
        return false;
    if (output) *output = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

void AudioService::refreshDevices()
{
    m_outputs.clear(); m_inputs.clear();
    QString result;
    if (run(QStringLiteral("pactl"), {QStringLiteral("list"), QStringLiteral("short"), QStringLiteral("sinks")}, &result)) {
        const auto lines = result.split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            const auto fields = line.split('\t');
            if (fields.size() >= 2)
                m_outputs.append(QVariantMap{{QStringLiteral("id"), fields.at(0)},
                                             {QStringLiteral("name"), fields.at(1)}});
        }
    }
    if (run(QStringLiteral("pactl"), {QStringLiteral("list"), QStringLiteral("short"), QStringLiteral("sources")}, &result)) {
        const auto lines = result.split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
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

void AudioService::refresh()
{
    QString output;
    m_available = false;
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
    if (!ok) { emit operationFailed(QStringLiteral("set-mute"), QStringLiteral("Neither wpctl nor pactl could change the default output mute state.")); return false; }
    refresh(); return true;
}

bool AudioService::setInputVolume(int percent)
{
    percent = qBound(0, percent, 100);
    const QString value = QString::number(percent / 100.0, 'f', 2);
    bool ok = run(QStringLiteral("wpctl"), {QStringLiteral("set-volume"), QStringLiteral("@DEFAULT_AUDIO_SOURCE@"), value});
    if (!ok) ok = run(QStringLiteral("pactl"), {QStringLiteral("set-source-volume"), QStringLiteral("@DEFAULT_SOURCE@"), QString::number(percent) + QStringLiteral("%")});
    if (!ok) { emit operationFailed(QStringLiteral("set-input-volume"), QStringLiteral("Could not change the default microphone level.")); return false; }
    refresh(); return true;
}

bool AudioService::setInputMuted(bool muted)
{
    const QString state = muted ? QStringLiteral("1") : QStringLiteral("0");
    bool ok = run(QStringLiteral("wpctl"), {QStringLiteral("set-mute"), QStringLiteral("@DEFAULT_AUDIO_SOURCE@"), state});
    if (!ok) ok = run(QStringLiteral("pactl"), {QStringLiteral("set-source-mute"), QStringLiteral("@DEFAULT_SOURCE@"), state});
    if (!ok) { emit operationFailed(QStringLiteral("set-input-mute"), QStringLiteral("Could not change the default microphone mute state.")); return false; }
    refresh(); return true;
}

bool AudioService::selectOutput(const QString &name)
{
    if (name.isEmpty() || !run(QStringLiteral("pactl"), {QStringLiteral("set-default-sink"), name})) {
        emit operationFailed(QStringLiteral("select-output"), QStringLiteral("Could not select that output device. PipeWire/PulseAudio control via pactl is required."));
        return false;
    }
    refresh(); return true;
}

bool AudioService::selectInput(const QString &name)
{
    if (name.isEmpty() || !run(QStringLiteral("pactl"), {QStringLiteral("set-default-source"), name})) {
        emit operationFailed(QStringLiteral("select-input"), QStringLiteral("Could not select that input device. PipeWire/PulseAudio control via pactl is required."));
        return false;
    }
    refresh(); return true;
}
