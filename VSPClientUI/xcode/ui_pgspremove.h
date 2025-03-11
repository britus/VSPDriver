/********************************************************************************
** Form generated from reading UI file 'pgspremove.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PGSPREMOVE_H
#define UI_PGSPREMOVE_H

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

class Ui_PGSPRemove
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout_2;
    QLabel *label;
    QComboBox *cbSerialPorts;
    QWidget *widget_7;
    QHBoxLayout *horizontalLayout_3;
    QSpacerItem *horizontalSpacer_2;
    QPushButton *btnDoSPRemove;
    QSpacerItem *verticalSpacer_3;

    void setupUi(QWidget *PGSPRemove)
    {
        if (PGSPRemove->objectName().isEmpty())
            PGSPRemove->setObjectName(QString::fromUtf8("PGSPRemove"));
        PGSPRemove->resize(400, 300);
        verticalLayout = new QVBoxLayout(PGSPRemove);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        groupBox_2 = new QGroupBox(PGSPRemove);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        gridLayout_2 = new QGridLayout(groupBox_2);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        label = new QLabel(groupBox_2);
        label->setObjectName(QString::fromUtf8("label"));
        label->setMouseTracking(true);
        label->setTabletTracking(true);

        gridLayout_2->addWidget(label, 0, 0, 1, 1);

        cbSerialPorts = new QComboBox(groupBox_2);
        cbSerialPorts->setObjectName(QString::fromUtf8("cbSerialPorts"));
        QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(cbSerialPorts->sizePolicy().hasHeightForWidth());
        cbSerialPorts->setSizePolicy(sizePolicy);
        cbSerialPorts->setMouseTracking(true);
        cbSerialPorts->setTabletTracking(true);

        gridLayout_2->addWidget(cbSerialPorts, 0, 1, 1, 1);

        widget_7 = new QWidget(groupBox_2);
        widget_7->setObjectName(QString::fromUtf8("widget_7"));
        horizontalLayout_3 = new QHBoxLayout(widget_7);
        horizontalLayout_3->setSpacing(24);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalLayout_3->setContentsMargins(0, 0, 4, 0);
        horizontalSpacer_2 = new QSpacerItem(349, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_2);

        btnDoSPRemove = new QPushButton(widget_7);
        btnDoSPRemove->setObjectName(QString::fromUtf8("btnDoSPRemove"));
        btnDoSPRemove->setMinimumSize(QSize(110, 0));
        btnDoSPRemove->setMouseTracking(true);
        btnDoSPRemove->setTabletTracking(true);

        horizontalLayout_3->addWidget(btnDoSPRemove);


        gridLayout_2->addWidget(widget_7, 1, 0, 1, 2);

        verticalSpacer_3 = new QSpacerItem(20, 102, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_2->addItem(verticalSpacer_3, 2, 0, 1, 1);


        verticalLayout->addWidget(groupBox_2);

#if QT_CONFIG(shortcut)
        label->setBuddy(cbSerialPorts);
#endif // QT_CONFIG(shortcut)
        QWidget::setTabOrder(cbSerialPorts, btnDoSPRemove);

        retranslateUi(PGSPRemove);

        QMetaObject::connectSlotsByName(PGSPRemove);
    } // setupUi

    void retranslateUi(QWidget *PGSPRemove)
    {
        PGSPRemove->setWindowTitle(QCoreApplication::translate("PGSPRemove", "Form", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("PGSPRemove", "Remove Serial Port:", nullptr));
        label->setText(QCoreApplication::translate("PGSPRemove", "Serial Port:", nullptr));
        btnDoSPRemove->setText(QCoreApplication::translate("PGSPRemove", "Remove", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PGSPRemove: public Ui_PGSPRemove {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PGSPREMOVE_H
