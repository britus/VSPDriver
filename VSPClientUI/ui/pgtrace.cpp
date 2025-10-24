// ********************************************************************
// pgtrace.cpp - Enable/Disable OS log traces
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#include "ui_pgtrace.h"
#include <pgtrace.h>
#include <vspabstractpage.h>
#include <vspcontroller.hpp>
#include <QComboBox>

PGTrace::PGTrace(QWidget *parent)
    : VSPAbstractPage(parent)
    , ui(new Ui::PGTrace)
    , m_lastIndex(0)
{
    ui->setupUi(this);
    ui->cbPorts->setEnabled(false);
    ui->btnUpdate->setEnabled(false);
    ui->pnlOptions->setEnabled(false);
    connectButton(ui->btnUpdate);
    connect(ui->cbPorts, &QComboBox::currentIndexChanged, this, [this](int index) { //
        QVariant v = ui->cbPorts->itemData(index, Qt::UserRole);
        if (v.isNull() || !v.isValid()) {
            return;
        }
        VSPDataModel::TPortItem p = v.value<VSPDataModel::TPortItem>();
        ui->cbxTraceRX->setChecked(p.flags & TRACE_PORT_RX);
        ui->cbxTraceTX->setChecked(p.flags & TRACE_PORT_TX);
        ui->cbxTraceCmd->setChecked(p.flags & TRACE_PORT_IO);
        m_lastIndex = index;
    });
}

PGTrace::~PGTrace()
{
    delete ui;
}

void PGTrace::onActionExecute()
{
    quint64 flags = 0;
    if (ui->cbxTraceRX->isChecked())
        flags |= TRACE_PORT_RX;
    if (ui->cbxTraceTX->isChecked())
        flags |= TRACE_PORT_TX;
    if (ui->cbxTraceCmd->isChecked())
        flags |= TRACE_PORT_IO;

    VSPDataModel::TPortItem port = {};
    if (ui->cbPorts->currentIndex() >= 0) {
        port = ui->cbPorts->currentData().value<VSPDataModel::TPortItem>();
        quint64 params = (flags | port.id);
        emit VSPAbstractPage::execute(vspControlEnableChecks, QVariant::fromValue(params));
    }
}

void PGTrace::update(TVSPControlCommand command, VSPPortListModel *portModel, VSPLinkListModel *linkModel)
{
    const QIcon icon1(":/vspclient_1");

    Q_UNUSED(command);
    Q_UNUSED(linkModel);

    ui->cbPorts->clear();
    for (int i = 0; i < portModel->rowCount(); i++) {
        VSPDataModel::TDataRecord r = portModel->at(i).value<VSPDataModel::TDataRecord>();
        ui->cbPorts->addItem(icon1, r.port.name, QVariant::fromValue(r.port));
    }

    if (ui->cbPorts->count() > 0) {
        emit ui->cbPorts->currentIndexChanged(m_lastIndex);
        ui->pnlOptions->setEnabled(true);
        ui->cbPorts->setEnabled(true);
        ui->btnUpdate->setEnabled(true);
    }
}
