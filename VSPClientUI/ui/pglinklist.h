// ********************************************************************
// pglinklist.h - Shows active port links
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
class PGLinkList;
}

class PGLinkList: public VSPAbstractPage
{
    Q_OBJECT

public:
    explicit PGLinkList(QWidget* parent = nullptr);
    ~PGLinkList();

    void update(TVSPControlCommand command, VSPPortListModel* portModel, VSPLinkListModel* linkModel) override;

protected:
    void onActionExecute() override;

private:
    Ui::PGLinkList* ui;
};
