#pragma once

#include <QComboBox>
#include <QDialog>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QShowEvent>
#include <QThread>
#include <QWindow>

QT_BEGIN_NAMESPACE

namespace Ui {
class VSPSerialIO;
}

QT_END_NAMESPACE

class VSPSerialIO: public QDialog
{
    Q_OBJECT

public:
    VSPSerialIO(QWidget* parent = nullptr);
    ~VSPSerialIO();
    void closeEvent(QCloseEvent* event) override;
private slots:
    /*serial port */
    void onPortErrorOccured(QSerialPort::SerialPortError error);
    void onPortBytesWritten(qint64 bytes);
    void onPortReadyRead();
    void onPortClosed();
    void onDTRChanged(bool set);
    void onRTSChanged(bool set);
    void onBreakChanged(bool set);
    void on_btnSendFile_clicked();
    void on_btnConnect_clicked();
    void on_edOutputLine_textEdited(const QString& arg1);
    void on_btnSendLine_clicked();
    void on_btnLooper_clicked();
    void on_actionNewWindow_triggered();

private:
    inline void initComboSerialPort(QComboBox* cbx, QComboBox* link = nullptr);
    inline void initComboBaudRate(QComboBox* cbx, QComboBox* link = nullptr);
    inline void initComboDataBits(QComboBox* cbx, QComboBox* link = nullptr);
    inline void initComboStopBits(QComboBox* cbx, QComboBox* link = nullptr);
    inline void initComboParity(QComboBox* cbx, QComboBox* link = nullptr);
    inline void initComboFlowCtrl(QComboBox* cbx, QComboBox* link = nullptr);
    inline void connectPort();
    inline void disconnectPort();
    inline void looperLooper();
    inline void sendFile(const QString& fileName);

private:
    Ui::VSPSerialIO* ui;
    QSerialPort* m_port;
    QThread* m_looper;
    quint64 m_outTotal;
    bool m_isLooping;
    bool m_looperStop;
    int m_looperCount;
};
