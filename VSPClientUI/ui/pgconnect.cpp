// ********************************************************************
// pgconnect.cpp - Connect VSPDriver user client interface
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#include "ui_pgconnect.h"
#include <pgconnect.h>
#include <vspabstractpage.h>

PGConnect::PGConnect(QWidget* parent)
    : VSPAbstractPage(parent)
    , ui(new Ui::PGConnect)
{
    ui->setupUi(this);

    connectButton(ui->btnDemo);

    connect(ui->btnInstall, &QPushButton::clicked, this, [this]() {
        emit installDriver();
    });
}

PGConnect::~PGConnect()
{
    delete ui;
}

void PGConnect::onActionExecute()
{
    emit VSPAbstractPage::execute(vspControlPingPong, {1});
}

void PGConnect::update(TVSPControlCommand command, VSPPortListModel* portModel, VSPLinkListModel* linkModel)
{
    Q_UNUSED(command);
    Q_UNUSED(portModel);
    Q_UNUSED(linkModel);
}
