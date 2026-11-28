#pragma once

#include <QObject>
#include "NetworkService.h"

class SystemBridge final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(NetworkService *network READ network CONSTANT)

public:
    explicit SystemBridge(QObject *parent = nullptr);
    NetworkService *network() { return &m_network; }

private:
    NetworkService m_network;
};
