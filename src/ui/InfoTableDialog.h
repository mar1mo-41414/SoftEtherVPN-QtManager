#pragma once

#include "rpc/JsonRpcClient.h"

#include <QDialog>
#include <QList>
#include <QPair>

#include <functional>

class QTableWidget;

namespace InfoTable {

using Row = QPair<QString, QString>;
using Rows = QList<Row>;

// 「項目 / 値」の2列テーブルを作る (公式Managerの状態表示と同じ体裁)。
QTableWidget *makeTable(QWidget *parent);
void setRows(QTableWidget *table, const Rows &rows);

} // namespace InfoTable

// 公式Managerの「サーバー状態」「バージョン情報」「仮想HUBの状態」のような、
// 見出し + 項目/値テーブル + [最新の状態に更新] [閉じる] で構成される読み取り専用ダイアログ。
class InfoTableDialog : public QDialog
{
    Q_OBJECT

public:
    using Deliver = std::function<void(const InfoTable::Rows &)>;
    using Fail = std::function<void(const RpcError &)>;
    // loader: 内容を非同期に取得し、完了したら deliver か fail を呼ぶ。
    using Loader = std::function<void(const Deliver &, const Fail &)>;

    InfoTableDialog(const QString &windowTitle, const QString &heading, bool refreshable, Loader loader,
                    QWidget *parent = nullptr);

private:
    void reload();

    Loader m_loader;
    QTableWidget *m_table;
};
