// ********************************************************************
// pgspcreate.h - Create serial port instance
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#pragma once

#include <QComboBox>
#include <QPushButton>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QWidget>
#include <vspabstractpage.h>
#include <vspdatamodel.h>
#include <vspdriverclient.h>

namespace Ui {
class PGSPCreate;
}

class PGSPCreate: public VSPAbstractPage
{
    Q_OBJECT

public:
    explicit PGSPCreate(QWidget* parent = nullptr);
    ~PGSPCreate();

    void update(TVSPControlCommand command, VSPPortListModel* portModel, VSPLinkListModel* linkModel) override;

protected:
    void onActionExecute() override;

private:
    Ui::PGSPCreate* ui;

private:
    inline void initComboSerialPort(QComboBox* cbx, QComboBox* link = nullptr);
    inline void initComboBaudRate(QComboBox* cbx, QComboBox* link = nullptr);
    inline void initComboDataBits(QComboBox* cbx, QComboBox* link = nullptr);
    inline void initComboStopBits(QComboBox* cbx, QComboBox* link = nullptr);
    inline void initComboParity(QComboBox* cbx, QComboBox* link = nullptr);
    inline void initComboFlowCtrl(QComboBox* cbx, QComboBox* link = nullptr);

    inline QSerialPort::BaudRate baudRate() const;
    inline QSerialPort::DataBits dataBits() const;
    inline QSerialPort::StopBits stopBits() const;
    inline QSerialPort::Parity parity() const;
    inline QSerialPort::FlowControl flowCtrl() const;
};
