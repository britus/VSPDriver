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
#include <QSettings>
#include <QWidget>
#include <vspdatamodel.h>
#include <vspdriverclient.h>
#include <vspsession.h>

class VSPAbstractPage: public QWidget
{
    Q_OBJECT

public:
    explicit VSPAbstractPage(QWidget* parent = nullptr);
    virtual ~VSPAbstractPage();

    virtual void loadSettings(QSettings* settings);
    virtual void saveSettings(QSettings* settings);

    virtual void update(
       TVSPControlCommand command,  //
       VSPPortListModel* portModel, //
       VSPLinkListModel* linkModel) = 0;

protected:
    inline void connectButton(QPushButton* button)
    {
        connect(button, &QPushButton::clicked, this, [this]() {
            onActionExecute();
        });
    }

protected slots:
    virtual void onActionExecute() = 0;

signals:
    void execute(const VSPClient::TVSPControlCommand command, const QVariant& data);
};

#endif // VSPABSTRACTPAGE_H
