#pragma once

#include <QComboBox>
#include <QMainWindow>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QThread>

QT_BEGIN_NAMESPACE

namespace Ui {
class MainWindow;
}

QT_END_NAMESPACE

class MainWindow: public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
private slots:
    /*serial port */
    void onPortErrorOccured(QSerialPort::SerialPortError error);
    void onPortBytesWritten(qint64 bytes);
    void onPortReadyRead();
    void onPortClosed();
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

private:
    Ui::MainWindow* ui;
    QSerialPort* m_port;
    QThread* m_looper;
    quint64 m_outTotal;
    bool m_isLooping;
    bool m_looperStop;
    int m_looperCount;
};
