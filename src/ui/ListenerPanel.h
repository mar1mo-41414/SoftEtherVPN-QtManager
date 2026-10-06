#pragma once

#include "rpc/VpnServerRpc.h"

#include <QGroupBox>

class QPushButton;
class QTableWidget;

// 公式Manager D_SM_SERVER 内の「リスナーの管理(J)」セクション相当。
// ポート一覧 + 新規作成/削除/開始/停止を、サーバー管理画面に埋め込む部品として提供する。
class ListenerPanel : public QGroupBox
{
    Q_OBJECT

public:
    explicit ListenerPanel(QWidget *parent = nullptr);

    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。nullptrを渡すと表示を空にする。
    void setRpc(VpnServerRpc *rpc);
    void reload();
    // サーバー管理権限が無い接続では操作させない。
    void setOperable(bool operable);

private slots:
    void onCreate();
    void onDelete();
    void onStart();
    void onStop();
    void updateButtons();

private:
    int selectedPort(bool *running) const;

    VpnServerRpc *m_rpc = nullptr;
    bool m_operable = true;
    QTableWidget *m_table;
    QPushButton *m_createButton;
    QPushButton *m_deleteButton;
    QPushButton *m_startButton;
    QPushButton *m_stopButton;
};
