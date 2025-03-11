/********************************************************************************
** Form generated from reading UI file 'pgconnect.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PGCONNECT_H
#define UI_PGCONNECT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_PGConnect
{
public:
    QVBoxLayout *verticalLayout_2;
    QGroupBox *groupBox_2;
    QVBoxLayout *verticalLayout;
    QWidget *pnlInfo;
    QVBoxLayout *verticalLayout_3;
    QTextBrowser *textBrowser;
    QWidget *pnlOps;
    QHBoxLayout *horizontalLayout;
    QPushButton *btnInstall;
    QSpacerItem *horizontalSpacer;
    QPushButton *btnDemo;

    void setupUi(QWidget *PGConnect)
    {
        if (PGConnect->objectName().isEmpty())
            PGConnect->setObjectName(QString::fromUtf8("PGConnect"));
        PGConnect->resize(310, 318);
        verticalLayout_2 = new QVBoxLayout(PGConnect);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        groupBox_2 = new QGroupBox(PGConnect);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(groupBox_2->sizePolicy().hasHeightForWidth());
        groupBox_2->setSizePolicy(sizePolicy);
        groupBox_2->setFlat(false);
        verticalLayout = new QVBoxLayout(groupBox_2);
        verticalLayout->setSpacing(12);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(4, 4, 4, 4);
        pnlInfo = new QWidget(groupBox_2);
        pnlInfo->setObjectName(QString::fromUtf8("pnlInfo"));
        sizePolicy.setHeightForWidth(pnlInfo->sizePolicy().hasHeightForWidth());
        pnlInfo->setSizePolicy(sizePolicy);
        verticalLayout_3 = new QVBoxLayout(pnlInfo);
        verticalLayout_3->setSpacing(0);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        textBrowser = new QTextBrowser(pnlInfo);
        textBrowser->setObjectName(QString::fromUtf8("textBrowser"));
        textBrowser->setFocusPolicy(Qt::NoFocus);
        textBrowser->setStyleSheet(QString::fromUtf8("background-color: transparent;"));
        textBrowser->setFrameShape(QFrame::NoFrame);
        textBrowser->setFrameShadow(QFrame::Plain);
        textBrowser->setOpenExternalLinks(true);

        verticalLayout_3->addWidget(textBrowser);


        verticalLayout->addWidget(pnlInfo);


        verticalLayout_2->addWidget(groupBox_2);

        pnlOps = new QWidget(PGConnect);
        pnlOps->setObjectName(QString::fromUtf8("pnlOps"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(pnlOps->sizePolicy().hasHeightForWidth());
        pnlOps->setSizePolicy(sizePolicy1);
        pnlOps->setMaximumSize(QSize(16777215, 56));
        horizontalLayout = new QHBoxLayout(pnlOps);
        horizontalLayout->setSpacing(12);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 12, 0, 4);
        btnInstall = new QPushButton(pnlOps);
        btnInstall->setObjectName(QString::fromUtf8("btnInstall"));

        horizontalLayout->addWidget(btnInstall);

        horizontalSpacer = new QSpacerItem(20, 225, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        btnDemo = new QPushButton(pnlOps);
        btnDemo->setObjectName(QString::fromUtf8("btnDemo"));

        horizontalLayout->addWidget(btnDemo);


        verticalLayout_2->addWidget(pnlOps);

        QWidget::setTabOrder(btnInstall, btnDemo);
        QWidget::setTabOrder(btnDemo, textBrowser);

        retranslateUi(PGConnect);

        QMetaObject::connectSlotsByName(PGConnect);
    } // setupUi

    void retranslateUi(QWidget *PGConnect)
    {
        PGConnect->setWindowTitle(QCoreApplication::translate("PGConnect", "Form", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("PGConnect", "Connect VSP Driver", nullptr));
        textBrowser->setHtml(QCoreApplication::translate("PGConnect", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><meta charset=\"utf-8\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"hr { height: 1px; border-width: 0; }\n"
"</style></head><body style=\" font-family:'.AppleSystemUIFont'; font-size:13pt; font-weight:400; font-style:normal;\">\n"
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:700;\">Getting started.</span></p>\n"
"<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">To transfer data between two virtual serial ports, the Virtual Serial Port extension is required. <br /><br />Proceed as follows:</p>\n"
"<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:700;\">* Click"
                        " the &quot;Install Driver&quot; button</span></p>\n"
"<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:700;\">* Confirm authorization by the operating system</span></p>\n"
"<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">After the extension has been activated, a connection to the  extension is automatically established.<br /><br /><span style=\" font-weight:700;\">Using demo mode </span></p>\n"
"<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Click the button <span style=\" font-weight:700;\">Demo Mode</span>. This mode allows you to try the app without the driver extension. Also, you cannot send or receive data to or from a Virtual Serial Port in this mode.<br /><br /><span style=\" font-weight:700;\">Serial I/O Demo Mode</span></p>\n"
"<p style=\" margin-top:12p"
                        "x; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">To use serial I/O in <span style=\" font-weight:700;\">Demo Mode</span>, you need a physical connection between a device that supports a serial port, your computer with a serial port, and a null modem cable.<br /></p></body></html>", nullptr));
        btnInstall->setText(QCoreApplication::translate("PGConnect", "Install Driver", nullptr));
        btnDemo->setText(QCoreApplication::translate("PGConnect", "Demo Mode", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PGConnect: public Ui_PGConnect {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PGCONNECT_H
