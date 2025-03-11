#include "vspdatamodel.h"
#include <QDebug>
#include <QList>
#include <QRect>
#include <QSize>
#include <QString>

VSPDataModel::VSPDataModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

QModelIndex VSPDataModel::parent(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return {};
}

int VSPDataModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return m_records.size();
}

int VSPDataModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    if (dataType() == TDataType::PortItem) {
        return 2;
    }
    return 4;
}

QVariant VSPDataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        switch (role) {
            case Qt::DisplayRole: {
                switch (section) {
                    case 0:
                        return QVariant::fromValue(tr("ID"));
                    case 1:
                        return QVariant::fromValue(tr("Name"));
                    case 2:
                        return QVariant::fromValue(tr("Port A"));
                    case 3:
                        return QVariant::fromValue(tr("Port B"));
                }
                break;
            }
            case Qt::SizeHintRole: {
                switch (section) {
                    case 0:
                        return QVariant::fromValue(QSize(30, 22));
                    case 1:
                        return QVariant::fromValue(QSize(230, 22));
                    case 2:
                        return QVariant::fromValue(QSize(60, 22));
                    case 3:
                        return QVariant::fromValue(QSize(60, 22));
                }
                break;
            }
        }
    }
    return QVariant();
}

bool VSPDataModel::hasChildren(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return false;
}

bool VSPDataModel::canFetchMore(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return false;
}

void VSPDataModel::fetchMore(const QModelIndex& parent)
{
    Q_UNUSED(parent);
}

bool VSPDataModel::removeRows(int row, int count, const QModelIndex& parent)
{
    Q_UNUSED(parent);
    if (m_records.size() < row + count) {
        return false;
    }
    beginResetModel();
    for (int i = row, j = 0; j < count && i < m_records.size(); i++, j++) {
        m_records.removeAt(i);
    }
    endResetModel();
    return true;
}

QVariant VSPDataModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_records.size()) {
        return QVariant();
    }

    const TDataRecord r = m_records.at(index.row());

    switch (role) {
        case Qt::DisplayRole: {
            switch (r.type) {
                case TDataType::PortItem: {
                    switch (index.column()) {
                        case 0: { // * id
                            return QVariant::fromValue(QString::number(r.port.id));
                        }
                        case 1: { // * name
                            return QVariant::fromValue(r.port.name);
                        }
                    }
                    break;
                }
                case TDataType::PortLink: {
                    switch (index.column()) {
                        case 0: { // * id
                            return QVariant::fromValue(QString::number(r.link.id));
                        }
                        case 1: { // * name
                            return QVariant::fromValue(r.link.name);
                        }
                        case 2: { // port 1
                            return QVariant::fromValue(r.link.source.name);
                        }
                        case 3: { // port 2
                            return QVariant::fromValue(r.link.target.name);
                        }
                    }
                    break;
                }
            }
            break;
        }
        case Qt::UserRole: {
            return QVariant::fromValue(r);
            break;
        }
        case Qt::FontRole: {
            break;
        }
        case Qt::BackgroundRole: {
            break;
        }
        case Qt::ForegroundRole: {
            break;
        }
    }

    return QVariant();
}

void VSPDataModel::resetModel()
{
    beginResetModel();
    m_records.clear();
    endResetModel();
}

void VSPDataModel::append(const TPortItem& port)
{
    if (dataType() != TDataType::PortItem)
        return;

    foreach (auto r, m_records) {
        if (r.port.id == port.id) {
            qWarning() << "Serial port" << port.id << "already assigned, skip.";
            return;
        }
    }
    beginResetModel();
    m_records.append({TDataType::PortItem, port, {}, 0});
    endResetModel();
}

void VSPDataModel::append(const TPortLink& link)
{
    if (dataType() != TDataType::PortLink)
        return;

    foreach (auto r, m_records) {
        if (r.link.id == link.id) {
            qWarning() << "Port link" << link.id << "already assigned, skip.";
            return;
        }
    }
    beginResetModel();
    m_records.append({TDataType::PortLink, {}, link, 0});
    endResetModel();
}

QVariant VSPDataModel::at(int index) const
{
    if (index < 0 || index >= m_records.size()) {
        return {};
    }
    return QVariant::fromValue(m_records.at(index));
}

bool VSPDataModel::loadModel(QSettings* settings)
{
    switch (dataType()) {
        case PortItem: {
            return load(QStringLiteral("ports"), settings);
            break;
        }
        case PortLink: {
            return load(QStringLiteral("links"), settings);
            break;
        }
    }
    return false;
}

