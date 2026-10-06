#pragma once

#include "rpc/VpnServerRpc.h"

#include <QDialog>
#include <QJsonObject>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QSpinBox;

// 公式Manager「アクセスリスト項目の編集 (IPv4 / IPv6)」(D_SM_EDIT_ACCESS) 相当。
// IPヘッダ・MACヘッダ・TCP/UDPヘッダ・ユーザー/グループ・TCP状態・HTTPリダイレクト・
// 遅延/ジッタ/パケットロス生成のすべての条件を扱う。
class AccessEditDialog : public QDialog
{
    Q_OBJECT

public:
    // rpc/hubName は「参照...」(ユーザー/グループの選択) でのみ使う (借用)。
    AccessEditDialog(VpnServerRpc *rpc, const QString &hubName, bool ipv6, QWidget *parent = nullptr);

    // 新規項目の優先順位の初期値。
    void setPriority(int priority);
    // 既存項目をフォームに反映する。
    void setValues(const QJsonObject &access);
    // AddAccess / SetAccessList にそのまま渡せる1項目分のJSON。
    QJsonObject toRpcParams() const;

    // 一覧の「内容」列に表示する公式Manager形式の文字列
    // (例: "(ipv4) SrcIPv4=192.0.2.2/32, Protocol=TCP, SrcPort=99, DstPort=99")。
    static QString describe(const QJsonObject &access);

private slots:
    void accept() override;
    void updateState();
    void onPickSrcUser();
    void onPickDstUser();
    void onRedirect();
    void onSimulation();

private:
    void pickUser(QLineEdit *target);
    int protocolNumber() const;

    VpnServerRpc *m_rpc;
    QString m_hubName;
    bool m_ipv6;

    // 項目として保持するが、このダイアログの外側 (一覧) で管理するもの
    quint32 m_id = 0;
    bool m_active = true;

    QLineEdit *m_noteEdit;
    QRadioButton *m_passRadio;
    QRadioButton *m_discardRadio;
    QSpinBox *m_prioritySpin;

    QLineEdit *m_srcUserEdit;
    QLineEdit *m_dstUserEdit;

    QCheckBox *m_srcMacAllCheck;
    QLineEdit *m_srcMacEdit;
    QLineEdit *m_srcMacMaskEdit;
    QCheckBox *m_dstMacAllCheck;
    QLineEdit *m_dstMacEdit;
    QLineEdit *m_dstMacMaskEdit;

    QCheckBox *m_srcAllCheck;
    QLineEdit *m_srcIpEdit;
    QLineEdit *m_srcMaskEdit;
    QCheckBox *m_dstAllCheck;
    QLineEdit *m_dstIpEdit;
    QLineEdit *m_dstMaskEdit;
    QComboBox *m_protocolCombo;
    QLineEdit *m_protocolNumberEdit;

    QLineEdit *m_srcPortMinEdit;
    QLineEdit *m_srcPortMaxEdit;
    QLineEdit *m_dstPortMinEdit;
    QLineEdit *m_dstPortMaxEdit;
    QCheckBox *m_tcpStateCheck;
    QRadioButton *m_establishedRadio;
    QRadioButton *m_unestablishedRadio;

    QCheckBox *m_redirectCheck;
    QPushButton *m_redirectButton;
    QString m_redirectUrl;
    int m_delay = 0;
    int m_jitter = 0;
    int m_loss = 0;
};
