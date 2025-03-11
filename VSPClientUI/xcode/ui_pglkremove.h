/********************************************************************************
** Form generated from reading UI file 'pglkremove.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PGLKREMOVE_H
#define UI_PGLKREMOVE_H

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

class Ui_PGLKRemove
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
    QLabel *label;
    QComboBox *comboBox;
    QWidget *widget;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *btnUnlink;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *PGLKRemove)
    {
        if (PGLKRemove->objectName().isEmpty())
            PGLKRemove->setObjectName(QString::fromUtf8("PGLKRemove"));
        PGLKRemove->resize(400, 300);
        verticalLayout = new QVBoxLayout(PGLKRemove);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        groupBox = new QGroupBox(PGLKRemove);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label = new QLabel(groupBox);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 0, 0, 1, 1);

        comboBox = new QComboBox(groupBox);
        comboBox->setObjectName(QString::fromUtf8("comboBox"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(comboBox->sizePolicy().hasHeightForWidth());
        comboBox->setSizePolicy(sizePolicy);
        comboBox->setMouseTracking(true);
        comboBox->setTabletTracking(true);

        gridLayout->addWidget(comboBox, 0, 1, 1, 1);

        widget = new QWidget(groupBox);
        widget->setObjectName(QString::fromUtf8("widget"));
        horizontalLayout = new QHBoxLayout(widget);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalSpacer = new QSpacerItem(219, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        btnUnlink = new QPushButton(widget);
        btnUnlink->setObjectName(QString::fromUtf8("btnUnlink"));
        btnUnlink->setMinimumSize(QSize(110, 0));
        btnUnlink->setMouseTracking(true);
        btnUnlink->setTabletTracking(true);

        horizontalLayout->addWidget(btnUnlink);


        gridLayout->addWidget(widget, 1, 0, 1, 2);

        verticalSpacer = new QSpacerItem(20, 155, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer, 2, 0, 1, 1);


        verticalLayout->addWidget(groupBox);

#if QT_CONFIG(shortcut)
        label->setBuddy(comboBox);
#endif // QT_CONFIG(shortcut)
        QWidget::setTabOrder(comboBox, btnUnlink);

        retranslateUi(PGLKRemove);

        QMetaObject::connectSlotsByName(PGLKRemove);
    } // setupUi

    void retranslateUi(QWidget *PGLKRemove)
    {
        PGLKRemove->setWindowTitle(QCoreApplication::translate("PGLKRemove", "Form", nullptr));
        groupBox->setTitle(QCoreApplication::translate("PGLKRemove", "Remove port link", nullptr));
        label->setText(QCoreApplication::translate("PGLKRemove", "Port link:", nullptr));
        btnUnlink->setText(QCoreApplication::translate("PGLKRemove", "Unlink", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PGLKRemove: public Ui_PGLKRemove {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PGLKREMOVE_H
