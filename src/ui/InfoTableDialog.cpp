#include "InfoTableDialog.h"

#include "util/RpcUiHelpers.h"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace InfoTable {

QTableWidget *makeTable(QWidget *parent)
{
    auto *table = new QTableWidget(parent);
    table->setColumnCount(2);
    // SM_STATUS_COLUMN_1/2
    table->setHorizontalHeaderLabels({QObject::tr("項目"), QObject::tr("値")});
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->hide();
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    return table;
}

void setRows(QTableWidget *table, const Rows &rows)
{
    table->setRowCount(rows.size());
    for (int row = 0; row < rows.size(); ++row) {
        table->setItem(row, 0, new QTableWidgetItem(rows.at(row).first));
        table->setItem(row, 1, new QTableWidgetItem(rows.at(row).second));
    }
    table->resizeColumnToContents(0);
}

} // namespace InfoTable

InfoTableDialog::InfoTableDialog(const QString &windowTitle, const QString &heading, bool refreshable, Loader loader,
                                 QWidget *parent)
    : QDialog(parent)
    , m_loader(std::move(loader))
{
    setWindowTitle(windowTitle);

    auto *headingLabel = new QLabel(heading, this);
    QFont headingFont = headingLabel->font();
    headingFont.setBold(true);
    headingFont.setPointSize(headingFont.pointSize() + 6);
    headingLabel->setFont(headingFont);

    m_table = InfoTable::makeTable(this);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    if (refreshable) {
        auto *refreshButton = new QPushButton(tr("最新の状態に更新(&H)"), this);
        connect(refreshButton, &QPushButton::clicked, this, &InfoTableDialog::reload);
        buttonLayout->addWidget(refreshButton);
    }
    auto *closeButton = new QPushButton(tr("閉じる(&X)"), this);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeButton);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(headingLabel);
    layout->addWidget(m_table, 1);
    layout->addLayout(buttonLayout);

    resize(640, 560);
    reload();
}

void InfoTableDialog::reload()
{
    m_loader([this](const InfoTable::Rows &rows) { InfoTable::setRows(m_table, rows); },
             [this](const RpcError &error) { RpcUi::showError(this, tr("情報の取得"), error); });
}
