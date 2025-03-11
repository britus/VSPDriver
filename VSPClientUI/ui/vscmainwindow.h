// ********************************************************************
// vscmainwindow.h - Application window
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#pragma once
#include <QCloseEvent>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSystemTrayIcon>
#include <QToolButton>
#include <QWidget>
#include <vspabstractpage.h>
#include <vspdatamodel.h>
#include <vspdriverclient.h>
#include <vspsession.h>

QT_BEGIN_NAMESPACE

namespace Ui {
class VSCMainWindow;
}

QT_END_NAMESPACE

class VSCMainWindow: public QMainWindow
{
    Q_OBJECT

public:
    VSCMainWindow(QWidget* parent = nullptr);
    ~VSCMainWindow();

    void closeEvent(QCloseEvent* event) override Q_OVERRIDE(QMainWindow);

public slots:
    // UI to VSP event
    void onActionExecute(const VSPClient::TVSPControlCommand command, const QVariant& data);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void changeEvent(QEvent* event) override;

protected slots:
    // Main window overlay event
    virtual void updateOverlayGeometry();

private slots:
    void onCommitSession(QSessionManager&);
    void onSaveSession(QSessionManager&);
    void onAppQuit();
    // VSPController events
    void onClientConnected();
    void onClientDisconnected();
    void onClientError(const VSPClient::TVSPSystemError& error, const QString& message);
    void onUpdateStatusLog(const QByteArray& message);
    void onUpdateButtons(bool enabled = false);
    void onCommandResult(VSPClient::TVSPControlCommand command, VSPPortListModel* portModel, VSPLinkListModel* linkModel);
    void onComplete();
    // VSPSetup events
    void onSetupFailWithError(quint64 code, const char* message);
    void onSetupFinishWithResult(quint64 code, const char* message);
    void onSetupNeedsUserApproval();
    // Main window events
    void onSelectPage();
    void onActionInstall();
    void onActionUninstall();

private:
    Ui::VSCMainWindow* ui;
    VSPDriverClient* m_vsp;
    VSPSession m_session;
    QMap<QPushButton*, VSPAbstractPage*> m_buttonMap;
    QMap<quint64, QString> m_errorStack;
    QMessageBox m_box;
    QSystemTrayIcon stIcon;
    bool m_firstStart;
    bool m_demoMode;

private:
    inline void setupSystemTray();
    inline void showAboutBox();
    inline void installDriver();
    inline bool connectDriver();
    inline void showDemoMessage(const QString& message);
    inline void showNotification(int ms, const QString& text);
    inline void resetDefaultButtons();
    inline void setDefaultButton(QPushButton* button, bool isDefault = true);
    inline void createVspController();
    inline void connectVspController();
    inline void addToolbarAndMenus();
    inline void connectUiEvents();
    inline void showOverlay();
    inline void removeOverlay();
};

class PopupMenu: public QMenu
{
    Q_OBJECT

public:
    explicit PopupMenu(QWidget* target, QWidget* parent = 0);
    void showEvent(QShowEvent* event);

private:
    QWidget* w;
};
