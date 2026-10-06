#pragma once

#include "rpc/VpnServerRpc.h"

#include <QComboBox>
#include <QJsonArray>
#include <QMessageBox>
#include <QPointer>
#include <QWidget>

#include <utility>

namespace RpcUi {

// 非同期RPCのコールバックを、owner (ダイアログ/ページ) が破棄された後は呼ばないようにするラッパー。
// ダイアログが閉じた後にRPC応答が返ると、破棄済みオブジェクトを触ってクラッシュするのを防ぐ。
//   m_rpc->enumUser(hub, RpcUi::guarded(this, [this](const QJsonObject &r) { ... }), RpcUi::guarded(this, [this](const RpcError &e) { ... }));
template <typename F>
auto guarded(QObject *owner, F callback)
{
    return [guard = QPointer<QObject>(owner), callback = std::move(callback)](auto &&...args) mutable {
        if (guard) {
            callback(std::forward<decltype(args)>(args)...);
        }
    };
}

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
