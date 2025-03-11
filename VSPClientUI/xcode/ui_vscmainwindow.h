/********************************************************************************
** Form generated from reading UI file 'vscmainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_VSCMAINWINDOW_H
#define UI_VSCMAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "pgchecks.h"
#include "pgconnect.h"
#include "pglinklist.h"
#include "pglkcreate.h"
#include "pglkremove.h"
#include "pgportlist.h"
#include "pgspcreate.h"
#include "pgspremove.h"
#include "pgtrace.h"

QT_BEGIN_NAMESPACE

class Ui_VSCMainWindow
{
public:
    QWidget *centralwidget;
    QHBoxLayout *horizontalLayout_2;
    QWidget *pnlButtons;
    QVBoxLayout *verticalLayout_2;
    QPushButton *btn01SPCreate;
    QPushButton *btn02SPRemove;
    QPushButton *btn03LKCreate;
    QPushButton *btn04LKRemove;
    QPushButton *btn05PortList;
    QPushButton *btn06LinkList;
    QPushButton *btn07Checks;
    QPushButton *btn08Traces;
    QPushButton *btn11SerialIO;
    QPushButton *btn09Connect;
    QPushButton *btn10Close;
    QSpacerItem *verticalSpacer;
    QWidget *widget_2;
    QHBoxLayout *horizontalLayout;
    QToolButton *toolButton;
    QSpacerItem *horizontalSpacer;
    QWidget *pnlContent;
    QVBoxLayout *verticalLayout_3;
    QWidget *pnlPages;
    QVBoxLayout *verticalLayout_4;
    QStackedWidget *stackedWidget;
    PGSPCreate *pg01SPCreate;
    PGSPRemove *pg02SPRemove;
    PGLKCreate *pg03LKCreate;
    PGLKRemove *pg04LKRemove;
    PGPortList *pg05PortList;
    PGLinkList *pg06LinkList;
    PGChecks *pg07Checks;
    PGTrace *pg08Traces;
    PGConnect *pg09Connect;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout;
    QTextBrowser *textBrowser;
    QMenuBar *menubar;

    void setupUi(QMainWindow *VSCMainWindow)
    {
        if (VSCMainWindow->objectName().isEmpty())
            VSCMainWindow->setObjectName(QString::fromUtf8("VSCMainWindow"));
        VSCMainWindow->resize(720, 580);
        VSCMainWindow->setMinimumSize(QSize(720, 580));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/assets/png/vspclient_7.png"), QSize(), QIcon::Normal, QIcon::Off);
        VSCMainWindow->setWindowIcon(icon);
        VSCMainWindow->setDocumentMode(true);
        centralwidget = new QWidget(VSCMainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        horizontalLayout_2 = new QHBoxLayout(centralwidget);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        pnlButtons = new QWidget(centralwidget);
        pnlButtons->setObjectName(QString::fromUtf8("pnlButtons"));
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Ignored);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(pnlButtons->sizePolicy().hasHeightForWidth());
        pnlButtons->setSizePolicy(sizePolicy);
        pnlButtons->setMinimumSize(QSize(160, 0));
        verticalLayout_2 = new QVBoxLayout(pnlButtons);
        verticalLayout_2->setSpacing(12);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(0, -1, 0, 0);
        btn01SPCreate = new QPushButton(pnlButtons);
        btn01SPCreate->setObjectName(QString::fromUtf8("btn01SPCreate"));
        btn01SPCreate->setEnabled(false);
        btn01SPCreate->setMouseTracking(true);
        btn01SPCreate->setTabletTracking(true);

        verticalLayout_2->addWidget(btn01SPCreate);

        btn02SPRemove = new QPushButton(pnlButtons);
        btn02SPRemove->setObjectName(QString::fromUtf8("btn02SPRemove"));
        btn02SPRemove->setEnabled(false);
        btn02SPRemove->setMouseTracking(true);
        btn02SPRemove->setTabletTracking(true);

        verticalLayout_2->addWidget(btn02SPRemove);

        btn03LKCreate = new QPushButton(pnlButtons);
        btn03LKCreate->setObjectName(QString::fromUtf8("btn03LKCreate"));
        btn03LKCreate->setEnabled(false);
        btn03LKCreate->setMinimumSize(QSize(150, 0));
        btn03LKCreate->setMouseTracking(true);
        btn03LKCreate->setTabletTracking(true);

        verticalLayout_2->addWidget(btn03LKCreate);

        btn04LKRemove = new QPushButton(pnlButtons);
        btn04LKRemove->setObjectName(QString::fromUtf8("btn04LKRemove"));
        btn04LKRemove->setEnabled(false);
        btn04LKRemove->setMinimumSize(QSize(150, 0));
        btn04LKRemove->setMouseTracking(true);
        btn04LKRemove->setTabletTracking(true);

        verticalLayout_2->addWidget(btn04LKRemove);

        btn05PortList = new QPushButton(pnlButtons);
        btn05PortList->setObjectName(QString::fromUtf8("btn05PortList"));
        btn05PortList->setEnabled(false);
        btn05PortList->setMinimumSize(QSize(150, 0));
        btn05PortList->setMouseTracking(true);
        btn05PortList->setTabletTracking(true);

        verticalLayout_2->addWidget(btn05PortList);

        btn06LinkList = new QPushButton(pnlButtons);
        btn06LinkList->setObjectName(QString::fromUtf8("btn06LinkList"));
        btn06LinkList->setEnabled(false);
        btn06LinkList->setMouseTracking(true);
        btn06LinkList->setTabletTracking(true);

        verticalLayout_2->addWidget(btn06LinkList);

        btn07Checks = new QPushButton(pnlButtons);
        btn07Checks->setObjectName(QString::fromUtf8("btn07Checks"));
        btn07Checks->setEnabled(false);
        btn07Checks->setMinimumSize(QSize(150, 0));
        btn07Checks->setMouseTracking(true);
        btn07Checks->setTabletTracking(true);

        verticalLayout_2->addWidget(btn07Checks);

        btn08Traces = new QPushButton(pnlButtons);
        btn08Traces->setObjectName(QString::fromUtf8("btn08Traces"));
        btn08Traces->setEnabled(false);
        btn08Traces->setMinimumSize(QSize(150, 0));
        btn08Traces->setMouseTracking(true);
        btn08Traces->setTabletTracking(true);

        verticalLayout_2->addWidget(btn08Traces);

        btn11SerialIO = new QPushButton(pnlButtons);
        btn11SerialIO->setObjectName(QString::fromUtf8("btn11SerialIO"));
        btn11SerialIO->setEnabled(false);
        btn11SerialIO->setMouseTracking(true);
        btn11SerialIO->setTabletTracking(true);

        verticalLayout_2->addWidget(btn11SerialIO);

        btn09Connect = new QPushButton(pnlButtons);
        btn09Connect->setObjectName(QString::fromUtf8("btn09Connect"));
        btn09Connect->setMouseTracking(true);
        btn09Connect->setTabletTracking(true);
        btn09Connect->setAutoDefault(true);

        verticalLayout_2->addWidget(btn09Connect);

        btn10Close = new QPushButton(pnlButtons);
        btn10Close->setObjectName(QString::fromUtf8("btn10Close"));
        btn10Close->setMouseTracking(true);
        btn10Close->setTabletTracking(true);

        verticalLayout_2->addWidget(btn10Close);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_2->addItem(verticalSpacer);

        widget_2 = new QWidget(pnlButtons);
        widget_2->setObjectName(QString::fromUtf8("widget_2"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Minimum);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(widget_2->sizePolicy().hasHeightForWidth());
        widget_2->setSizePolicy(sizePolicy1);
        widget_2->setMaximumSize(QSize(16777215, 24));
        horizontalLayout = new QHBoxLayout(widget_2);
        horizontalLayout->setSpacing(24);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        toolButton = new QToolButton(widget_2);
        toolButton->setObjectName(QString::fromUtf8("toolButton"));
        toolButton->setMaximumSize(QSize(12, 12));
        toolButton->setFocusPolicy(Qt::NoFocus);
        toolButton->setPopupMode(QToolButton::DelayedPopup);
        toolButton->setToolButtonStyle(Qt::ToolButtonIconOnly);

        horizontalLayout->addWidget(toolButton);

        horizontalSpacer = new QSpacerItem(14, 111, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        verticalLayout_2->addWidget(widget_2);


        horizontalLayout_2->addWidget(pnlButtons);

        pnlContent = new QWidget(centralwidget);
        pnlContent->setObjectName(QString::fromUtf8("pnlContent"));
        QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(pnlContent->sizePolicy().hasHeightForWidth());
        pnlContent->setSizePolicy(sizePolicy2);
        verticalLayout_3 = new QVBoxLayout(pnlContent);
        verticalLayout_3->setSpacing(24);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        verticalLayout_3->setContentsMargins(12, 0, 0, 0);
        pnlPages = new QWidget(pnlContent);
        pnlPages->setObjectName(QString::fromUtf8("pnlPages"));
        sizePolicy1.setHeightForWidth(pnlPages->sizePolicy().hasHeightForWidth());
        pnlPages->setSizePolicy(sizePolicy1);
        verticalLayout_4 = new QVBoxLayout(pnlPages);
        verticalLayout_4->setSpacing(0);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        verticalLayout_4->setContentsMargins(0, 0, 0, 0);
        stackedWidget = new QStackedWidget(pnlPages);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
        stackedWidget->setMouseTracking(true);
        stackedWidget->setTabletTracking(true);
        stackedWidget->setFrameShape(QFrame::NoFrame);
        pg01SPCreate = new PGSPCreate();
        pg01SPCreate->setObjectName(QString::fromUtf8("pg01SPCreate"));
        stackedWidget->addWidget(pg01SPCreate);
        pg02SPRemove = new PGSPRemove();
        pg02SPRemove->setObjectName(QString::fromUtf8("pg02SPRemove"));
        stackedWidget->addWidget(pg02SPRemove);
        pg03LKCreate = new PGLKCreate();
        pg03LKCreate->setObjectName(QString::fromUtf8("pg03LKCreate"));
        stackedWidget->addWidget(pg03LKCreate);
        pg04LKRemove = new PGLKRemove();
        pg04LKRemove->setObjectName(QString::fromUtf8("pg04LKRemove"));
        stackedWidget->addWidget(pg04LKRemove);
        pg05PortList = new PGPortList();
        pg05PortList->setObjectName(QString::fromUtf8("pg05PortList"));
        stackedWidget->addWidget(pg05PortList);
        pg06LinkList = new PGLinkList();
        pg06LinkList->setObjectName(QString::fromUtf8("pg06LinkList"));
        stackedWidget->addWidget(pg06LinkList);
        pg07Checks = new PGChecks();
        pg07Checks->setObjectName(QString::fromUtf8("pg07Checks"));
        stackedWidget->addWidget(pg07Checks);
        pg08Traces = new PGTrace();
        pg08Traces->setObjectName(QString::fromUtf8("pg08Traces"));
        stackedWidget->addWidget(pg08Traces);
        pg09Connect = new PGConnect();
        pg09Connect->setObjectName(QString::fromUtf8("pg09Connect"));
        stackedWidget->addWidget(pg09Connect);

        verticalLayout_4->addWidget(stackedWidget);


        verticalLayout_3->addWidget(pnlPages);

        groupBox = new QGroupBox(pnlContent);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        verticalLayout = new QVBoxLayout(groupBox);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
        verticalLayout->setContentsMargins(4, 4, 4, 4);
        textBrowser = new QTextBrowser(groupBox);
        textBrowser->setObjectName(QString::fromUtf8("textBrowser"));
        QFont font;
        font.setFamily(QString::fromUtf8("Menlo"));
        font.setBold(true);
        textBrowser->setFont(font);
        textBrowser->setMouseTracking(false);
        textBrowser->setFocusPolicy(Qt::NoFocus);
        textBrowser->setAutoFormatting(QTextEdit::AutoNone);
        textBrowser->setTabChangesFocus(true);
        textBrowser->setLineWrapMode(QTextEdit::NoWrap);
        textBrowser->setOpenExternalLinks(true);
        textBrowser->setOpenLinks(true);

        verticalLayout->addWidget(textBrowser);


        verticalLayout_3->addWidget(groupBox);


        horizontalLayout_2->addWidget(pnlContent);

        horizontalLayout_2->setStretch(1, 90);
        VSCMainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(VSCMainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 720, 24));
        VSCMainWindow->setMenuBar(menubar);
        QWidget::setTabOrder(btn01SPCreate, btn02SPRemove);
        QWidget::setTabOrder(btn02SPRemove, btn03LKCreate);
        QWidget::setTabOrder(btn03LKCreate, btn04LKRemove);
        QWidget::setTabOrder(btn04LKRemove, btn05PortList);
        QWidget::setTabOrder(btn05PortList, btn06LinkList);
        QWidget::setTabOrder(btn06LinkList, btn07Checks);
        QWidget::setTabOrder(btn07Checks, btn08Traces);
        QWidget::setTabOrder(btn08Traces, btn11SerialIO);
        QWidget::setTabOrder(btn11SerialIO, btn09Connect);
        QWidget::setTabOrder(btn09Connect, btn10Close);
        QWidget::setTabOrder(btn10Close, toolButton);
        QWidget::setTabOrder(toolButton, textBrowser);

        retranslateUi(VSCMainWindow);
        QObject::connect(btn10Close, SIGNAL(clicked()), VSCMainWindow, SLOT(close()));

        btn09Connect->setDefault(true);
        stackedWidget->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(VSCMainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *VSCMainWindow)
    {
        VSCMainWindow->setWindowTitle(QCoreApplication::translate("VSCMainWindow", "Virtual Serial Port Controller", nullptr));
        btn01SPCreate->setText(QCoreApplication::translate("VSCMainWindow", "Create Serial Port", nullptr));
        btn02SPRemove->setText(QCoreApplication::translate("VSCMainWindow", "Remove Serial Port", nullptr));
        btn03LKCreate->setText(QCoreApplication::translate("VSCMainWindow", "Create Port Link", nullptr));
        btn04LKRemove->setText(QCoreApplication::translate("VSCMainWindow", "Remove Port Link", nullptr));
        btn05PortList->setText(QCoreApplication::translate("VSCMainWindow", "Get Port List", nullptr));
        btn06LinkList->setText(QCoreApplication::translate("VSCMainWindow", "Get Link List", nullptr));
        btn07Checks->setText(QCoreApplication::translate("VSCMainWindow", "Enable Checks", nullptr));
        btn08Traces->setText(QCoreApplication::translate("VSCMainWindow", "Enable Traces", nullptr));
        btn11SerialIO->setText(QCoreApplication::translate("VSCMainWindow", "Serial I/O Test", nullptr));
        btn09Connect->setText(QCoreApplication::translate("VSCMainWindow", "Connect", nullptr));
        btn10Close->setText(QCoreApplication::translate("VSCMainWindow", "Close", nullptr));
        toolButton->setText(QString());
        groupBox->setTitle(QCoreApplication::translate("VSCMainWindow", "Results", nullptr));
    } // retranslateUi

};

namespace Ui {
    class VSCMainWindow: public Ui_VSCMainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_VSCMAINWINDOW_H
