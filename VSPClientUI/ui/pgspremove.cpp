// ********************************************************************
// pgspremove.cpp - Remove active serial port instance
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#include "ui_pgspremove.h"
#include <pgspremove.h>
#include <vspabstractpage.h>

PGSPRemove::PGSPRemove(QWidget* parent)
    : VSPAbstractPage(parent)
    , ui(new Ui::PGSPRemove)
{
    ui->setupUi(this);

    connectButton(ui->btnDoSPRemove);
}

PGSPRemove::~PGSPRemove()
{
    delete ui;
}

void PGSPRemove::onActionExecute()
{
    const VSPDataModel::TPortItem p = selection();
    emit VSPAbstractPage::execute(vspControlRemovePort, QVariant::fromValue(p));
}

void PGSPRemove::update(TVSPControlCommand command, VSPPortListModel* portModel, VSPLinkListModel* linkModel)
{
    const QIcon icon1(":/vspclient_1");

    Q_UNUSED(command);
    Q_UNUSED(linkModel);

    ui->cbSerialPorts->clear();
    for (int i = 0; i < portModel->rowCount(); i++) {
        VSPDataModel::TDataRecord r = portModel->at(i).value<VSPDataModel::TDataRecord>();
        ui->cbSerialPorts->addItem(icon1, r.port.name, QVariant::fromValue(r.port));
    }

    bool enab = ui->cbSerialPorts->count() > 0;
    ui->cbSerialPorts->setEnabled(enab);
    ui->btnDoSPRemove->setEnabled(enab);
}

VSPDataModel::TPortItem PGSPRemove::selection() const
{
    const QVariant v = ui->cbSerialPorts->currentData();
    if (v.isNull() || !v.isValid()) {
        return {};
    }
    return v.value<VSPDataModel::TPortItem>();
}
