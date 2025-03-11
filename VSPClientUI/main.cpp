#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QLocale>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QTranslator>
#include <vscmainwindow.h>

extern "C" {
    extern const char* GetBundleVersion();
    extern const char* GetBuildNumber();
}

class ApplicationStyle: public QProxyStyle
{
public:
    int styleHint(
       StyleHint hint,
       const QStyleOption* option = nullptr, //
       const QWidget* widget = nullptr,      //
       QStyleHintReturn* returnData = nullptr) const override
    {
        switch (hint) {
            case QStyle::SH_ComboBox_Popup: {
                return 0;
            }
            case QStyle::SH_MessageBox_CenterButtons: {
                return 0;
            }
            case QStyle::SH_FocusFrame_AboveWidget: {
                return 1;
            }
            default: {
                break;
            }
        }
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

// See VSPClient.config.pri - extract target name
#define xstr(a) str(a)
#define str(a)  #a

#if defined(VSPCLIENT_LIBRARY)
extern "C" bool qt_main(int argc, char* argv[])
#else
int main(int argc, char* argv[])
#endif
{
    QString bversion = GetBundleVersion();
    QString bnumber = GetBuildNumber();

    // must be set before QApplication instance
    QApplication::setAttribute(Qt::AA_NativeWindows, true);
    QApplication::setAttribute(Qt::AA_UseDesktopOpenGL, true);
    QApplication::setAttribute(Qt::AA_Use96Dpi, true);
    QApplication::setDesktopSettingsAware(true);

    // --
    QApplication a(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("EoF Software Labs"));
    QApplication::setOrganizationDomain(QStringLiteral("org.eof.tools"));
    QApplication::setApplicationDisplayName(QStringLiteral("Virtual Serial Port Controller"));
    QApplication::setApplicationName(QStringLiteral("VSP Controller"));
    QApplication::setApplicationVersion(QStringLiteral("%1.%2").arg(bversion, bnumber));
    QApplication::setQuitOnLastWindowClosed(true);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    a.setFallbackSessionManagementEnabled(true);
#endif

    a.setAutoSipEnabled(true);
    a.connect(&a, &QApplication::lastWindowClosed, &a, &QApplication::quit);

    /* Override commandline style with our fixed
     * GUI style type Windows, Fusion etc. */
    QString styleName;

#if defined(Q_OS_UNIX)
    styleName = QStringLiteral("Fusion");
#else
    styleName = "Windows";
#endif

    /* Configure custom GUI style hinter */
    QStyle* style;
    if ((style = QStyleFactory::create(styleName))) {
        ApplicationStyle* myStyle = new ApplicationStyle();
        myStyle->setBaseStyle(style);
        a.setStyle(myStyle);
    }

    /* Prepare basic styles UI */
    a.setStyleSheet(QStringLiteral(
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
       "*{font-size:16pt;}"
#endif
       "QPushButton {border-radius:5px;"
       "border-style:solid;"
       "border-width:1px;"
       "border-color:rgb(76,76,76);"
       "min-height:32px;min-width:130px;padding:4px,4px,4px,4px;}"
       "QPushButton::focus {border-color:white;}"
       "QPushButton::default {border-color:rgb(252,115,9);}"
       "QPushButton::pressed {border-color:rgb(252,115,9);}"
       "QPushButton::hover {border-color:rgb(252,115,9);}"
       "QPushButton::disabled {border-color:rgb(46,46,46);}"
#if defined(Q_OS_UNIX)
       "QComboBox {color: white;}"
#endif
       ));

#if defined(VSPCLIENT_LIBRARY) && defined(VSP_TARGET_NAME)
    const QString lprojPath = QStringLiteral( //
       "%1/../Frameworks/" xstr(VSP_TARGET_NAME) ".framework/Resources/%2.lproj");
#else
    const QString lprojPath = QStringLiteral("%1/../Resources/%2.lproj");
#endif

    /* Available locales */
    const QStringList uiLanguages = QLocale::system().uiLanguages();

    /* Load App translation */
    QTranslator translator;
    for (const QString& locale : uiLanguages) {
        const QLocale lc(locale);
        const QString baseName = "vspui_" + lc.name();
        const QString resPath = lprojPath.arg(a.applicationDirPath(), lc.bcp47Name());
        const QString fileName = QStringLiteral("%1.qm").arg(baseName);
        const QString fullPath = QStringLiteral("%1/%2").arg(resPath, fileName);

        // qDebug("[APPLOC] pm file: %s", qPrintable(fullPath));

        if (QFile::exists(fullPath)) {
            if (translator.load(fileName, resPath)) {
                qDebug("[APPLOC] Set translation from: %s", qPrintable(fullPath));
                a.installTranslator(&translator);
            }
            break;
        }
        else if (translator.load(":/i18n/" + baseName)) {
            qDebug("[APPLOC] (r) Set translation for: %s", qPrintable(baseName));
            a.installTranslator(&translator);
            break;
        }
    }

    VSCMainWindow w;
    w.show();

    return a.exec();
}
