#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>

class QPlainTextEdit;

// 公式Manager「Config ファイルの編集」(D_SM_CONFIG) 相当。
// サーバーの現在のコンフィグレーションファイルを表示し、ファイルへの保存・ファイルからの書き込みができる。
// 「設定をリセットして初期化」は対応するAPIが無いため未対応。
class ConfigEditDialog : public QDialog
{
    Q_OBJECT

public:
    // rpcの所有権は借用のみ (呼び出し側が生存管理する)。
    ConfigEditDialog(VpnServerRpc *rpc, QString serverName, QWidget *parent = nullptr);

private slots:
    void onExport();
    void onImport();

private:
    VpnServerRpc *m_rpc;
    QString m_fileName;
    QPlainTextEdit *m_text;
};
