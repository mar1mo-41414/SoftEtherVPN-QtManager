#include "ui/MainWindow.h"

#include <QApplication>
#include <QFormLayout>
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

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
