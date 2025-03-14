// ********************************************************************
// pglinklist.cpp - Shows active port links
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#include "ui_pglinklist.h"
#include <QColumnView>
#include <QSettings>
#include <QTableView>
#include <QTimer>
#include <pglinklist.h>
#include <vscmainwindow.h>
#include <vspabstractpage.h>
#include <vspsession.h>

PGLinkList::PGLinkList(QWidget* parent)
    : VSPAbstractPage(parent)
    , ui(new Ui::PGLinkList)
    , m_colWidths {20, 260, 110, 110}
{
    ui->setupUi(this);

    connectButton(ui->btnRefresh);

    QHeaderView* hv = ui->tableView->horizontalHeader();
    connect(hv, &QHeaderView::sectionResized, this, &PGLinkList::onSectionResized);
}

PGLinkList::~PGLinkList()
{
    delete ui;
}

void PGLinkList::onSectionResized(int logicalIndex, int, int newSize)
{
    if (logicalIndex >= 0 && logicalIndex < 4) {
        m_colWidths[logicalIndex] = newSize;
    }
}

void PGLinkList::onActionExecute()
{
    emit execute(TVSPControlCommand::vspControlGetLinkList, {});
}

void PGLinkList::loadSettings(QSettings* settings)
{
    settings->beginGroup(this->objectName());
    for (int i = 0; i < 4; i++) {
        m_colWidths[i] = settings->value(QStringLiteral("col_%1").arg(i), m_colWidths[i]).toUInt();
    }
    settings->endGroup();
}

void PGLinkList::saveSettings(QSettings* settings)
{
    settings->beginGroup(this->objectName());
    for (int i = 0; i < 4; i++) {
        settings->setValue(QStringLiteral("col_%1").arg(i), m_colWidths[i]);
    }
    settings->endGroup();
}

void PGLinkList::update(TVSPControlCommand command, VSPPortListModel* portModel, VSPLinkListModel* linkModel)
{
    Q_UNUSED(command);
    Q_UNUSED(portModel);

    ui->tableView->setModel(linkModel);

    QTimer::singleShot(20, this, [this, linkModel]() {
        for (int i = 0; i < linkModel->columnCount(); i++) {
            ui->tableView->setColumnWidth(i, m_colWidths[i]);
        }
    });
}
