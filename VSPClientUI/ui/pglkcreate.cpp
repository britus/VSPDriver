// ********************************************************************
// pglkcreate.cpp - Create a port link
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#include "ui_pglkcreate.h"
#include <pglkcreate.h>
#include <vspabstractpage.h>

PGLKCreate::PGLKCreate(QWidget* parent)
    : VSPAbstractPage(parent)
    , ui(new Ui::PGLKCreate)
{
    ui->setupUi(this);

    connectButton(ui->btnLinkPorts);
    ui->txInfo->setVisible(false);
}

PGLKCreate::~PGLKCreate()
{
    delete ui;
}

void PGLKCreate::onActionExecute()
{
    VSPDataModel::TPortLink link = {
       .id = 0,
       .name = "<new>",
       .source = selection1(),
       .target = selection2(),
    };
    emit execute(vspControlLinkPorts, QVariant::fromValue(link));
}

void PGLKCreate::update(TVSPControlCommand command, VSPPortListModel* portModel, VSPLinkListModel* linkModel)
{
    const QIcon icon1(":/vspclient_1");

    Q_UNUSED(command);

    QList<quint8> linkedPorts;

    ui->cbPort1->clear();
    ui->cbPort2->clear();

    // load port IDs already linked
    for (int i = 0; i < linkModel->rowCount(); i++) {
        VSPDataModel::TDataRecord r = linkModel->at(i).value<VSPDataModel::TDataRecord>();
        if (!linkedPorts.contains(r.link.source.id)) {
            linkedPorts.append(r.link.source.id);
        }
        if (!linkedPorts.contains(r.link.target.id)) {
            linkedPorts.append(r.link.target.id);
        }
    }

    // load port IDs into comboboxes and skip already linked ports
    for (int i = 0; i < portModel->rowCount(); i++) {
        VSPDataModel::TDataRecord r = portModel->at(i).value<VSPDataModel::TDataRecord>();
        if (!linkedPorts.contains(r.port.id)) {
            ui->cbPort1->addItem(icon1, r.port.name, QVariant::fromValue(r.port));
            ui->cbPort2->addItem(icon1, r.port.name, QVariant::fromValue(r.port));
        }
    }

    const bool enab =              //
       ui->cbPort1->count() > 0 && //
       ui->cbPort2->count() > 0;
    if (!enab) {
        ui->txInfo->setText(tr("No unlinked ports available. Create new port or unlink others."));
        ui->txInfo->setVisible(true);
    }
    else {
        ui->txInfo->setVisible(false);
    }
    ui->cbPort1->setEnabled(enab);
    ui->cbPort2->setEnabled(enab);
    ui->btnLinkPorts->setEnabled(enab);
}

VSPDataModel::TPortItem PGLKCreate::selection1() const
{
    const QVariant v = ui->cbPort1->currentData();
    if (v.isNull() || !v.isValid()) {
        return {};
    }
    return v.value<VSPDataModel::TPortItem>();
}

VSPDataModel::TPortItem PGLKCreate::selection2() const
{
    const QVariant v = ui->cbPort2->currentData();
    if (v.isNull() || !v.isValid()) {
        return {};
    }
    return v.value<VSPDataModel::TPortItem>();
}
