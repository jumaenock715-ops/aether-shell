#pragma once

#include <QObject>
#include "NetworkService.h"
#include "LinuxHardwareService.h"

class SystemBridge final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(NetworkService *network READ network CONSTANT)
    Q_PROPERTY(LinuxHardwareService *hardware READ hardware CONSTANT)

public:
    explicit SystemBridge(QObject *parent = nullptr);
    NetworkService *network() { return &m_network; }
    LinuxHardwareService *hardware() { return &m_hardware; }

private:
    NetworkService m_network;
    LinuxHardwareService m_hardware;
};
