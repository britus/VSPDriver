// ********************************************************************
// vspabstractpage.h - Generic class for UI pages
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#ifndef VSPABSTRACTPAGE_H
#define VSPABSTRACTPAGE_H

#include <QPushButton>
#include <QWidget>
#include <vspdatamodel.h>
#include <vspdriverclient.h>

class VSPAbstractPage: public QWidget
{
    Q_OBJECT

public:
    explicit VSPAbstractPage(QWidget* parent = nullptr);
    virtual ~VSPAbstractPage();

    virtual void update(
       TVSPControlCommand command,  //
       VSPPortListModel* portModel, //
       VSPLinkListModel* linkModel) = 0;

protected:
    // --
    inline void connectButton(QPushButton* button)
    {
        connect(button, &QPushButton::clicked, this, [this]() {
            onActionExecute();
        });
    }

protected slots:
    virtual void onActionExecute() = 0;

signals:
    // connected with execute button on each page
    void execute(const TVSPControlCommand command, const QVariant& data);
};

#endif // VSPABSTRACTPAGE_H
