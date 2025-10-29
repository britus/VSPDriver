// ********************************************************************
// VSPClient native OS Bridge
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: GPLv3
// ********************************************************************
#include "VSPControllerBridge.h"
#include "VSPController.h"
#include <IOKit/IOBSD.h>
#include <IOKit/IOTypes.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>

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

/* Copied from DriverKitSerial header */
#define PD_RS232_PARITY_DEFAULT    0    // Valid only for RX, means follow TX
#define PD_RS232_PARITY_NONE    1    // No Parity bit inserted or expected
#define PD_RS232_PARITY_ODD    2    // Odd Parity bit inserted or expected
#define PD_RS232_PARITY_EVEN    3    // Even Parity bit inserted or expected
#define PD_RS232_PARITY_MARK    4    // Mark inserted or expected
#define PD_RS232_PARITY_SPACE    5    // Space inserted or expected
#define PD_RS232_PARITY_ANY    6    // Valid only for RX, means discard parity

bool CreatePort(uint8_t baudRate, uint8_t dataBits, uint8_t stopBits, uint8_t parity, uint8_t flowCtrl) {
    if (!IsDriverConnected()) {
        return false;
    }
    
    CVSPPortParameters p= {115200, 8, 2, 0, 0};
    p.baudRate = baudRate;
    p.dataBits = dataBits;
    p.flowCtrl = flowCtrl;
    switch (stopBits) {
        case 1:
            p.stopBits = stopBits;
            break;
        case 2:
            p.stopBits = stopBits;
            break;
        default:
            p.stopBits = 1;
            break;
    }
    switch (parity) {
        case 0:
            p.parity = PD_RS232_PARITY_NONE;
            break;
        case 1:
            p.parity = PD_RS232_PARITY_ODD;
            break;
        case 2:
            p.parity = PD_RS232_PARITY_EVEN;
            break;
        case 3:
            p.parity = PD_RS232_PARITY_MARK;
            break;
        case 4:
            p.parity = PD_RS232_PARITY_SPACE;
            break;
        default:
            p.parity = PD_RS232_PARITY_DEFAULT;
            break;
    }

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

bool EnableChecksAndTrace(uint8_t port, uint64_t checkFlags, uint64_t traceFlags) {
    if (!IsDriverConnected()) {
        return false;
    }
    return m_controller->EnableChecksAndTrace(port, checkFlags, traceFlags);
}

} // extern "C"
