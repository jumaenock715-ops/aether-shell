#include "SystemBridge.h"

SystemBridge::SystemBridge(QObject *parent)
    : QObject(parent), m_network(this), m_hardware(this)
{
}
