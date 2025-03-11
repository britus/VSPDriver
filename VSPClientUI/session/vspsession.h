#pragma once

#include <QObject>
#include <QSessionManager>
#include <QSettings>
#include <vspdatamodel.h>

class VSPSession: public QObject
{
    Q_OBJECT

public:
    explicit VSPSession(QObject* parent = nullptr);

    inline QSettings* settings()
    {
        return m_settings;
    }

    bool saveSession();

private:
    QSettings* m_settings;
};
