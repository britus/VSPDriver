// ********************************************************************
// pglinklist.cpp - Shows active port links
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#include "ui_pglinklist.h"
#include <QTimer>
#include <pglinklist.h>
#include <vspabstractpage.h>

PGLinkList::PGLinkList(QWidget* parent)
    : VSPAbstractPage(parent)
    , ui(new Ui::PGLinkList)
{
    ui->setupUi(this);

    connectButton(ui->btnRefresh);
}

PGLinkList::~PGLinkList()
{
    delete ui;
}

void PGLinkList::onActionExecute()
{
    emit execute(TVSPControlCommand::vspControlGetLinkList, {});
}

void PGLinkList::update(TVSPControlCommand command, VSPPortListModel* portModel, VSPLinkListModel* linkModel)
{
    Q_UNUSED(command);
    Q_UNUSED(portModel);

    QTimer::singleShot(50, this, [this, linkModel]() {
        for (int i = 0; i < linkModel->columnCount(); i++) {
            switch (i) {
                case 0: {
                    ui->tableView->setColumnWidth(i, 20);
                    break;
                }
                case 1: {
                    ui->tableView->setColumnWidth(i, 250);
                    break;
                }
                case 2: {
                    ui->tableView->setColumnWidth(i, 100);
                    break;
                }
                case 3: {
                    ui->tableView->setColumnWidth(i, 100);
                    break;
                }
            }
        }
    });

    ui->tableView->setModel(linkModel);
}
