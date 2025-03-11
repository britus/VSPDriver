// ********************************************************************
// pglkremove.cpp - Remove selected port link
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#include "ui_pglkremove.h"
#include <pglkremove.h>
#include <vspabstractpage.h>

PGLKRemove::PGLKRemove(QWidget* parent)
    : VSPAbstractPage(parent)
    , ui(new Ui::PGLKRemove)
{
    ui->setupUi(this);

    connectButton(ui->btnUnlink);
}

PGLKRemove::~PGLKRemove()
{
    delete ui;
}

void PGLKRemove::onActionExecute()
{
    const VSPDataModel::TPortLink l = selection();
    emit execute(vspControlUnlinkPorts, QVariant::fromValue(l));
}

void PGLKRemove::update(TVSPControlCommand command, VSPPortListModel* portModel, VSPLinkListModel* linkModel)
{
    const QIcon icon1(":/vspclient_1");

    Q_UNUSED(command);
    Q_UNUSED(portModel);

    ui->comboBox->clear();

    for (int i = 0; i < linkModel->rowCount(); i++) {
        VSPDataModel::TDataRecord r = //
           linkModel->at(i).value<VSPDataModel::TDataRecord>();
        ui->comboBox->addItem(icon1, r.link.name, QVariant::fromValue(r.link));
    }

    ui->comboBox->setEnabled(ui->comboBox->count() > 0);
    ui->btnUnlink->setEnabled(ui->comboBox->count() > 0);
}

VSPDataModel::TPortLink PGLKRemove::selection() const
{
    const QVariant v = ui->comboBox->currentData();
    if (v.isNull() || !v.isValid()) {
        return {};
    }
    return v.value<VSPDataModel::TPortLink>();
}
