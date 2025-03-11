// ********************************************************************
// pgconnect.h - Connect VSPDriver user client interface
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
class PGConnect;
}

class PGConnect: public VSPAbstractPage
{
    Q_OBJECT

public:
    explicit PGConnect(QWidget* parent = nullptr);
    ~PGConnect();

    void update(TVSPControlCommand command, VSPPortListModel* portModel, VSPLinkListModel* linkModel) override;

signals:
    void installDriver();
    void uninstallDriver();

protected:
    void onActionExecute() override;

private:
    Ui::PGConnect* ui;
};
