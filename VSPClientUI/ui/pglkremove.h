// ********************************************************************
// pglkremove.h - Remove selected port link
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
class PGLKRemove;
}

class PGLKRemove: public VSPAbstractPage
{
    Q_OBJECT

public:
    explicit PGLKRemove(QWidget* parent = nullptr);
    ~PGLKRemove();

    void update(TVSPControlCommand command, VSPPortListModel* portModel, VSPLinkListModel* linkModel) override;

    VSPDataModel::TPortLink selection() const;

protected:
    void onActionExecute() override;

private:
    Ui::PGLKRemove* ui;
};
