// ********************************************************************
// pgportlist.cpp - Shows active ports
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#include "ui_pgportlist.h"
#include <pgportlist.h>
#include <vspabstractpage.h>

PGPortList::PGPortList(QWidget* parent)
    : VSPAbstractPage(parent)
    , ui(new Ui::PGPortList)
{
    ui->setupUi(this);

    connectButton(ui->btnRefresh);
}

PGPortList::~PGPortList()
{
    delete ui;
}

void PGPortList::onActionExecute()
{
    emit execute(vspControlGetPortList, {});
}

void PGPortList::update(TVSPControlCommand command, VSPPortListModel* portModel, VSPLinkListModel* linkModel)
{
    Q_UNUSED(command);
    Q_UNUSED(linkModel);

    ui->tableView->setModel(portModel);
}
