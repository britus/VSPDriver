/********************************************************************************
** Form generated from reading UI file 'vspserialio.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_VSPSERIALIO_H
#define UI_VSPSERIALIO_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_VSPSerialIO
{
public:
    QAction *actionClose;
    QAction *actionNewWindow;
    QVBoxLayout *verticalLayout;
    QGroupBox *gbxSerialPort;
    QGridLayout *gridLayout;
    QLabel *label_25;
    QLabel *label_26;
    QComboBox *cbxBaud;
    QLabel *label_28;
    QComboBox *cbxStopBits;
    QLabel *label_30;
    QComboBox *cbxParity;
    QLabel *label_27;
    QComboBox *cbxDataBits;
    QWidget *widget;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *btnConnect;
    QCheckBox *cbxDtr;
    QCheckBox *cbxRts;
    QCheckBox *cbxCts;
    QCheckBox *cbxDSR;
    QLabel *label_29;
    QComboBox *cbxFlowControl;
    QComboBox *cbxComPort;
    QGroupBox *gbxOutput;
    QGridLayout *gridLayout_2;
    QLabel *label;
    QLabel *label_3;
    QComboBox *cbxLineEnding;
    QLabel *label_2;
    QPushButton *btnLooper;
    QSpinBox *edGenLength;
    QLineEdit *edOutputLine;
    QPushButton *btnSendLine;
    QPushButton *btnSendFile;
    QGroupBox *gbxInput;
    QVBoxLayout *verticalLayout_2;
    QPlainTextEdit *txInputView;
    QLabel *txOutputInfo;

    void setupUi(QDialog *VSPSerialIO)
    {
        if (VSPSerialIO->objectName().isEmpty())
            VSPSerialIO->setObjectName(QString::fromUtf8("VSPSerialIO"));
        VSPSerialIO->resize(417, 694);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/assets/png/vspclient_7.png"), QSize(), QIcon::Normal, QIcon::Off);
        VSPSerialIO->setWindowIcon(icon);
        VSPSerialIO->setSizeGripEnabled(true);
        actionClose = new QAction(VSPSerialIO);
        actionClose->setObjectName(QString::fromUtf8("actionClose"));
        QIcon icon1(QIcon::fromTheme(QString::fromUtf8("process-stop")));
        actionClose->setIcon(icon1);
        actionNewWindow = new QAction(VSPSerialIO);
        actionNewWindow->setObjectName(QString::fromUtf8("actionNewWindow"));
        QIcon icon2(QIcon::fromTheme(QString::fromUtf8("document-open")));
        actionNewWindow->setIcon(icon2);
        verticalLayout = new QVBoxLayout(VSPSerialIO);
        verticalLayout->setSpacing(24);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(8, 8, 8, 8);
        gbxSerialPort = new QGroupBox(VSPSerialIO);
        gbxSerialPort->setObjectName(QString::fromUtf8("gbxSerialPort"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(gbxSerialPort->sizePolicy().hasHeightForWidth());
        gbxSerialPort->setSizePolicy(sizePolicy);
        gridLayout = new QGridLayout(gbxSerialPort);
        gridLayout->setSpacing(12);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label_25 = new QLabel(gbxSerialPort);
        label_25->setObjectName(QString::fromUtf8("label_25"));

        gridLayout->addWidget(label_25, 0, 0, 1, 1);

        label_26 = new QLabel(gbxSerialPort);
        label_26->setObjectName(QString::fromUtf8("label_26"));

        gridLayout->addWidget(label_26, 1, 0, 1, 1);

        cbxBaud = new QComboBox(gbxSerialPort);
        cbxBaud->setObjectName(QString::fromUtf8("cbxBaud"));
        QSizePolicy sizePolicy1(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(cbxBaud->sizePolicy().hasHeightForWidth());
        cbxBaud->setSizePolicy(sizePolicy1);
        cbxBaud->setMinimumSize(QSize(0, 24));
        cbxBaud->setMaxVisibleItems(15);

        gridLayout->addWidget(cbxBaud, 1, 1, 1, 1);

        label_28 = new QLabel(gbxSerialPort);
        label_28->setObjectName(QString::fromUtf8("label_28"));

        gridLayout->addWidget(label_28, 2, 0, 1, 1);

        cbxStopBits = new QComboBox(gbxSerialPort);
        cbxStopBits->setObjectName(QString::fromUtf8("cbxStopBits"));
        sizePolicy1.setHeightForWidth(cbxStopBits->sizePolicy().hasHeightForWidth());
        cbxStopBits->setSizePolicy(sizePolicy1);
        cbxStopBits->setMinimumSize(QSize(0, 24));
        cbxStopBits->setMaxVisibleItems(15);

        gridLayout->addWidget(cbxStopBits, 2, 1, 1, 1);

        label_30 = new QLabel(gbxSerialPort);
        label_30->setObjectName(QString::fromUtf8("label_30"));

        gridLayout->addWidget(label_30, 3, 0, 1, 1);

        cbxParity = new QComboBox(gbxSerialPort);
        cbxParity->setObjectName(QString::fromUtf8("cbxParity"));
        sizePolicy1.setHeightForWidth(cbxParity->sizePolicy().hasHeightForWidth());
        cbxParity->setSizePolicy(sizePolicy1);
        cbxParity->setMinimumSize(QSize(0, 24));
        cbxParity->setMaxVisibleItems(15);

        gridLayout->addWidget(cbxParity, 3, 1, 1, 1);

        label_27 = new QLabel(gbxSerialPort);
        label_27->setObjectName(QString::fromUtf8("label_27"));

        gridLayout->addWidget(label_27, 4, 0, 1, 1);

        cbxDataBits = new QComboBox(gbxSerialPort);
        cbxDataBits->setObjectName(QString::fromUtf8("cbxDataBits"));
        sizePolicy1.setHeightForWidth(cbxDataBits->sizePolicy().hasHeightForWidth());
        cbxDataBits->setSizePolicy(sizePolicy1);
        cbxDataBits->setMinimumSize(QSize(0, 24));
        cbxDataBits->setMaxVisibleItems(15);

        gridLayout->addWidget(cbxDataBits, 4, 1, 1, 1);

        widget = new QWidget(gbxSerialPort);
        widget->setObjectName(QString::fromUtf8("widget"));
        horizontalLayout = new QHBoxLayout(widget);
        horizontalLayout->setSpacing(12);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        btnConnect = new QPushButton(widget);
        btnConnect->setObjectName(QString::fromUtf8("btnConnect"));

        horizontalLayout->addWidget(btnConnect);


        gridLayout->addWidget(widget, 5, 0, 1, 4);

        cbxDtr = new QCheckBox(gbxSerialPort);
        cbxDtr->setObjectName(QString::fromUtf8("cbxDtr"));
        cbxDtr->setChecked(true);

        gridLayout->addWidget(cbxDtr, 2, 2, 1, 1);

        cbxRts = new QCheckBox(gbxSerialPort);
        cbxRts->setObjectName(QString::fromUtf8("cbxRts"));
        cbxRts->setChecked(true);

        gridLayout->addWidget(cbxRts, 3, 2, 1, 1);

        cbxCts = new QCheckBox(gbxSerialPort);
        cbxCts->setObjectName(QString::fromUtf8("cbxCts"));
        cbxCts->setEnabled(false);

        gridLayout->addWidget(cbxCts, 2, 3, 1, 1);

        cbxDSR = new QCheckBox(gbxSerialPort);
        cbxDSR->setObjectName(QString::fromUtf8("cbxDSR"));
        cbxDSR->setEnabled(false);

        gridLayout->addWidget(cbxDSR, 3, 3, 1, 1);

        label_29 = new QLabel(gbxSerialPort);
        label_29->setObjectName(QString::fromUtf8("label_29"));

        gridLayout->addWidget(label_29, 1, 2, 1, 1);

        cbxFlowControl = new QComboBox(gbxSerialPort);
        cbxFlowControl->setObjectName(QString::fromUtf8("cbxFlowControl"));
        sizePolicy1.setHeightForWidth(cbxFlowControl->sizePolicy().hasHeightForWidth());
        cbxFlowControl->setSizePolicy(sizePolicy1);
        cbxFlowControl->setMinimumSize(QSize(0, 24));
        cbxFlowControl->setMaxVisibleItems(15);

        gridLayout->addWidget(cbxFlowControl, 1, 3, 1, 1);

        cbxComPort = new QComboBox(gbxSerialPort);
        cbxComPort->setObjectName(QString::fromUtf8("cbxComPort"));
        sizePolicy1.setHeightForWidth(cbxComPort->sizePolicy().hasHeightForWidth());
        cbxComPort->setSizePolicy(sizePolicy1);
        cbxComPort->setMinimumSize(QSize(0, 24));
        cbxComPort->setMaxVisibleItems(15);

        gridLayout->addWidget(cbxComPort, 0, 1, 1, 3);


        verticalLayout->addWidget(gbxSerialPort);

        gbxOutput = new QGroupBox(VSPSerialIO);
        gbxOutput->setObjectName(QString::fromUtf8("gbxOutput"));
        sizePolicy.setHeightForWidth(gbxOutput->sizePolicy().hasHeightForWidth());
        gbxOutput->setSizePolicy(sizePolicy);
        gridLayout_2 = new QGridLayout(gbxOutput);
        gridLayout_2->setSpacing(12);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        label = new QLabel(gbxOutput);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout_2->addWidget(label, 0, 0, 1, 1);

        label_3 = new QLabel(gbxOutput);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        gridLayout_2->addWidget(label_3, 1, 0, 1, 1);

        cbxLineEnding = new QComboBox(gbxOutput);
        cbxLineEnding->addItem(QString());
        cbxLineEnding->addItem(QString());
        cbxLineEnding->addItem(QString());
        cbxLineEnding->addItem(QString());
        cbxLineEnding->addItem(QString());
        cbxLineEnding->addItem(QString());
        cbxLineEnding->addItem(QString());
        cbxLineEnding->setObjectName(QString::fromUtf8("cbxLineEnding"));
        sizePolicy1.setHeightForWidth(cbxLineEnding->sizePolicy().hasHeightForWidth());
        cbxLineEnding->setSizePolicy(sizePolicy1);
        cbxLineEnding->setMinimumSize(QSize(0, 24));

        gridLayout_2->addWidget(cbxLineEnding, 3, 1, 1, 1);

        label_2 = new QLabel(gbxOutput);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gridLayout_2->addWidget(label_2, 3, 0, 1, 1);

        btnLooper = new QPushButton(gbxOutput);
        btnLooper->setObjectName(QString::fromUtf8("btnLooper"));

        gridLayout_2->addWidget(btnLooper, 3, 2, 1, 1);

        edGenLength = new QSpinBox(gbxOutput);
        edGenLength->setObjectName(QString::fromUtf8("edGenLength"));
        edGenLength->setMinimumSize(QSize(0, 24));
        edGenLength->setMinimum(1);
        edGenLength->setMaximum(16384);
        edGenLength->setValue(16);

        gridLayout_2->addWidget(edGenLength, 1, 1, 1, 2);

        edOutputLine = new QLineEdit(gbxOutput);
        edOutputLine->setObjectName(QString::fromUtf8("edOutputLine"));
        sizePolicy1.setHeightForWidth(edOutputLine->sizePolicy().hasHeightForWidth());
        edOutputLine->setSizePolicy(sizePolicy1);
        edOutputLine->setMinimumSize(QSize(0, 24));

        gridLayout_2->addWidget(edOutputLine, 0, 1, 1, 2);

        btnSendLine = new QPushButton(gbxOutput);
        btnSendLine->setObjectName(QString::fromUtf8("btnSendLine"));

        gridLayout_2->addWidget(btnSendLine, 0, 4, 1, 1);

        btnSendFile = new QPushButton(gbxOutput);
        btnSendFile->setObjectName(QString::fromUtf8("btnSendFile"));

        gridLayout_2->addWidget(btnSendFile, 1, 4, 1, 1);


        verticalLayout->addWidget(gbxOutput);

        gbxInput = new QGroupBox(VSPSerialIO);
        gbxInput->setObjectName(QString::fromUtf8("gbxInput"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(gbxInput->sizePolicy().hasHeightForWidth());
        gbxInput->setSizePolicy(sizePolicy2);
        verticalLayout_2 = new QVBoxLayout(gbxInput);
        verticalLayout_2->setSpacing(12);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(4, 4, 4, 4);
        txInputView = new QPlainTextEdit(gbxInput);
        txInputView->setObjectName(QString::fromUtf8("txInputView"));
        txInputView->setTabChangesFocus(true);
        txInputView->setLineWrapMode(QPlainTextEdit::LineWrapMode::NoWrap);
        txInputView->setReadOnly(true);
        txInputView->setTextInteractionFlags(Qt::TextInteractionFlag::TextSelectableByKeyboard|Qt::TextInteractionFlag::TextSelectableByMouse);

        verticalLayout_2->addWidget(txInputView);

        txOutputInfo = new QLabel(gbxInput);
        txOutputInfo->setObjectName(QString::fromUtf8("txOutputInfo"));

        verticalLayout_2->addWidget(txOutputInfo);


        verticalLayout->addWidget(gbxInput);

#if QT_CONFIG(shortcut)
        label_25->setBuddy(cbxComPort);
        label_26->setBuddy(cbxBaud);
        label_28->setBuddy(cbxStopBits);
        label_30->setBuddy(cbxParity);
        label_27->setBuddy(cbxDataBits);
        label_29->setBuddy(cbxFlowControl);
        label->setBuddy(edOutputLine);
        label_3->setBuddy(edGenLength);
        label_2->setBuddy(cbxLineEnding);
#endif // QT_CONFIG(shortcut)
        QWidget::setTabOrder(cbxComPort, cbxBaud);
        QWidget::setTabOrder(cbxBaud, cbxStopBits);
        QWidget::setTabOrder(cbxStopBits, cbxParity);
        QWidget::setTabOrder(cbxParity, cbxDataBits);
        QWidget::setTabOrder(cbxDataBits, cbxFlowControl);
        QWidget::setTabOrder(cbxFlowControl, cbxDtr);
        QWidget::setTabOrder(cbxDtr, cbxRts);
        QWidget::setTabOrder(cbxRts, cbxCts);
        QWidget::setTabOrder(cbxCts, cbxDSR);
        QWidget::setTabOrder(cbxDSR, btnConnect);
        QWidget::setTabOrder(btnConnect, edOutputLine);
        QWidget::setTabOrder(edOutputLine, edGenLength);
        QWidget::setTabOrder(edGenLength, cbxLineEnding);
        QWidget::setTabOrder(cbxLineEnding, btnLooper);
        QWidget::setTabOrder(btnLooper, txInputView);

        retranslateUi(VSPSerialIO);
        QObject::connect(actionClose, SIGNAL(triggered()), VSPSerialIO, SLOT(close()));

        QMetaObject::connectSlotsByName(VSPSerialIO);
    } // setupUi

    void retranslateUi(QDialog *VSPSerialIO)
    {
        VSPSerialIO->setWindowTitle(QCoreApplication::translate("VSPSerialIO", "VSP Serial I/O", nullptr));
        actionClose->setText(QCoreApplication::translate("VSPSerialIO", "Close", nullptr));
#if QT_CONFIG(tooltip)
        actionClose->setToolTip(QCoreApplication::translate("VSPSerialIO", "Close window", nullptr));
#endif // QT_CONFIG(tooltip)
        actionNewWindow->setText(QCoreApplication::translate("VSPSerialIO", "New...", nullptr));
#if QT_CONFIG(tooltip)
        actionNewWindow->setToolTip(QCoreApplication::translate("VSPSerialIO", "Open new window", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(shortcut)
        actionNewWindow->setShortcut(QCoreApplication::translate("VSPSerialIO", "Ctrl+N", nullptr));
#endif // QT_CONFIG(shortcut)
        gbxSerialPort->setTitle(QCoreApplication::translate("VSPSerialIO", "Serial Port", nullptr));
        label_25->setText(QCoreApplication::translate("VSPSerialIO", "Serial Port", nullptr));
        label_26->setText(QCoreApplication::translate("VSPSerialIO", "Baud Rate:", nullptr));
        label_28->setText(QCoreApplication::translate("VSPSerialIO", "Stop Bits:", nullptr));
        label_30->setText(QCoreApplication::translate("VSPSerialIO", "Parity:", nullptr));
        label_27->setText(QCoreApplication::translate("VSPSerialIO", "Data Bits:", nullptr));
        btnConnect->setText(QCoreApplication::translate("VSPSerialIO", "Connect", nullptr));
        cbxDtr->setText(QCoreApplication::translate("VSPSerialIO", "DTR", nullptr));
        cbxRts->setText(QCoreApplication::translate("VSPSerialIO", "RTS", nullptr));
        cbxCts->setText(QCoreApplication::translate("VSPSerialIO", "CTS", nullptr));
        cbxDSR->setText(QCoreApplication::translate("VSPSerialIO", "DSR", nullptr));
        label_29->setText(QCoreApplication::translate("VSPSerialIO", "Flow Ctrl.:", nullptr));
        gbxOutput->setTitle(QCoreApplication::translate("VSPSerialIO", "Output", nullptr));
        label->setText(QCoreApplication::translate("VSPSerialIO", "Send line:", nullptr));
        label_3->setText(QCoreApplication::translate("VSPSerialIO", "Looper length:", nullptr));
        cbxLineEnding->setItemText(0, QCoreApplication::translate("VSPSerialIO", "CR/LF", nullptr));
        cbxLineEnding->setItemText(1, QCoreApplication::translate("VSPSerialIO", "LF", nullptr));
        cbxLineEnding->setItemText(2, QCoreApplication::translate("VSPSerialIO", "CR", nullptr));
        cbxLineEnding->setItemText(3, QCoreApplication::translate("VSPSerialIO", "$", nullptr));
        cbxLineEnding->setItemText(4, QCoreApplication::translate("VSPSerialIO", "|", nullptr));
        cbxLineEnding->setItemText(5, QCoreApplication::translate("VSPSerialIO", ":", nullptr));
        cbxLineEnding->setItemText(6, QCoreApplication::translate("VSPSerialIO", ";", nullptr));

        label_2->setText(QCoreApplication::translate("VSPSerialIO", "Line ending:", nullptr));
        btnLooper->setText(QCoreApplication::translate("VSPSerialIO", "Looper", nullptr));
        btnSendLine->setText(QCoreApplication::translate("VSPSerialIO", "Send Line", nullptr));
        btnSendFile->setText(QCoreApplication::translate("VSPSerialIO", "Send file...", nullptr));
        gbxInput->setTitle(QCoreApplication::translate("VSPSerialIO", "Input", nullptr));
        txInputView->setDocumentTitle(QCoreApplication::translate("VSPSerialIO", "Input", nullptr));
        txOutputInfo->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class VSPSerialIO: public Ui_VSPSerialIO {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_VSPSERIALIO_H
