// ********************************************************************
// pgspcreate.cpp - Create serial port instance
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#include "ui_pgspcreate.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include <pgspcreate.h>
#include <vspabstractpage.h>

Q_DECLARE_METATYPE(QSerialPortInfo)
Q_DECLARE_METATYPE(QSerialPort::BaudRate)
Q_DECLARE_METATYPE(QSerialPort::DataBits)
Q_DECLARE_METATYPE(QSerialPort::StopBits)
Q_DECLARE_METATYPE(QSerialPort::Parity)

PGSPCreate::PGSPCreate(QWidget* parent)
    : VSPAbstractPage(parent)
    , ui(new Ui::PGSPCreate)
{
    ui->setupUi(this);

    // initComboSerialPort(ui->cbSerialPort, nullptr);
    initComboBaudRate(ui->cbBaud, nullptr);
    initComboDataBits(ui->cbDataBits, nullptr);
    initComboStopBits(ui->cbStopBits, nullptr);
    initComboParity(ui->cbParity, nullptr);
    initComboFlowCtrl(ui->cbFlowControl, nullptr);

    connectButton(ui->btnDoSPCreate);
}

PGSPCreate::~PGSPCreate()
{
    delete ui;
}

/* from MacOS DriverKit
 * #include <SerialDriverKit/SerialDriverKit.h>
 * RS232 Parity stuff, used with the DATA_INTEGRITY event
 */
#define PD_RS232_PARITY_DEFAULT 0 // Valid only for RX, means follow TX
#define PD_RS232_PARITY_NONE    1 // No Parity bit inserted or expected
#define PD_RS232_PARITY_ODD     2 // Odd Parity bit inserted or expected
#define PD_RS232_PARITY_EVEN    3 // Even Parity bit inserted or expected
#define PD_RS232_PARITY_MARK    4 // Mark inserted or expected
#define PD_RS232_PARITY_SPACE   5 // Space inserted or expected
#define PD_RS232_PARITY_ANY     6 // Valid only for RX, means discard parity

void PGSPCreate::onActionExecute()
{
    TVSPPortParameters p = {
       .baudRate = (uint32_t) this->baudRate(),
       .dataBits = (uint8_t) this->dataBits(),
       .stopBits = (uint8_t) this->stopBits(),
       .parity = PD_RS232_PARITY_NONE,
       .flowCtrl = (uint8_t) this->flowCtrl(),
    };

    // translate QT -> SerialDriverKit
    switch (parity()) {
        case QSerialPort::Parity::NoParity: {
            p.parity = PD_RS232_PARITY_NONE;
            break;
        }
        case QSerialPort::Parity::EvenParity: {
            p.parity = PD_RS232_PARITY_EVEN;
            break;
        }
        case QSerialPort::Parity::OddParity: {
            p.parity = PD_RS232_PARITY_ODD;
            break;
        }
        case QSerialPort::Parity::SpaceParity: {
            p.parity = PD_RS232_PARITY_SPACE;
            break;
        }
        case QSerialPort::Parity::MarkParity: {
            p.parity = PD_RS232_PARITY_MARK;
            break;
        }
        default: {
            p.parity = PD_RS232_PARITY_DEFAULT;
            break;
        }
    }

    switch (stopBits()) {
        case QSerialPort::StopBits::OneStop: {
            p.stopBits = 2;
            break;
        }
        case QSerialPort::StopBits::OneAndHalfStop: {
            p.stopBits = 3;
            break;
        }
        case QSerialPort::StopBits::TwoStop: {
            p.stopBits = 4;
            break;
        }
        default: {
            p.stopBits = 2;
            break;
        }
    }

    emit execute(vspControlCreatePort, QVariant::fromValue(p));
}

void PGSPCreate::update(TVSPControlCommand command, VSPPortListModel* portModel, VSPLinkListModel* linkModel)
{
    Q_UNUSED(command);
    Q_UNUSED(portModel);
    Q_UNUSED(linkModel);
}

QSerialPort::BaudRate PGSPCreate::baudRate() const
{
    QVariant v = ui->cbBaud->currentData();
    if (v.isNull() || !v.isValid()) {
        return QSerialPort::BaudRate::Baud115200;
    }
    return v.value<QSerialPort::BaudRate>();
}