void VSPDataModel::saveModel(QSettings* settings)
{
    switch (dataType()) {
        case PortItem: {
            save(QStringLiteral("ports"), settings);
            break;
        }
        case PortLink: {
            save(QStringLiteral("links"), settings);
            break;
        }
    }
}

// -------------------------------------------

inline static bool __setId(quint8* id, const QString& value)
{
    bool ok;
    (*id) = value.toUInt(&ok);
    if (!ok || !(*id)) {
        return false;
    }
    return true;
}

inline static bool __setName(QString* name, const QString& value)
{
    (*name) = value;
    if (name->isEmpty()) {
        return false;
    }
    return true;
}

// -------------------------------------------

bool VSPPortListModel::load(const QString& group, QSettings* settings)
{
    uint count;
    QString value;

    settings->beginGroup(group);
    count = settings->value("count").toUInt();
    for (uint i = 0; i < count; i++) {
        value = settings->value(QStringLiteral("port_%1").arg(i)).toString();
        if (value.isEmpty())
            continue;
        VSPDataModel::TPortItem p = {};
        QStringList items = value.split("|");
        for (int j = 0; j < items.size(); j++) {
            switch (j) {
                case 0: {
                    if (!__setId(&p.id, items[j])) {
                        goto error_exit;
                    }
                    break;
                }
                case 1: {
                    if (!__setName(&p.name, items[j])) {
                        goto error_exit;
                    }
                    break;
                }
            }
        }
        append(p);
    }
    settings->endGroup();

    return m_records.size() > 0;

error_exit:
    m_records.clear();
    return false;
}

void VSPPortListModel::save(const QString& group, QSettings* settings)
{
    settings->beginGroup(group);
    settings->setValue("count", m_records.size());
    for (quint8 i = 0; i < m_records.size(); i++) {
        const VSPDataModel::TDataRecord r = m_records.at(i);
        const QString key = QStringLiteral("port_%1").arg(i);
        const QString value = QStringLiteral("%1|%2").arg(r.port.id).arg(r.port.name);
        settings->setValue(key, value);
    }
    settings->endGroup();
}

// -------------------------------------------

bool VSPLinkListModel::load(const QString& group, QSettings* settings)
{
    uint count;
    QString value;

    settings->beginGroup(group);
    count = settings->value("count").toUInt();
    for (uint i = 0; i < count; i++) {
        value = settings->value(QStringLiteral("link_%1").arg(i)).toString();
        if (value.isEmpty())
            continue;
        VSPDataModel::TPortLink l = {};
        QStringList items = value.split("|");
        for (int j = 0; j < items.size(); j++) {
            switch (j) {
                case 0: {
                    if (!__setId(&l.id, items[j])) {
                        goto error_exit;
                    }
                    break;
                }
                case 1: {
                    if (!__setName(&l.name, items[j])) {
                        goto error_exit;
                    }
                    break;
                }
                case 2: {
                    if (!__setId(&l.source.id, items[j])) {
                        goto error_exit;
                    }
                    break;
                }
                case 3: {
                    if (!__setName(&l.source.name, items[j])) {
                        goto error_exit;
                    }
                    break;
                }
                case 4: {
                    if (!__setId(&l.target.id, items[j])) {
                        goto error_exit;
                    }
                    break;
                }
                case 5: {
                    if (!__setName(&l.target.name, items[j])) {
                        goto error_exit;
                    }
                    break;
                }
            }
        }
        append(l);
    }
    settings->endGroup();

    return m_records.size() > 0;

error_exit:
    m_records.clear();
    return false;
}

void VSPLinkListModel::save(const QString& group, QSettings* settings)
{
    settings->beginGroup(group);
    settings->setValue("count", m_records.size());
    for (quint8 i = 0; i < m_records.size(); i++) {
        const VSPDataModel::TDataRecord r = m_records.at(i);
        const QString key = QStringLiteral("link_%1").arg(i);
        const QString value = QStringLiteral("%1|%2|%3|%4|%5|%6") //
                                 .arg(r.link.id)
                                 .arg(r.link.name)
                                 .arg(r.link.source.id)
                                 .arg(r.link.source.name)
                                 .arg(r.link.target.id)
                                 .arg(r.link.target.name);
        settings->setValue(key, value);
    }
    settings->endGroup();
}
