/********************************************************************************
** Form generated from reading UI file 'pgspcreate.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PGSPCREATE_H
#define UI_PGSPCREATE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_PGSPCreate
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *gbxSerialPort;
    QGridLayout *gridLayout;
    QLabel *label_26;
    QComboBox *cbBaud;
    QLabel *label_29;
    QComboBox *cbFlowControl;
    QLabel *label_28;
    QComboBox *cbStopBits;
    QLabel *label_27;
    QComboBox *cbDataBits;
    QLabel *label_30;
    QComboBox *cbParity;
    QSpacerItem *verticalSpacer;
    QWidget *widget_6;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *btnDoSPCreate;

    void setupUi(QWidget *PGSPCreate)
    {
        if (PGSPCreate->objectName().isEmpty())
            PGSPCreate->setObjectName(QString::fromUtf8("PGSPCreate"));
        PGSPCreate->resize(400, 300);
        verticalLayout = new QVBoxLayout(PGSPCreate);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        gbxSerialPort = new QGroupBox(PGSPCreate);
        gbxSerialPort->setObjectName(QString::fromUtf8("gbxSerialPort"));
        gridLayout = new QGridLayout(gbxSerialPort);
        gridLayout->setSpacing(12);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label_26 = new QLabel(gbxSerialPort);
        label_26->setObjectName(QString::fromUtf8("label_26"));

        gridLayout->addWidget(label_26, 0, 0, 1, 1);

        cbBaud = new QComboBox(gbxSerialPort);
        cbBaud->setObjectName(QString::fromUtf8("cbBaud"));
        QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(cbBaud->sizePolicy().hasHeightForWidth());
        cbBaud->setSizePolicy(sizePolicy);
        cbBaud->setMinimumSize(QSize(0, 24));
        cbBaud->setMouseTracking(true);
        cbBaud->setTabletTracking(true);
        cbBaud->setMaxVisibleItems(15);

        gridLayout->addWidget(cbBaud, 0, 1, 1, 1);

        label_29 = new QLabel(gbxSerialPort);
        label_29->setObjectName(QString::fromUtf8("label_29"));

        gridLayout->addWidget(label_29, 0, 2, 1, 1);

        cbFlowControl = new QComboBox(gbxSerialPort);
        cbFlowControl->setObjectName(QString::fromUtf8("cbFlowControl"));
        sizePolicy.setHeightForWidth(cbFlowControl->sizePolicy().hasHeightForWidth());
        cbFlowControl->setSizePolicy(sizePolicy);
        cbFlowControl->setMinimumSize(QSize(0, 24));
        cbFlowControl->setMouseTracking(true);
        cbFlowControl->setTabletTracking(true);
        cbFlowControl->setMaxVisibleItems(15);

        gridLayout->addWidget(cbFlowControl, 0, 3, 1, 1);

        label_28 = new QLabel(gbxSerialPort);
        label_28->setObjectName(QString::fromUtf8("label_28"));

        gridLayout->addWidget(label_28, 1, 0, 1, 1);

        cbStopBits = new QComboBox(gbxSerialPort);
        cbStopBits->setObjectName(QString::fromUtf8("cbStopBits"));
        sizePolicy.setHeightForWidth(cbStopBits->sizePolicy().hasHeightForWidth());
        cbStopBits->setSizePolicy(sizePolicy);
        cbStopBits->setMinimumSize(QSize(0, 24));
        cbStopBits->setMouseTracking(true);
        cbStopBits->setTabletTracking(true);
        cbStopBits->setMaxVisibleItems(15);

        gridLayout->addWidget(cbStopBits, 1, 1, 1, 1);

        label_27 = new QLabel(gbxSerialPort);
        label_27->setObjectName(QString::fromUtf8("label_27"));

        gridLayout->addWidget(label_27, 1, 2, 1, 1);

        cbDataBits = new QComboBox(gbxSerialPort);
        cbDataBits->setObjectName(QString::fromUtf8("cbDataBits"));
        sizePolicy.setHeightForWidth(cbDataBits->sizePolicy().hasHeightForWidth());
        cbDataBits->setSizePolicy(sizePolicy);
        cbDataBits->setMinimumSize(QSize(0, 24));
        cbDataBits->setMouseTracking(true);
        cbDataBits->setTabletTracking(true);
        cbDataBits->setMaxVisibleItems(15);

        gridLayout->addWidget(cbDataBits, 1, 3, 1, 1);

        label_30 = new QLabel(gbxSerialPort);
        label_30->setObjectName(QString::fromUtf8("label_30"));

        gridLayout->addWidget(label_30, 2, 0, 1, 1);

        cbParity = new QComboBox(gbxSerialPort);
        cbParity->setObjectName(QString::fromUtf8("cbParity"));
        sizePolicy.setHeightForWidth(cbParity->sizePolicy().hasHeightForWidth());
        cbParity->setSizePolicy(sizePolicy);
        cbParity->setMinimumSize(QSize(0, 24));
        cbParity->setMouseTracking(true);
        cbParity->setTabletTracking(true);
        cbParity->setMaxVisibleItems(15);

        gridLayout->addWidget(cbParity, 2, 1, 1, 1);

        verticalSpacer = new QSpacerItem(20, 38, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer, 4, 0, 1, 1);

        widget_6 = new QWidget(gbxSerialPort);
        widget_6->setObjectName(QString::fromUtf8("widget_6"));
        horizontalLayout = new QHBoxLayout(widget_6);
        horizontalLayout->setSpacing(12);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 4, 0);
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        btnDoSPCreate = new QPushButton(widget_6);
        btnDoSPCreate->setObjectName(QString::fromUtf8("btnDoSPCreate"));
        btnDoSPCreate->setMouseTracking(true);
        btnDoSPCreate->setTabletTracking(true);

        horizontalLayout->addWidget(btnDoSPCreate);


        gridLayout->addWidget(widget_6, 3, 0, 1, 4);


        verticalLayout->addWidget(gbxSerialPort);

#if QT_CONFIG(shortcut)
        label_26->setBuddy(cbBaud);
        label_29->setBuddy(cbFlowControl);
        label_28->setBuddy(cbStopBits);
        label_27->setBuddy(cbDataBits);
        label_30->setBuddy(cbParity);
#endif // QT_CONFIG(shortcut)
        QWidget::setTabOrder(cbBaud, cbStopBits);
        QWidget::setTabOrder(cbStopBits, cbParity);
        QWidget::setTabOrder(cbParity, cbFlowControl);
        QWidget::setTabOrder(cbFlowControl, cbDataBits);
        QWidget::setTabOrder(cbDataBits, btnDoSPCreate);

        retranslateUi(PGSPCreate);

        QMetaObject::connectSlotsByName(PGSPCreate);
    } // setupUi

    void retranslateUi(QWidget *PGSPCreate)
    {
        PGSPCreate->setWindowTitle(QCoreApplication::translate("PGSPCreate", "Form", nullptr));
        gbxSerialPort->setTitle(QCoreApplication::translate("PGSPCreate", "Serial Port", nullptr));
        label_26->setText(QCoreApplication::translate("PGSPCreate", "Baud Rate:", nullptr));
        label_29->setText(QCoreApplication::translate("PGSPCreate", "Flow Ctrl.:", nullptr));
        label_28->setText(QCoreApplication::translate("PGSPCreate", "Stop Bits:", nullptr));
        label_27->setText(QCoreApplication::translate("PGSPCreate", "Data Bits:", nullptr));
        label_30->setText(QCoreApplication::translate("PGSPCreate", "Parity:", nullptr));
        btnDoSPCreate->setText(QCoreApplication::translate("PGSPCreate", "Create", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PGSPCreate: public Ui_PGSPCreate {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PGSPCREATE_H
