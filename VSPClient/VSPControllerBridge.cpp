//
//  VSPControllerBridge.c
//  VSPClient
//
//  Created by Björn Eschrich on 27.10.25.
//

#include "VSPControllerBridge.h"
#include "VSPController.h"

static VSPController* m_controller = NULL;

/* -------------------------------------------------------------------
 * C API Called by Swift code
 * ------------------------------------------------------------------- */

extern "C" {

bool ConnectDriver(void)
{
    if (m_controller == NULL) {
        m_controller =  new VSPController("VSPDriver");
    }
    
    return m_controller->ConnectDriver();
}

void DisconnectDriver(void)
{
    if (m_controller != NULL) {
        delete m_controller;
        m_controller = NULL;
    }
}

bool IsDriverConnected(void) {
    if (m_controller != NULL) {
        return m_controller->IsConnected();
    }
    return false;
}

bool GetStatus(void) {
    if (!IsDriverConnected()) {
        return false;
    }
    return m_controller->GetStatus();
}

bool CreatePort(uint8_t baudRate, uint8_t dataBits, uint8_t stopBits, uint8_t parity, uint8_t flowCtrl) {
    if (!IsDriverConnected()) {
        return false;
    }
    CVSPPortParameters p = { //
        baudRate, dataBits, stopBits, parity, flowCtrl,
    };
    return m_controller->CreatePort(&p);
}

bool RemovePort(uint8_t id) {
    if (!IsDriverConnected()) {
        return false;
    }
    return m_controller->RemovePort(id);
}

bool GetPortList() {
    if (!IsDriverConnected()) {
        return false;
    }
    return m_controller->GetPortList();
}

bool GetLinkList() {
    if (!IsDriverConnected()) {
        return false;
    }
    return m_controller->GetLinkList();
}

bool LinkPorts(uint8_t source, uint8_t target) {
    if (!IsDriverConnected()) {
        return false;
    }
    return m_controller->LinkPorts(source, target);
}

bool UnlinkPorts(uint8_t source, uint8_t target) {
    if (!IsDriverConnected()) {
        return false;
    }
    return m_controller->UnlinkPorts(source, target);
}

bool EnableChecks(uint8_t port, uint64_t flags) {
    if (!IsDriverConnected()) {
        return false;
    }
    return m_controller->EnableChecks(port, flags);
}

bool EnableTrace(uint8_t port, uint64_t flags) {
    if (!IsDriverConnected()) {
        return false;
    }
    return m_controller->EnableTrace(port, flags);
}

} // extern "C"
