// ********************************************************************
// vspdriverclient.cpp - VSPDriver user client interface
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#include <QDebug>
#include <QTextStream>
#include <QTimer>
#include <vspdriverclient.h>

VSPDriverClient::VSPDriverClient(const QByteArray& dextBundleId, const QByteArray& dextClassName, VSPSession* session, QObject* parent)
    : QObject(parent)
    , VSPController(dextClassName.constData())
    , VSPDriverSetup(dextBundleId.constData())
    , m_session(session)
    , m_portList(this)
    , m_linkList(this)
{
}

VSPDriverClient::~VSPDriverClient()
{
    //
}

void VSPDriverClient::OnConnected()
{
    m_portList.resetModel();
    m_linkList.resetModel();
    restoreDriverSession();
    emit connected();
}

void VSPDriverClient::OnDisconnected()
{
    m_portList.saveModel(m_session->settings());
    m_portList.resetModel();

    m_linkList.saveModel(m_session->settings());
    m_linkList.resetModel();

    emit disconnected();
}

void VSPDriverClient::OnDidFailWithError(quint64 code, const char* message)
{
    emit didFailWithError(code, message);
}

void VSPDriverClient::OnDidFinishWithResult(quint64 code, const char* message)
{
    emit didFinishWithResult(code, message);
}

void VSPDriverClient::OnNeedsUserApproval()
{
    emit needsUserApproval();
}

// called in sync of request
void VSPDriverClient::OnDataReady(const TVSPControllerData& data)
{
    QByteArray buffer;
    QTextStream text(&buffer);

    // make sure we have an error code
    uint result = data.status.code;

    QByteArray txStatus =
       (data.context != 4 //
           ? tr(" success").toUtf8()
           : tr("Last command failed.").toUtf8());

    text << tr("Driver callback result:") << Qt::endl;
    text << tr("Data size......: ") << Qt::dec << VSP_UCD_SIZE << Qt::endl;
    text << tr("Context........: ") << Qt::dec << data.context << Qt::endl;
    text << tr("Command........: ") << Qt::dec << data.command << txStatus << Qt::endl;
    text << tr("Status code....: 0x") << Qt::hex << data.status.code << Qt::endl;
    text << tr("Status flags...: 0x") << Qt::hex << data.status.flags << Qt::endl;
    text << tr("Parameter flags: 0x") << Qt::hex << data.parameter.flags << Qt::endl;
    text << tr("Port 1.........: ") << Qt::dec << data.parameter.link.source << Qt::endl;
    text << tr("Port 2.........: ") << Qt::dec << data.parameter.link.target << Qt::endl;
    text << tr("Port count.....: ") << Qt::dec << data.ports.count << Qt::endl;
    text << tr("Link count.....: ") << Qt::dec << data.links.count << Qt::endl;

    if (data.ports.count) {
        m_portList.resetModel();
        for (uint i = 0; i < data.ports.count; i++) {
            TVSPPortListItem pli = data.ports.list[i];
            QString name = strlen(pli.name) == 0 //
                              ? tr("Port %1").arg(pli.id)
                              : tr("%1").arg(pli.name);
            text << "Port item......: " << pli.id << " " << name << Qt::endl;
            m_portList.append(VSPDataModel::TPortItem({pli.id, name}));
            continue;
        }
    }
    else if (data.command == vspControlRemovePort) {
        if (m_portList.rowCount() > 0) {
            m_portList.resetModel();
        }
    }

    if (data.links.count) {
        m_linkList.resetModel();
        for (uint i = 0; i < data.links.count; i++) {
            const uint8_t _lid = (data.links.list[i] >> 16) & 0x000000ff;
            const uint8_t _src = (data.links.list[i] >> 8) & 0x000000ff;
            const uint8_t _tgt = (data.links.list[i]) & 0x000000ff;
            VSPDataModel::TPortItem p1 = {};
            VSPDataModel::TPortItem p2 = {};
            QString name = tr("[Port A: %1 <-> Port B: %2]").arg(_src).arg(_tgt);
            text << "Link item......: " << _lid << " " << name << Qt::endl;
            for (int i = 0; i < m_portList.rowCount(); i++) {
                VSPDataModel::TDataRecord r = m_portList.at(i).value<VSPDataModel::TDataRecord>();
                if (r.port.id == _src) {
                    p1 = r.port;
                }
                if (r.port.id == _tgt) {
                    p2 = r.port;
                }
                if (p1.id && p2.id) {
                    break;
                }
            }
            m_linkList.append(VSPDataModel::TPortLink(
               {_lid, //
                tr("Port Link %1 %2").arg(_lid).arg(name),
                p1,
                p2}));
            continue;
        }
    }
    else if (data.command == vspControlUnlinkPorts) {
        if (m_linkList.rowCount() > 0) {
            m_linkList.resetModel();
        }
    }

    // Overlay-Größe anpassen
    QTimer* t = new QTimer(this);
    connect(t, &QTimer::timeout, this, [this, txStatus, buffer, data, result]() {
        emit updateStatusLog(buffer);
        emit updateButtons(true);
        if (result != 0) {
            emit errorOccured(GetSystemError(result), txStatus);
        }
        else {
            emit commandResult(                               //
               static_cast<TVSPControlCommand>(data.command), //
               &m_portList,
               &m_linkList);
        }
        emit complete();
    });
    t->setTimerType(Qt::PreciseTimer);
    t->setSingleShot(true);
    t->start(150);
}

// called async of request
void VSPDriverClient::OnIOUCCallback(int result, void* /*args*/, uint32_t /*size*/)
{
    // const TVSPControllerData* data = (TVSPControllerData*) (args);
    if (result != 0) {
        //
    }
}

void VSPDriverClient::OnErrorOccured(const VSPClient::TVSPSystemError& error, const char* message)
{
    emit errorOccured(error, message);
}

bool VSPDriverClient::saveDriverSession()
{
    m_portList.saveModel(m_session->settings());
    m_linkList.saveModel(m_session->settings());
    return m_session->saveSession();
}

inline bool VSPDriverClient::restoreDriverSession()
{
    TVSPControllerData data = {};

    data.context = vspContextPing;
    data.command = vspControlPingPong;
    data.parameter.flags = 0xff08;

    if (m_portList.loadModel(m_session->settings())) {
        data.ports.count = m_portList.rowCount();
        for (quint8 i = 0; i < data.ports.count && i < MAX_SERIAL_PORTS; i++) {
            const VSPDataModel::TDataRecord r = m_portList.at(i).value<VSPDataModel::TDataRecord>();
            strncpy(data.ports.list[i].name, qPrintable(r.port.name), MAX_PORT_NAME - 1);
            data.ports.list[i].id = r.port.id;
        }
    }

    if (m_linkList.loadModel(m_session->settings())) {
        data.links.count = m_linkList.rowCount();
        for (quint8 i = 0; i < data.links.count && i < MAX_PORT_LINKS; i++) {
            const VSPDataModel::TDataRecord r = m_linkList.at(i).value<VSPDataModel::TDataRecord>();
            data.links.list[i] = ((r.link.id << 16) & 0xff0000);
            data.links.list[i] |= ((r.link.source.id << 8) & 0xff00);
            data.links.list[i] |= ((r.link.target.id) & 0x00ff);
        }
    }

    return SendData(data);
}
