#pragma once

#include <QDialog>
#include <QLayout>

namespace DialogSizing {

// 折り返しラベルを含むダイアログでは sizeHint().height() が低く出てラベルが潰れるため、
// 指定幅に対するレイアウトの高さ (heightForWidth) を使って大きさを決める。
inline void fitToWidth(QDialog *dialog, int width)
{
    int height = dialog->sizeHint().height();
    if (QLayout *layout = dialog->layout()) {
        layout->activate();
        if (layout->hasHeightForWidth()) {
            const QMargins margins = layout->contentsMargins();
            height = qMax(height, layout->totalHeightForWidth(width) + margins.top() + margins.bottom());
        }
    }
    dialog->resize(width, height);
}

} // namespace DialogSizing
