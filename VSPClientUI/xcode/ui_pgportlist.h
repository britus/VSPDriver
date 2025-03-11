/********************************************************************************
** Form generated from reading UI file 'pgportlist.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PGPORTLIST_H
#define UI_PGPORTLIST_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTableView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_PGPortList
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout_2;
    QTableView *tableView;
    QWidget *widget;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *btnRefresh;

    void setupUi(QWidget *PGPortList)
    {
        if (PGPortList->objectName().isEmpty())
            PGPortList->setObjectName(QString::fromUtf8("PGPortList"));
        PGPortList->resize(400, 300);
        verticalLayout = new QVBoxLayout(PGPortList);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        groupBox = new QGroupBox(PGPortList);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        verticalLayout_2 = new QVBoxLayout(groupBox);
        verticalLayout_2->setSpacing(12);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(4, 4, 4, 4);
        tableView = new QTableView(groupBox);
        tableView->setObjectName(QString::fromUtf8("tableView"));
        tableView->setMouseTracking(true);
        tableView->setTabletTracking(true);
        tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        tableView->setTabKeyNavigation(false);
        tableView->setProperty("showDropIndicator", QVariant(false));
        tableView->setDragDropOverwriteMode(false);
        tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        tableView->setShowGrid(false);
        tableView->setWordWrap(false);
        tableView->setCornerButtonEnabled(false);
        tableView->horizontalHeader()->setDefaultSectionSize(50);
        tableView->horizontalHeader()->setHighlightSections(false);
        tableView->horizontalHeader()->setStretchLastSection(true);
        tableView->verticalHeader()->setVisible(false);
        tableView->verticalHeader()->setHighlightSections(false);

        verticalLayout_2->addWidget(tableView);

        widget = new QWidget(groupBox);
        widget->setObjectName(QString::fromUtf8("widget"));
        horizontalLayout = new QHBoxLayout(widget);
        horizontalLayout->setSpacing(24);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 4, 0);
        horizontalSpacer = new QSpacerItem(278, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        btnRefresh = new QPushButton(widget);
        btnRefresh->setObjectName(QString::fromUtf8("btnRefresh"));
        btnRefresh->setMouseTracking(true);
        btnRefresh->setTabletTracking(true);

        horizontalLayout->addWidget(btnRefresh);


        verticalLayout_2->addWidget(widget);


        verticalLayout->addWidget(groupBox);

        QWidget::setTabOrder(tableView, btnRefresh);

        retranslateUi(PGPortList);

        QMetaObject::connectSlotsByName(PGPortList);
    } // setupUi

    void retranslateUi(QWidget *PGPortList)
    {
        PGPortList->setWindowTitle(QCoreApplication::translate("PGPortList", "Form", nullptr));
        groupBox->setTitle(QCoreApplication::translate("PGPortList", "Available serial ports", nullptr));
        btnRefresh->setText(QCoreApplication::translate("PGPortList", "Refresh", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PGPortList: public Ui_PGPortList {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PGPORTLIST_H
