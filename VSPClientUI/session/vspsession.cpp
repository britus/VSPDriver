#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <vspsession.h>

VSPSession::VSPSession(QObject* parent)
    : QObject(parent)
    , m_settings(nullptr)
{
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation));
    QString fileName = dir.absolutePath() //
                          .append(QDir::separator())
                          .append(qApp->organizationDomain())
                          .append(".vspsession.conf");
    m_settings = new QSettings(fileName, QSettings::IniFormat, this);
}

bool VSPSession::saveSession()
{
    if (m_settings) {
        m_settings->sync();
        switch (m_settings->status()) {
            case QSettings::Status::AccessError: {
                qWarning() << "[VSPSES] AccessError: Failed to save session.";
                return false;
            }
            case QSettings::Status::FormatError: {
                qWarning() << "[VSPSES] FormatError: Failed to save session.";
                return false;
            }
            default: {
            }
        }
    }
    return true;
}
