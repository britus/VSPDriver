// ********************************************************************
// pgspremove.h - Remove active serial port instance
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#pragma once

#include <QPushButton>
#include <QWidget>
#include <vspabstractpage.h>
#include <vspdatamodel.h>
#include <vspdriverclient.h>

namespace Ui {
class PGSPRemove;
}

class PGSPRemove: public VSPAbstractPage
{
    Q_OBJECT

public:
    explicit PGSPRemove(QWidget* parent = nullptr);
    ~PGSPRemove();

    void update(TVSPControlCommand command, VSPPortListModel* portModel, VSPLinkListModel* linkModel) override;

    VSPDataModel::TPortItem selection() const;

protected:
    void onActionExecute() override;

private:
    Ui::PGSPRemove* ui;
};
