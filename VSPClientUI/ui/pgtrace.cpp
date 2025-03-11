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

PGTrace::PGTrace(QWidget* parent)
    : VSPAbstractPage(parent)
    , ui(new Ui::PGTrace)
{
    ui->setupUi(this);

    connectButton(ui->btnUpdate);
}

PGTrace::~PGTrace()
{
    delete ui;
}

void PGTrace::onActionExecute()
{
    quint32 flags = 0;
    if (ui->cbxTraceRX->isChecked())
        flags |= 0x1;
    if (ui->cbxTraceTX->isChecked())
        flags |= 0x2;
    if (ui->cbxTraceCmd->isChecked())
        flags |= 0x4;

    VSPDataModel::TPortItem port = {};
    if (ui->cbPorts->currentIndex() >= 0) {
        port = ui->cbPorts->currentData().value<VSPDataModel::TPortItem>();
    }
    quint64 params = (flags << 16) | port.id;

    emit VSPAbstractPage::execute(vspControlEnableChecks, QVariant::fromValue(params));
}

void PGTrace::update(TVSPControlCommand command, VSPPortListModel* portModel, VSPLinkListModel* linkModel)
{
    const QIcon icon1(":/vspclient_1");

    Q_UNUSED(command);
    Q_UNUSED(linkModel);

    ui->cbPorts->clear();
    for (int i = 0; i < portModel->rowCount(); i++) {
        VSPDataModel::TDataRecord r = portModel->at(i).value<VSPDataModel::TDataRecord>();
        ui->cbPorts->addItem(icon1, r.port.name, QVariant::fromValue(r.port));
    }

    bool enab = ui->cbPorts->count() > 0;
    ui->cbPorts->setEnabled(enab);
    ui->btnUpdate->setEnabled(enab);
}
