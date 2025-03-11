// ********************************************************************
// vspdriverclient.h - VSPDriver user client interface
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#pragma once
#include <QObject>
#include <vspcontroller.hpp>
#include <vspdatamodel.h>
#include <vspdriversetup.hpp>
#include <vspsession.h>

#define kIOErrorNotFound -536870160

using namespace VSPClient;

class VSPDriverClient: public QObject, public VSPController, public VSPDriverSetup
{
    Q_OBJECT

public:
    VSPDriverClient(const QByteArray& dextBundleId, const QByteArray& dextClassName, VSPSession* session, QObject* parent = nullptr);
    virtual ~VSPDriverClient();

    inline VSPPortListModel* portList()
    {
        return &m_portList;
    }

    inline VSPLinkListModel* linkList()
    {
        return &m_linkList;
    }

    inline void createDemoPort()
    {
        VSPDataModel::TPortItem p;
        p.id = m_portList.rowCount() + 1;
        p.name = tr("Port %1").arg(p.id);
        m_portList.append(p);
        emit commandResult(                          //
           TVSPControlCommand::vspControlCreatePort, //
           &m_portList,
           &m_linkList);
        emit complete();
    }

    inline void removeDemoPort(quint8 pid)
    {
        if (m_portList.rowCount() < 1)
            return;
        for (int i = 0; i < m_portList.rowCount(); i++) {
            VSPDataModel::TDataRecord r = m_portList.at(i).value<VSPDataModel::TDataRecord>();
            if (r.port.id == pid) {
                removeDemoLink(r.port.id, r.port.id);
                m_portList.removeRows(i, 1, QModelIndex());
                break;
            }
        }
        emit commandResult(                          //
           TVSPControlCommand::vspControlRemovePort, //
           &m_portList,
           &m_linkList);
        emit complete();
    }

    inline void createDemoLink(quint8 p1, quint8 p2)
    {
        if (m_portList.rowCount() < 2)
            return;

        VSPDataModel::TPortLink l;
        l.id = m_linkList.rowCount() + 1;
        l.name = tr("Link %1: Port %2 <-> Port %3").arg(l.id).arg(p1).arg(p2);
        l.source.id = p1;
        l.source.name = tr("Port %1").arg(l.source.id);
        l.target.id = p2;
        l.target.name = tr("Port %1").arg(l.target.id);
        m_linkList.append(l);
        emit commandResult(                         //
           TVSPControlCommand::vspControlLinkPorts, //
           &m_portList,
           &m_linkList);
        emit complete();
    }

    inline void removeDemoLink(quint8 p1, quint8 p2)
    {
        if (m_linkList.rowCount() < 1)
            return;
        for (int i = 0; i < m_linkList.rowCount(); i++) {
            VSPDataModel::TDataRecord r = m_linkList.at(i).value<VSPDataModel::TDataRecord>();
            if (
               r.link.source.id == p1 || r.link.target.id == p1 || //
               r.link.source.id == p2 || r.link.target.id == p2) {
                m_linkList.removeRows(i, 1, QModelIndex());
                break;
            }
        }
        emit commandResult(                           //
           TVSPControlCommand::vspControlUnlinkPorts, //
           &m_portList,
           &m_linkList);
        emit complete();
    }

    // Interface VSPSetup.framework
    void OnDidFailWithError(uint64_t /*code*/, const char* /*message*/) override;
    void OnDidFinishWithResult(uint64_t /*code*/, const char* /*message*/) override;
    void OnNeedsUserApproval() override;

    bool saveDriverSession();

signals:
    // VSPSetup interface events
    void didFailWithError(quint64 code, const char* message);
    void didFinishWithResult(quint64 code, const char* message);
    void needsUserApproval();
    // VSPController interface events
    void connected();
    void disconnected();
    void commandResult(VSPClient::TVSPControlCommand command, VSPPortListModel* portModel, VSPLinkListModel* linkModel);
    void errorOccured(const VSPClient::TVSPSystemError& error, const QString& message);
    void updateStatusLog(const QByteArray& message);
    void updateButtons(bool enabled = false);
    void complete();

protected:
    // Interface VSPController.framework
    void OnConnected() override;
    void OnDisconnected() override;
    void OnIOUCCallback(int result, void* args, uint32_t numArgs) override;
    void OnErrorOccured(const VSPClient::TVSPSystemError& error, const char* message) override;
    void OnDataReady(const TVSPControllerData& data) override;

private:
    VSPSession* m_session;
    VSPPortListModel m_portList;
    VSPLinkListModel m_linkList;

private:
    inline bool restoreDriverSession();
};
Q_DECLARE_METATYPE(TVSPControllerData)
Q_DECLARE_METATYPE(TVSPPortParameters)
Q_DECLARE_METATYPE(TVSPSystemError)
