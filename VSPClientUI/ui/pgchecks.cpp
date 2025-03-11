// ********************************************************************
// pgchecks.cpp - Enable/Disable port parameter checks
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#include "ui_pgchecks.h"
#include <pgchecks.h>
#include <vspabstractpage.h>

PGChecks::PGChecks(QWidget* parent)
    : VSPAbstractPage(parent)
    , ui(new Ui::PGChecks)
{
    ui->setupUi(this);

    connectButton(ui->btnUpdate);
}

PGChecks::~PGChecks()
{
    delete ui;
}

void PGChecks::onActionExecute()
{
    uint flags = 0;
    if (ui->cbxBaudRate->isChecked())
        flags |= 0x01;
    if (ui->cbxDataBits->isChecked())
        flags |= 0x02;
    if (ui->cbxStopBits->isChecked())
        flags |= 0x04;
    if (ui->cbxParity->isChecked())
        flags |= 0x08;
    if (ui->cbxFlowCtrl->isChecked())
        flags |= 0x10;

    VSPDataModel::TPortItem port = {};
    if (ui->cbPorts->currentIndex() >= 0) {
        port = ui->cbPorts->currentData().value<VSPDataModel::TPortItem>();
    }
    quint64 params = (flags << 16) | port.id;

    emit VSPAbstractPage::execute(vspControlEnableChecks, QVariant::fromValue(params));
}

void PGChecks::update(TVSPControlCommand command, VSPPortListModel* portModel, VSPLinkListModel* linkModel)
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
