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
#include <vspcontroller.hpp>

PGChecks::PGChecks(QWidget *parent)
    : VSPAbstractPage(parent)
    , ui(new Ui::PGChecks)
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
        ui->cbxBaudRate->setChecked(p.flags & CHECK_BAUD);
        ui->cbxDataBits->setChecked(p.flags & CHECK_DATA_SIZE);
        ui->cbxStopBits->setChecked(p.flags & CHECK_STOP_BITS);
        ui->cbxParity->setChecked(p.flags & CHECK_PARITY);
        ui->cbxFlowCtrl->setChecked(p.flags & CHECK_FLOWCTRL);
        m_lastIndex = index;
    });
}

PGChecks::~PGChecks()
{
    delete ui;
}

void PGChecks::onActionExecute()
{
    quint64 flags = 0;
    if (ui->cbxBaudRate->isChecked())
        flags |= CHECK_BAUD;
    if (ui->cbxDataBits->isChecked())
        flags |= CHECK_DATA_SIZE;
    if (ui->cbxStopBits->isChecked())
        flags |= CHECK_STOP_BITS;
    if (ui->cbxParity->isChecked())
        flags |= CHECK_PARITY;
    if (ui->cbxFlowCtrl->isChecked())
        flags |= CHECK_FLOWCTRL;

    VSPDataModel::TPortItem port = {};
    if (ui->cbPorts->currentIndex() >= 0) {
        port = ui->cbPorts->currentData().value<VSPDataModel::TPortItem>();
        quint64 params = (flags | port.id);
        emit VSPAbstractPage::execute(vspControlEnableChecks, QVariant::fromValue(params));
    }
}

void PGChecks::update(TVSPControlCommand command, VSPPortListModel *portModel, VSPLinkListModel *linkModel)
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
