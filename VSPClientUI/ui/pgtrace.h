// ********************************************************************
// pgtrace.h - Enable/Disable OS log traces
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
class PGTrace;
}

class PGTrace: public VSPAbstractPage
{
    Q_OBJECT

public:
    explicit PGTrace(QWidget* parent = nullptr);
    ~PGTrace();

    void update(TVSPControlCommand command, VSPPortListModel* portModel, VSPLinkListModel* linkModel) override;

protected:
    void onActionExecute() override;

private:
    Ui::PGTrace* ui;
};
