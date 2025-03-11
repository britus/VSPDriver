/********************************************************************************
** Form generated from reading UI file 'pgtrace.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PGTRACE_H
#define UI_PGTRACE_H

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

class Ui_PGTrace
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *groupBox;
    QGridLayout *gridLayout_2;
    QLabel *label;
    QComboBox *cbPorts;
    QWidget *widget_2;
    QGridLayout *gridLayout;
    QCheckBox *cbxTraceRX;
    QCheckBox *cbxTraceTX;
    QCheckBox *cbxTraceCmd;
    QWidget *widget;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *btnUpdate;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *PGTrace)
    {
        if (PGTrace->objectName().isEmpty())
            PGTrace->setObjectName(QString::fromUtf8("PGTrace"));
        PGTrace->resize(400, 375);
        verticalLayout = new QVBoxLayout(PGTrace);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        groupBox = new QGroupBox(PGTrace);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        gridLayout_2 = new QGridLayout(groupBox);
        gridLayout_2->setSpacing(12);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        label = new QLabel(groupBox);
        label->setObjectName(QString::fromUtf8("label"));
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy);

        gridLayout_2->addWidget(label, 0, 0, 1, 1);

        cbPorts = new QComboBox(groupBox);
        cbPorts->setObjectName(QString::fromUtf8("cbPorts"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(cbPorts->sizePolicy().hasHeightForWidth());
        cbPorts->setSizePolicy(sizePolicy1);
        cbPorts->setMouseTracking(true);
        cbPorts->setTabletTracking(true);

        gridLayout_2->addWidget(cbPorts, 0, 1, 1, 1);

        widget_2 = new QWidget(groupBox);
        widget_2->setObjectName(QString::fromUtf8("widget_2"));
        gridLayout = new QGridLayout(widget_2);
        gridLayout->setSpacing(24);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(0, 48, 0, 0);
        cbxTraceRX = new QCheckBox(widget_2);
        cbxTraceRX->setObjectName(QString::fromUtf8("cbxTraceRX"));
        cbxTraceRX->setMouseTracking(true);
        cbxTraceRX->setTabletTracking(true);

        gridLayout->addWidget(cbxTraceRX, 0, 0, 1, 1);

        cbxTraceTX = new QCheckBox(widget_2);
        cbxTraceTX->setObjectName(QString::fromUtf8("cbxTraceTX"));
        cbxTraceTX->setMouseTracking(true);
        cbxTraceTX->setTabletTracking(true);

        gridLayout->addWidget(cbxTraceTX, 1, 0, 1, 1);

        cbxTraceCmd = new QCheckBox(widget_2);
        cbxTraceCmd->setObjectName(QString::fromUtf8("cbxTraceCmd"));
        cbxTraceCmd->setMouseTracking(true);
        cbxTraceCmd->setTabletTracking(true);

        gridLayout->addWidget(cbxTraceCmd, 0, 1, 1, 1);


        gridLayout_2->addWidget(widget_2, 1, 0, 1, 2);

        widget = new QWidget(groupBox);
        widget->setObjectName(QString::fromUtf8("widget"));
        horizontalLayout = new QHBoxLayout(widget);
        horizontalLayout->setSpacing(24);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalSpacer = new QSpacerItem(233, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        btnUpdate = new QPushButton(widget);
        btnUpdate->setObjectName(QString::fromUtf8("btnUpdate"));
        btnUpdate->setMinimumSize(QSize(110, 0));
        btnUpdate->setMouseTracking(true);
        btnUpdate->setTabletTracking(true);

        horizontalLayout->addWidget(btnUpdate);


        gridLayout_2->addWidget(widget, 3, 0, 1, 2);

        verticalSpacer = new QSpacerItem(20, 136, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_2->addItem(verticalSpacer, 2, 0, 1, 2);


        verticalLayout->addWidget(groupBox);

#if QT_CONFIG(shortcut)
        label->setBuddy(cbPorts);
#endif // QT_CONFIG(shortcut)
        QWidget::setTabOrder(cbPorts, cbxTraceRX);
        QWidget::setTabOrder(cbxTraceRX, cbxTraceTX);
        QWidget::setTabOrder(cbxTraceTX, cbxTraceCmd);
        QWidget::setTabOrder(cbxTraceCmd, btnUpdate);

        retranslateUi(PGTrace);

        QMetaObject::connectSlotsByName(PGTrace);
    } // setupUi

    void retranslateUi(QWidget *PGTrace)
    {
        PGTrace->setWindowTitle(QCoreApplication::translate("PGTrace", "Form", nullptr));
        groupBox->setTitle(QCoreApplication::translate("PGTrace", "Enable/Disable Trace", nullptr));
        label->setText(QCoreApplication::translate("PGTrace", "Serial Port:", nullptr));
        cbxTraceRX->setText(QCoreApplication::translate("PGTrace", "Trace RX", nullptr));
        cbxTraceTX->setText(QCoreApplication::translate("PGTrace", "Trace TX", nullptr));
        cbxTraceCmd->setText(QCoreApplication::translate("PGTrace", "Trace commands", nullptr));
        btnUpdate->setText(QCoreApplication::translate("PGTrace", "Save", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PGTrace: public Ui_PGTrace {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PGTRACE_H
