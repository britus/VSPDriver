/********************************************************************************
** Form generated from reading UI file 'pgchecks.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PGCHECKS_H
#define UI_PGCHECKS_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
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

class Ui_PGChecks
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *groupBox;
    QGridLayout *gridLayout_2;
    QWidget *widget_3;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label;
    QComboBox *cbPorts;
    QWidget *widget_2;
    QGridLayout *gridLayout;
    QCheckBox *cbxBaudRate;
    QCheckBox *cbxDataBits;
    QCheckBox *cbxStopBits;
    QCheckBox *cbxParity;
    QCheckBox *cbxFlowCtrl;
    QSpacerItem *verticalSpacer;
    QWidget *widget;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *btnUpdate;

    void setupUi(QWidget *PGChecks)
    {
        if (PGChecks->objectName().isEmpty())
            PGChecks->setObjectName(QString::fromUtf8("PGChecks"));
        PGChecks->resize(400, 300);
        verticalLayout = new QVBoxLayout(PGChecks);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        groupBox = new QGroupBox(PGChecks);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        gridLayout_2 = new QGridLayout(groupBox);
        gridLayout_2->setSpacing(12);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        widget_3 = new QWidget(groupBox);
        widget_3->setObjectName(QString::fromUtf8("widget_3"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(widget_3->sizePolicy().hasHeightForWidth());
        widget_3->setSizePolicy(sizePolicy);
        horizontalLayout_2 = new QHBoxLayout(widget_3);
        horizontalLayout_2->setSpacing(12);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(0, 0, 0, 24);
        label = new QLabel(widget_3);
        label->setObjectName(QString::fromUtf8("label"));
        QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy1);

        horizontalLayout_2->addWidget(label);

        cbPorts = new QComboBox(widget_3);
        cbPorts->setObjectName(QString::fromUtf8("cbPorts"));
        QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(cbPorts->sizePolicy().hasHeightForWidth());
        cbPorts->setSizePolicy(sizePolicy2);
        cbPorts->setMouseTracking(true);
        cbPorts->setTabletTracking(true);

        horizontalLayout_2->addWidget(cbPorts);


        gridLayout_2->addWidget(widget_3, 0, 0, 1, 1);

        widget_2 = new QWidget(groupBox);
        widget_2->setObjectName(QString::fromUtf8("widget_2"));
        gridLayout = new QGridLayout(widget_2);
        gridLayout->setSpacing(24);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(0, 0, 0, 0);
        cbxBaudRate = new QCheckBox(widget_2);
        cbxBaudRate->setObjectName(QString::fromUtf8("cbxBaudRate"));
        cbxBaudRate->setMouseTracking(true);
        cbxBaudRate->setTabletTracking(true);

        gridLayout->addWidget(cbxBaudRate, 0, 0, 1, 1);

        cbxDataBits = new QCheckBox(widget_2);
        cbxDataBits->setObjectName(QString::fromUtf8("cbxDataBits"));
        cbxDataBits->setMouseTracking(true);
        cbxDataBits->setTabletTracking(true);

        gridLayout->addWidget(cbxDataBits, 0, 1, 1, 1);

        cbxStopBits = new QCheckBox(widget_2);
        cbxStopBits->setObjectName(QString::fromUtf8("cbxStopBits"));
        cbxStopBits->setMouseTracking(true);
        cbxStopBits->setTabletTracking(true);

        gridLayout->addWidget(cbxStopBits, 0, 2, 1, 1);

        cbxParity = new QCheckBox(widget_2);
        cbxParity->setObjectName(QString::fromUtf8("cbxParity"));
        cbxParity->setMouseTracking(true);
        cbxParity->setTabletTracking(true);

        gridLayout->addWidget(cbxParity, 1, 0, 1, 1);

        cbxFlowCtrl = new QCheckBox(widget_2);
        cbxFlowCtrl->setObjectName(QString::fromUtf8("cbxFlowCtrl"));
        cbxFlowCtrl->setMouseTracking(true);
        cbxFlowCtrl->setTabletTracking(true);

        gridLayout->addWidget(cbxFlowCtrl, 1, 1, 1, 1);


        gridLayout_2->addWidget(widget_2, 1, 0, 1, 1);

        verticalSpacer = new QSpacerItem(20, 119, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_2->addItem(verticalSpacer, 2, 0, 1, 1);

        widget = new QWidget(groupBox);
        widget->setObjectName(QString::fromUtf8("widget"));
        horizontalLayout = new QHBoxLayout(widget);
        horizontalLayout->setSpacing(24);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalSpacer = new QSpacerItem(229, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        btnUpdate = new QPushButton(widget);
        btnUpdate->setObjectName(QString::fromUtf8("btnUpdate"));
        btnUpdate->setMinimumSize(QSize(110, 0));
        btnUpdate->setMouseTracking(true);
        btnUpdate->setTabletTracking(true);

        horizontalLayout->addWidget(btnUpdate);


        gridLayout_2->addWidget(widget, 3, 0, 1, 1);


        verticalLayout->addWidget(groupBox);

#if QT_CONFIG(shortcut)
        label->setBuddy(cbPorts);
#endif // QT_CONFIG(shortcut)
        QWidget::setTabOrder(cbPorts, cbxBaudRate);
        QWidget::setTabOrder(cbxBaudRate, cbxParity);
        QWidget::setTabOrder(cbxParity, cbxDataBits);
        QWidget::setTabOrder(cbxDataBits, cbxFlowCtrl);
        QWidget::setTabOrder(cbxFlowCtrl, cbxStopBits);
        QWidget::setTabOrder(cbxStopBits, btnUpdate);

        retranslateUi(PGChecks);

        QMetaObject::connectSlotsByName(PGChecks);
    } // setupUi

    void retranslateUi(QWidget *PGChecks)
    {
        PGChecks->setWindowTitle(QCoreApplication::translate("PGChecks", "Form", nullptr));
        groupBox->setTitle(QCoreApplication::translate("PGChecks", "Serial Port Parameter Checks", nullptr));
        label->setText(QCoreApplication::translate("PGChecks", "Serial Port:", nullptr));
        cbxBaudRate->setText(QCoreApplication::translate("PGChecks", "Baud rate", nullptr));
        cbxDataBits->setText(QCoreApplication::translate("PGChecks", "Data bits", nullptr));
        cbxStopBits->setText(QCoreApplication::translate("PGChecks", "Stop bits", nullptr));
        cbxParity->setText(QCoreApplication::translate("PGChecks", "Parity", nullptr));
        cbxFlowCtrl->setText(QCoreApplication::translate("PGChecks", "Flow control", nullptr));
        btnUpdate->setText(QCoreApplication::translate("PGChecks", "Save", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PGChecks: public Ui_PGChecks {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PGCHECKS_H
