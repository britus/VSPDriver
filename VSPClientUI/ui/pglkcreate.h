// ********************************************************************
// pglkcreate.h - Create a port link
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#pragma once

#include "ui_pglkcreate.h"
#include <QPushButton>
#include <QWidget>
#include <vspabstractpage.h>
#include <vspdatamodel.h>
#include <vspdriverclient.h>

namespace Ui {
class PGLKCreate;
}

class PGLKCreate: public VSPAbstractPage
{
    Q_OBJECT

public:
    explicit PGLKCreate(QWidget* parent = nullptr);
    ~PGLKCreate();

    void update(
       TVSPControlCommand command,  //
       VSPPortListModel* portModel, //
       VSPLinkListModel* linkModel) override;

    VSPDataModel::TPortItem selection1() const;
    VSPDataModel::TPortItem selection2() const;

protected:
    void onActionExecute() override;

private:
    Ui::PGLKCreate* ui;
};
