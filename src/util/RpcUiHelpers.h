#pragma once

#include "rpc/VpnServerRpc.h"

#include <QComboBox>
#include <QJsonArray>
#include <QMessageBox>
#include <QPointer>
#include <QWidget>

namespace RpcUi {

inline void showError(QWidget *parent, const QString &what, const RpcError &error)
{
    QMessageBox::warning(parent, QObject::tr("エラー"),
                          QObject::tr("%1に失敗しました: %2 (code %3)").arg(what, error.message).arg(error.code));
}

// 仮想HUB名の一覧をEnumHubで取得してコンボボックスへ流し込む。
// 取得完了後にselectNameが一覧にあればそれを、無ければ編集欄にそのまま設定する。
inline void populateHubCombo(VpnServerRpc *rpc, QComboBox *combo, const QString &selectName = QString())
{
    QPointer<QComboBox> guard(combo);
    rpc->enumHub(
        [guard, selectName](const QJsonObject &result) {
            if (!guard) {
                return;
            }
            const QJsonArray hubs = result.value("HubList").toArray();
            for (const QJsonValue &value : hubs) {
                guard->addItem(value.toObject().value("HubName_str").toString());
            }
            if (!selectName.isEmpty()) {
                const int index = guard->findText(selectName);
                if (index >= 0) {
                    guard->setCurrentIndex(index);
                } else if (guard->isEditable()) {
                    guard->setEditText(selectName);
                }
            }
        },
        [](const RpcError &) {});
}

} // namespace RpcUi
