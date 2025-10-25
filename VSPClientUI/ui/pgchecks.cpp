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
        if (index > -1) {
            QVariant v = ui->cbPorts->itemData(index, Qt::UserRole);
            if (!v.isNull() && v.isValid()) {
                VSPDataModel::TPortItem p;
                p = v.value<VSPDataModel::TPortItem>();
                ui->cbxBaudRate->setChecked(p.flags & CHECK_BAUD);
                ui->cbxDataBits->setChecked(p.flags & CHECK_DATA_SIZE);
                ui->cbxStopBits->setChecked(p.flags & CHECK_STOP_BITS);
                ui->cbxParity->setChecked(p.flags & CHECK_PARITY);
                ui->cbxFlowCtrl->setChecked(p.flags & CHECK_FLOWCTRL);
                m_lastIndex = index;
            }
        }
    });
}

PGChecks::~PGChecks()
{
    delete ui;
}

void PGChecks::onActionExecute()
{
    if (ui->cbPorts->currentIndex() > -1) {
        m_lastIndex = ui->cbPorts->currentIndex();
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
        auto data = ui->cbPorts->currentData();
        if (data.isValid()) {
            VSPDataModel::TPortItem p;
            p = data.value<VSPDataModel::TPortItem>();
            emit VSPAbstractPage::execute( //
                  vspControlEnableChecks, //
                  QVariant::fromValue((flags | p.id)));
        }
    }
}

void PGChecks::update(TVSPControlCommand, VSPPortListModel *portModel, VSPLinkListModel *)
{
    const QIcon icon1(":/vspclient_1");

    ui->cbPorts->clear();
    for (int i = 0; i < portModel->rowCount(); i++) {
        VSPDataModel::TDataRecord r;
        r = portModel->at(i).value<VSPDataModel::TDataRecord>();
        ui->cbPorts->addItem(icon1, r.port.name, QVariant::fromValue(r.port));
    }

    if (ui->cbPorts->count() > 0) {
        ui->cbPorts->setCurrentIndex(m_lastIndex);
        ui->pnlOptions->setEnabled(true);
        ui->cbPorts->setEnabled(true);
        ui->btnUpdate->setEnabled(true);
    }
}