QSerialPort::DataBits PGSPCreate::dataBits() const
{
    QVariant v = ui->cbDataBits->currentData();
    if (v.isNull() || !v.isValid()) {
        return QSerialPort::DataBits::Data8;
    }
    return v.value<QSerialPort::DataBits>();
}

QSerialPort::StopBits PGSPCreate::stopBits() const
{
    QVariant v = ui->cbStopBits->currentData();
    if (v.isNull() || !v.isValid()) {
        return QSerialPort::StopBits::OneStop;
    }
    return v.value<QSerialPort::StopBits>();
}

QSerialPort::Parity PGSPCreate::parity() const
{
    QVariant v = ui->cbParity->currentData();
    if (v.isNull() || !v.isValid()) {
        return QSerialPort::Parity::NoParity;
    }
    return v.value<QSerialPort::Parity>();
}

QSerialPort::FlowControl PGSPCreate::flowCtrl() const
{
    QVariant v = ui->cbFlowControl->currentData();
    if (v.isNull() || !v.isValid()) {
        return QSerialPort::FlowControl::NoFlowControl;
    }
    return v.value<QSerialPort::FlowControl>();
}

inline void PGSPCreate::initComboSerialPort(QComboBox* cbx, QComboBox* link)
{
    const QIcon icon1(":/vspclient_1");
    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    const int selection = (!link ? 0 : link->currentIndex());
    const QVariant vsel = (!link ? QVariant() : link->itemData(selection));

    /* fill combo box with serial ports */
    cbx->clear();
    cbx->setCurrentIndex(-1);
    for (int i = 0; i < ports.count(); i++) {
        // <Name>, <QSerialPortInfo>
        cbx->addItem(icon1, ports[i].portName(), QVariant::fromValue(ports[i]));
        if (!vsel.isNull() && vsel.isValid()) {
            if (ports[i].portName().contains(vsel.toString())) {
                cbx->setCurrentIndex(i);
            }
        }
    }
    if (cbx->currentIndex() < 0) {
        cbx->setCurrentIndex(0);
    }
}

inline void PGSPCreate::initComboBaudRate(QComboBox* cbx, QComboBox* link)
{
    const QIcon icon1(":/vspclient_1");

    typedef struct {
        QString name;
        QSerialPort::BaudRate baud;
    } TBaudRates;

    TBaudRates baudRates[8] = {
       {tr("1200"), QSerialPort::Baud1200},
       {tr("2400"), QSerialPort::Baud2400},
       {tr("4800"), QSerialPort::Baud4800},
       {tr("9600"), QSerialPort::Baud9600},
       {tr("19200"), QSerialPort::Baud19200},
       {tr("38400"), QSerialPort::Baud38400},
       {tr("57600"), QSerialPort::Baud57600},
       {tr("115200"), QSerialPort::Baud115200},
    };

    const int selection = (!link ? 0 : link->currentIndex());
    QVariant vsel = (!link ? QVariant() : link->itemData(selection));

    if (vsel.isNull() || !vsel.isValid()) {
        vsel = QVariant::fromValue(baudRates[7].baud);
    }

    cbx->clear();
    for (int i = 0; i < 8; i++) {
        cbx->addItem(icon1, baudRates[i].name, QVariant::fromValue(baudRates[i].baud));
        if (!vsel.isNull() && vsel.isValid()) {
            if (baudRates[i].baud == vsel.value<QSerialPort::BaudRate>()) {
                cbx->setCurrentIndex(i);
            }
        }
    }
}

inline void PGSPCreate::initComboDataBits(QComboBox* cbx, QComboBox* link)
{
    const QIcon icon1(":/vspclient_1");

    typedef struct {
        QString name;
        QSerialPort::DataBits bits;
    } TDataBits;

    TDataBits dataBits[4] = {
       {tr("5 Bits"), QSerialPort::Data5},
       {tr("6 Bits"), QSerialPort::Data6},
       {tr("7 Bits"), QSerialPort::Data7},
       {tr("8 Bits"), QSerialPort::Data8},
    };

    const int selection = (!link ? 0 : link->currentIndex());
    QVariant vsel = (!link ? QVariant() : link->itemData(selection));

    if (vsel.isNull() || !vsel.isValid()) {
        vsel = QVariant::fromValue(dataBits[3].bits);
    }

    cbx->clear();
    for (int i = 0; i < 4; i++) {
        cbx->addItem(icon1, dataBits[i].name, QVariant::fromValue(dataBits[i].bits));
        if (!vsel.isNull() && vsel.isValid()) {
            if (dataBits[i].bits == vsel.value<QSerialPort::DataBits>()) {
                cbx->setCurrentIndex(i);
            }
        }
    }
}

