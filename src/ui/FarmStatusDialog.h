#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QPushButton;
class QTableWidget;

// 公式Manager「クラスタメンバ一覧」(D_SM_FARM_MEMBER) と
// 「クラスタコントローラへの接続状態」(SM_FC_*) 相当。
// サーバーの動作モードに応じて表示内容を切り替える。
class FarmStatusDialog : public QDialog
{
    Q_OBJECT

public:
    // isController==true: クラスタメンバ一覧、false: コントローラへの接続状態を表示する。
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    FarmStatusDialog(VpnServerRpc *rpc, bool isController, QWidget *parent = nullptr);

private slots:
    void onShowMemberInfo();

private:
    void buildControllerView();
    void buildMemberView();
    void reloadMembers();

    VpnServerRpc *m_rpc;
    QTableWidget *m_table = nullptr;
    QPushButton *m_infoButton = nullptr;
};
