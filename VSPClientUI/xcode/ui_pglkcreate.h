/********************************************************************************
** Form generated from reading UI file 'pglkcreate.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PGLKCREATE_H
#define UI_PGLKCREATE_H

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

class Ui_PGLKCreate
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *gbxLinkPorts;
    QGridLayout *gridLayout;
    QSpacerItem *verticalSpacer;
    QComboBox *cbPort2;
    QLabel *label_2;
    QComboBox *cbPort1;
    QWidget *pnlButtons;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *btnLinkPorts;
    QLabel *label;
    QLabel *txInfo;

    void setupUi(QWidget *PGLKCreate)
    {
        if (PGLKCreate->objectName().isEmpty())
            PGLKCreate->setObjectName(QString::fromUtf8("PGLKCreate"));
        PGLKCreate->resize(479, 354);
        verticalLayout = new QVBoxLayout(PGLKCreate);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        gbxLinkPorts = new QGroupBox(PGLKCreate);
        gbxLinkPorts->setObjectName(QString::fromUtf8("gbxLinkPorts"));
        gridLayout = new QGridLayout(gbxLinkPorts);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        verticalSpacer = new QSpacerItem(20, 185, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer, 6, 0, 1, 1);

        cbPort2 = new QComboBox(gbxLinkPorts);
        cbPort2->setObjectName(QString::fromUtf8("cbPort2"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(cbPort2->sizePolicy().hasHeightForWidth());
        cbPort2->setSizePolicy(sizePolicy);

        gridLayout->addWidget(cbPort2, 2, 2, 1, 1);

        label_2 = new QLabel(gbxLinkPorts);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gridLayout->addWidget(label_2, 2, 0, 1, 1);

        cbPort1 = new QComboBox(gbxLinkPorts);
        cbPort1->setObjectName(QString::fromUtf8("cbPort1"));
        sizePolicy.setHeightForWidth(cbPort1->sizePolicy().hasHeightForWidth());
        cbPort1->setSizePolicy(sizePolicy);

        gridLayout->addWidget(cbPort1, 1, 2, 1, 1);

        pnlButtons = new QWidget(gbxLinkPorts);
        pnlButtons->setObjectName(QString::fromUtf8("pnlButtons"));
        horizontalLayout = new QHBoxLayout(pnlButtons);
        horizontalLayout->setSpacing(24);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 24, 5, 0);
        horizontalSpacer = new QSpacerItem(307, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        btnLinkPorts = new QPushButton(pnlButtons);
        btnLinkPorts->setObjectName(QString::fromUtf8("btnLinkPorts"));
        btnLinkPorts->setMinimumSize(QSize(110, 0));
        btnLinkPorts->setAutoDefault(true);

        horizontalLayout->addWidget(btnLinkPorts);


        gridLayout->addWidget(pnlButtons, 4, 0, 1, 3);

        label = new QLabel(gbxLinkPorts);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 1, 0, 1, 1);

        txInfo = new QLabel(gbxLinkPorts);
        txInfo->setObjectName(QString::fromUtf8("txInfo"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(txInfo->sizePolicy().hasHeightForWidth());
        txInfo->setSizePolicy(sizePolicy1);
        txInfo->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        txInfo->setWordWrap(true);
        txInfo->setMargin(2);
        txInfo->setIndent(1);

        gridLayout->addWidget(txInfo, 5, 0, 1, 3);


        verticalLayout->addWidget(gbxLinkPorts);

#if QT_CONFIG(shortcut)
        label_2->setBuddy(cbPort2);
        label->setBuddy(cbPort1);
#endif // QT_CONFIG(shortcut)
        QWidget::setTabOrder(cbPort1, cbPort2);
        QWidget::setTabOrder(cbPort2, btnLinkPorts);

        retranslateUi(PGLKCreate);

        btnLinkPorts->setDefault(true);


        QMetaObject::connectSlotsByName(PGLKCreate);
    } // setupUi

    void retranslateUi(QWidget *PGLKCreate)
    {
        PGLKCreate->setWindowTitle(QCoreApplication::translate("PGLKCreate", "Form", nullptr));
        gbxLinkPorts->setTitle(QCoreApplication::translate("PGLKCreate", "Link serial ports together", nullptr));
        label_2->setText(QCoreApplication::translate("PGLKCreate", "Serial Port 2:", nullptr));
        btnLinkPorts->setText(QCoreApplication::translate("PGLKCreate", "Link", nullptr));
        label->setText(QCoreApplication::translate("PGLKCreate", "Serial Port 1:", nullptr));
        txInfo->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class PGLKCreate: public Ui_PGLKCreate {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PGLKCREATE_H