inline void PGSPCreate::initComboStopBits(QComboBox* cbx, QComboBox* link)
{
    const QIcon icon1(":/vspclient_1");

    typedef struct {
        QString name;
        QSerialPort::StopBits bits;
    } TStopBits;

    TStopBits stopBits[3] = {
       {tr("One Stop"), QSerialPort::OneStop},
       {tr("One and Half"), QSerialPort::OneAndHalfStop},
       {tr("Two Stop"), QSerialPort::TwoStop},
    };

    const int selection = (!link ? 0 : link->currentIndex());
    QVariant vsel = (!link ? QVariant() : link->itemData(selection));

    if (vsel.isNull() || !vsel.isValid()) {
        vsel = QVariant::fromValue(stopBits[0].bits);
    }

    cbx->clear();
    for (int i = 0; i < 3; i++) {
        cbx->addItem(icon1, stopBits[i].name, QVariant::fromValue(stopBits[i].bits));
        if (!vsel.isNull() && vsel.isValid()) {
            if (stopBits[i].bits == vsel.value<QSerialPort::StopBits>()) {
                cbx->setCurrentIndex(i);
            }
        }
    }
}

inline void PGSPCreate::initComboParity(QComboBox* cbx, QComboBox* link)
{
    const QIcon icon1(":/vspclient_1");

    typedef struct {
        QString name;
        QSerialPort::Parity parity;
    } TParity;

    TParity parity[5] = {
       {tr("No Parity"), QSerialPort::NoParity},
       {tr("Event Parity"), QSerialPort::EvenParity},
       {tr("Odd Parity"), QSerialPort::OddParity},
       {tr("Space Parity"), QSerialPort::SpaceParity},
       {tr("Mark Parity"), QSerialPort::MarkParity},
    };

    const int selection = (!link ? 0 : link->currentIndex());
    QVariant vsel = (!link ? QVariant() : link->itemData(selection));

    if (vsel.isNull() || !vsel.isValid()) {
        vsel = QVariant::fromValue(parity[0].parity);
    }

    cbx->clear();
    for (int i = 0; i < 5; i++) {
        cbx->addItem(icon1, parity[i].name, QVariant::fromValue(parity[i].parity));
        if (!vsel.isNull() && vsel.isValid()) {
            if (parity[i].parity == vsel.value<QSerialPort::Parity>()) {
                cbx->setCurrentIndex(i);
            }
        }
    }

    /* connect event only once */
    if (cbx->property("init").toBool()) {
        return;
    }
}

inline void PGSPCreate::initComboFlowCtrl(QComboBox* cbx, QComboBox* link)
{
    const QIcon icon1(":/vspclient_1");

    typedef struct {
        QString name;
        QSerialPort::FlowControl flow;
    } TFlowControl;

    TFlowControl flowctrl[3] = {
       {tr("No Flow Control"), QSerialPort::NoFlowControl},
       {tr("Hardware Control"), QSerialPort::HardwareControl},
       {tr("Software Control"), QSerialPort::SoftwareControl},
    };

    const int selection = (!link ? 0 : link->currentIndex());
    QVariant vsel = (!link ? QVariant() : link->itemData(selection));

    if (vsel.isNull() || !vsel.isValid()) {
        vsel = QVariant::fromValue(flowctrl[0].flow);
    }

    cbx->clear();
    for (int i = 0; i < 3; i++) {
        cbx->addItem(icon1, flowctrl[i].name, QVariant::fromValue(flowctrl[i].flow));
        if (!vsel.isNull() && vsel.isValid()) {
            if (flowctrl[i].flow == vsel.value<QSerialPort::FlowControl>()) {
                cbx->setCurrentIndex(i);
            }
        }
    }
}
