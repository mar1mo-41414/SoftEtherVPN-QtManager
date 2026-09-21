#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>
#include <QStringList>

class QComboBox;
class QLineEdit;
class QRadioButton;
class QTableWidget;

// 公式Manager「ローカルブリッジ設定」(D_SM_BRIDGE) 相当。サーバー全体の設定。
// タグVLANパケット透過設定ツールは後続フェーズで追加する。
class LocalBridgeDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。hubNamesは新規追加時の仮想HUB選択肢。
    LocalBridgeDialog(VpnServerRpc *rpc, QStringList hubNames, QWidget *parent = nullptr);

private slots:
    void onAdd();
    void onDelete();
    void onBridgeTypeToggled(bool usePhysical);

private:
    void reload();
    void loadEthernetList();

    VpnServerRpc *m_rpc;
    QStringList m_hubNames;
    QTableWidget *m_table;

    QComboBox *m_hubCombo;
    QRadioButton *m_physicalRadio;
    QRadioButton *m_tapRadio;
    QComboBox *m_ethernetCombo;
    QLineEdit *m_tapNameEdit;
};
