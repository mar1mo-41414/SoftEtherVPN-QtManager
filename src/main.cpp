#include "ui/MainWindow.h"

#include <QApplication>
#include <QFormLayout>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>
#include <QProxyStyle>
#include <QStyleFactory>

namespace {

// macOS スタイルの QFormLayout は入力欄が sizeHint のまま広がらず、
// 公式Manager (Windows) のように欄が列幅いっぱいに広がる見た目にならないため、
// 入力欄を広げる設定に差し替える。
class FormGrowStyle : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;

    int styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                  QStyleHintReturn *returnData) const override
    {
        if (hint == SH_FormLayoutFieldGrowthPolicy) {
            return QFormLayout::AllNonFixedFieldsGrow;
        }
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("SoftEtherVPN-QtManager"));
    app.setOrganizationName(QStringLiteral("SoftEtherVPN-QtManager"));
    app.setStyle(new FormGrowStyle(app.style()->name()));

    // UI文言の原文(tr()の第一引数)は日本語。システムのロケールが日本語以外であれば
    // i18n/SoftEtherVPN-QtManager_en.ts (CMakeで.qmにコンパイルしリソース埋め込み) から
    // 英語訳を読み込む。未訳の文字列は日本語のまま表示される。
    // 環境変数 SEQTM_LANG でロケールを明示指定できる (例: "en"、"ja")。テストや、OSのロケール
    // 判定がうまくいかない環境向け。未指定ならシステムのロケールに従う。
    QLocale uiLocale = QLocale::system();
    const QByteArray forcedLang = qgetenv("SEQTM_LANG");
    if (!forcedLang.isEmpty()) {
        uiLocale = QLocale(QString::fromLocal8Bit(forcedLang));
    }

    QTranslator translator;
    if (uiLocale.language() != QLocale::Japanese &&
        translator.load(uiLocale, QStringLiteral("SoftEtherVPN-QtManager"), QStringLiteral("_"), QStringLiteral(":/i18n"))) {
        app.installTranslator(&translator);
    }

    QTranslator qtBaseTranslator;
    if (qtBaseTranslator.load(uiLocale, QStringLiteral("qtbase"), QStringLiteral("_"),
                               QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        app.installTranslator(&qtBaseTranslator);
    }

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
