/********************************************************************************
** Form generated from reading UI file 'pglinklist.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PGLINKLIST_H
#define UI_PGLINKLIST_H

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

class Ui_PGLinkList
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

    void setupUi(QWidget *PGLinkList)
    {
        if (PGLinkList->objectName().isEmpty())
            PGLinkList->setObjectName(QString::fromUtf8("PGLinkList"));
        PGLinkList->resize(400, 300);
        verticalLayout = new QVBoxLayout(PGLinkList);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        groupBox = new QGroupBox(PGLinkList);
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

        retranslateUi(PGLinkList);

        QMetaObject::connectSlotsByName(PGLinkList);
    } // setupUi

    void retranslateUi(QWidget *PGLinkList)
    {
        PGLinkList->setWindowTitle(QCoreApplication::translate("PGLinkList", "Form", nullptr));
        groupBox->setTitle(QCoreApplication::translate("PGLinkList", "Available port links", nullptr));
        btnRefresh->setText(QCoreApplication::translate("PGLinkList", "Refresh", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PGLinkList: public Ui_PGLinkList {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PGLINKLIST_H
