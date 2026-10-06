#include "ServerInfoDialogs.h"

#include "util/CapsLabels.h"
#include "util/SoftEtherLabels.h"

#include <QCoreApplication>
#include <QJsonArray>

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("ServerInfoDialogs", text);
}

QString serverTypeText(int type)
{
    // SM_SERVER_STANDALONE / SM_FARM_CONTROLLER / SM_FARM_MEMBER
    switch (type) {
    case 1:
        return tr("クラスタコントローラ");
    case 2:
        return tr("クラスタメンバサーバー");
    default:
        return tr("スタンドアロンサーバー");
    }
}

QString yesNo(bool value)
{
    return value ? tr("はい") : tr("いいえ");
}

QString memory(double value)
{
    // SM_ST_RAM_SIZE_KB: "%S バイト" (値はバイト単位)
    return SoftEtherLabels::bytes(value);
}

} // namespace

namespace ServerInfoDialogs {

InfoTable::Rows hubStatusRows(const QJsonObject &s)
{
    using SoftEtherLabels::bytes;
    using SoftEtherLabels::number;
    using SoftEtherLabels::packets;

    InfoTable::Rows rows;
    // SM_HUB_STATUS_* / SM_HUB_NUM_* / SM_ST_* の並びを公式Managerに合わせる。
    rows << qMakePair(tr("仮想 HUB 名"), s.value("HubName_str").toString());
    rows << qMakePair(tr("状態"), SoftEtherLabels::onlineStatus(s.value("Online_bool").toBool()));
    rows << qMakePair(tr("種類"), SoftEtherLabels::hubType(s.value("HubType_u32").toInt()));
    rows << qMakePair(tr("SecureNAT 機能"), SoftEtherLabels::secureNatEnabled(s.value("SecureNATEnabled_bool").toBool()));
    rows << qMakePair(tr("セッション数"), number(s.value("NumSessions_u32").toDouble()));
    rows << qMakePair(tr("セッション数 (クライアント)"), number(s.value("NumSessionsClient_u32").toDouble()));
    rows << qMakePair(tr("セッション数 (ブリッジ)"), number(s.value("NumSessionsBridge_u32").toDouble()));
    rows << qMakePair(tr("アクセスリスト数"), number(s.value("NumAccessLists_u32").toDouble()));
    rows << qMakePair(tr("ユーザー数"), number(s.value("NumUsers_u32").toDouble()));
    rows << qMakePair(tr("グループ数"), number(s.value("NumGroups_u32").toDouble()));
    rows << qMakePair(tr("MAC テーブル数"), number(s.value("NumMacTables_u32").toDouble()));
    rows << qMakePair(tr("IP テーブル数"), number(s.value("NumIpTables_u32").toDouble()));
    rows << qMakePair(tr("ログイン回数"), number(s.value("NumLogin_u32").toDouble()));
    rows << qMakePair(tr("最終ログイン日時"), SoftEtherLabels::dateTime(s.value("LastLoginTime_dt").toString()));
    rows << qMakePair(tr("最終通信日時"), SoftEtherLabels::dateTime(s.value("LastCommTime_dt").toString()));
    rows << qMakePair(tr("作成日時"), SoftEtherLabels::dateTime(s.value("CreatedTime_dt").toString()));
    rows << qMakePair(tr("送信ユニキャストパケット数"), packets(s.value("Send.UnicastCount_u64").toDouble()));
    rows << qMakePair(tr("送信ユニキャスト合計サイズ"), bytes(s.value("Send.UnicastBytes_u64").toDouble()));
    rows << qMakePair(tr("送信ブロードキャストパケット数"), packets(s.value("Send.BroadcastCount_u64").toDouble()));
    rows << qMakePair(tr("送信ブロードキャスト合計サイズ"), bytes(s.value("Send.BroadcastBytes_u64").toDouble()));
    rows << qMakePair(tr("受信ユニキャストパケット数"), packets(s.value("Recv.UnicastCount_u64").toDouble()));
    rows << qMakePair(tr("受信ユニキャスト合計サイズ"), bytes(s.value("Recv.UnicastBytes_u64").toDouble()));
    rows << qMakePair(tr("受信ブロードキャストパケット数"), packets(s.value("Recv.BroadcastCount_u64").toDouble()));
    rows << qMakePair(tr("受信ブロードキャスト合計サイズ"), bytes(s.value("Recv.BroadcastBytes_u64").toDouble()));
    return rows;
}

InfoTableDialog *createHubStatusDialog(VpnServerRpc *rpc, const QString &hubName, QWidget *parent)
{
    // SM_HUB_STATUS_CAPTION
    return new InfoTableDialog(
        tr("仮想 HUB \"%1\" の状態").arg(hubName), tr("仮想 HUB の状態"), /*refreshable=*/true,
        [rpc, hubName](const InfoTableDialog::Deliver &deliver, const InfoTableDialog::Fail &fail) {
            rpc->getHubStatus(
                hubName, [deliver](const QJsonObject &status) { deliver(hubStatusRows(status)); },
                [fail](const RpcError &error) { fail(error); });
        },
        parent);
}

InfoTableDialog *createServerStatusDialog(VpnServerRpc *rpc, QWidget *parent)
{
    using SoftEtherLabels::bytes;
    using SoftEtherLabels::number;
    using SoftEtherLabels::packets;

    return new InfoTableDialog(
        tr("サーバー状態"), tr("サーバー状態"), /*refreshable=*/true,
        [rpc](const InfoTableDialog::Deliver &deliver, const InfoTableDialog::Fail &fail) {
            rpc->getServerStatus(
                [deliver](const QJsonObject &s) {
                    const int type = s.value("ServerType_u32").toInt();
                    const bool cluster = type != 0;
                    InfoTable::Rows rows;
                    rows << qMakePair(tr("サーバーの種類"), serverTypeText(type));
                    rows << qMakePair(tr("開いているソケット数"), number(s.value("NumTcpConnections_u32").toDouble()));
                    if (cluster) {
                        rows << qMakePair(tr("このサーバーのソケット数"), number(s.value("NumTcpConnectionsLocal_u32").toDouble()));
                        rows << qMakePair(tr("他のクラスタメンバサーバーのソケット数合計"),
                                          number(s.value("NumTcpConnectionsRemote_u32").toDouble()));
                    }
                    rows << qMakePair(tr("仮想 HUB 数"), number(s.value("NumHubTotal_u32").toDouble()));
                    if (cluster) {
                        rows << qMakePair(tr("スタティック仮想 HUB 数"), number(s.value("NumHubStatic_u32").toDouble()));
                        rows << qMakePair(tr("ダイナミック仮想 HUB 数"), number(s.value("NumHubDynamic_u32").toDouble()));
                    }
                    rows << qMakePair(tr("セッション数"), number(s.value("NumSessionsTotal_u32").toDouble()));
                    if (cluster) {
                        rows << qMakePair(tr("このサーバーのセッション数"), number(s.value("NumSessionsLocal_u32").toDouble()));
                        rows << qMakePair(tr("他のクラスタメンバサーバーのセッション数"),
                                          number(s.value("NumSessionsRemote_u32").toDouble()));
                    }
                    rows << qMakePair(tr("MAC アドレステーブル数"), number(s.value("NumMacTables_u32").toDouble()));
                    rows << qMakePair(tr("IP アドレステーブル数"), number(s.value("NumIpTables_u32").toDouble()));
                    rows << qMakePair(tr("ユーザー数"), number(s.value("NumUsers_u32").toDouble()));
                    rows << qMakePair(tr("グループ数"), number(s.value("NumGroups_u32").toDouble()));
                    rows << qMakePair(tr("消費クライアント接続ライセンス数 (このサーバー)"),
                                      number(s.value("AssignedClientLicenses_u32").toDouble()));
                    rows << qMakePair(tr("消費ブリッジ接続ライセンス数 (このサーバー)"),
                                      number(s.value("AssignedBridgeLicenses_u32").toDouble()));
                    if (cluster) {
                        rows << qMakePair(tr("消費クライアント接続ライセンス数 (クラスタ全体)"),
                                          number(s.value("AssignedClientLicensesTotal_u32").toDouble()));
                        rows << qMakePair(tr("消費ブリッジ接続ライセンス数 (クラスタ全体)"),
                                          number(s.value("AssignedBridgeLicensesTotal_u32").toDouble()));
                    }
                    rows << qMakePair(tr("送信ユニキャストパケット数"), packets(s.value("Send.UnicastCount_u64").toDouble()));
                    rows << qMakePair(tr("送信ユニキャスト合計サイズ"), bytes(s.value("Send.UnicastBytes_u64").toDouble()));
                    rows << qMakePair(tr("送信ブロードキャストパケット数"), packets(s.value("Send.BroadcastCount_u64").toDouble()));
                    rows << qMakePair(tr("送信ブロードキャスト合計サイズ"), bytes(s.value("Send.BroadcastBytes_u64").toDouble()));
                    rows << qMakePair(tr("受信ユニキャストパケット数"), packets(s.value("Recv.UnicastCount_u64").toDouble()));
                    rows << qMakePair(tr("受信ユニキャスト合計サイズ"), bytes(s.value("Recv.UnicastBytes_u64").toDouble()));
                    rows << qMakePair(tr("受信ブロードキャストパケット数"), packets(s.value("Recv.BroadcastCount_u64").toDouble()));
                    rows << qMakePair(tr("受信ブロードキャスト合計サイズ"), bytes(s.value("Recv.BroadcastBytes_u64").toDouble()));
                    rows << qMakePair(tr("サーバー起動時刻"), SoftEtherLabels::dateTime(s.value("StartTime_dt").toString()));
                    rows << qMakePair(tr("現在時刻"), SoftEtherLabels::dateTime(s.value("CurrentTime_dt").toString()));
                    rows << qMakePair(tr("64 bit 高精度論理システム時刻"), number(s.value("CurrentTick_u64").toDouble()));
                    rows << qMakePair(tr("合計論理メモリサイズ"), memory(s.value("TotalMemory_u64").toDouble()));
                    rows << qMakePair(tr("使用中の論理メモリサイズ"), memory(s.value("UsedMemory_u64").toDouble()));
                    rows << qMakePair(tr("空き論理メモリサイズ"), memory(s.value("FreeMemory_u64").toDouble()));
                    rows << qMakePair(tr("合計物理メモリサイズ"), memory(s.value("TotalPhys_u64").toDouble()));
                    rows << qMakePair(tr("使用中の物理メモリサイズ"), memory(s.value("UsedPhys_u64").toDouble()));
                    rows << qMakePair(tr("空き物理メモリサイズ"), memory(s.value("FreePhys_u64").toDouble()));
                    deliver(rows);
                },
                [fail](const RpcError &error) { fail(error); });
        },
        parent);
}

InfoTableDialog *createServerInfoDialog(VpnServerRpc *rpc, QWidget *parent)
{
    return new InfoTableDialog(
        tr("接続先 VPN Server バージョン情報"), tr("接続先 VPN Server バージョン情報"), /*refreshable=*/false,
        [rpc](const InfoTableDialog::Deliver &deliver, const InfoTableDialog::Fail &fail) {
            rpc->getServerInfo(
                [rpc, deliver, fail](const QJsonObject &info) {
                    InfoTable::Rows rows;
                    rows << qMakePair(tr("製品名"), info.value("ServerProductName_str").toString());
                    rows << qMakePair(tr("バージョン情報"), info.value("ServerVersionString_str").toString());
                    rows << qMakePair(tr("ビルド情報"), info.value("ServerBuildInfoString_str").toString());
                    rows << qMakePair(tr("ホスト名"), info.value("ServerHostName_str").toString());
                    rows << qMakePair(tr("サーバーの種類"), serverTypeText(info.value("ServerType_u32").toInt()));
                    rows << qMakePair(tr("オペレーティングシステム種類"), info.value("OsSystemName_str").toString());
                    rows << qMakePair(tr("オペレーティングシステム製品名"), info.value("OsProductName_str").toString());
                    rows << qMakePair(tr("オペレーティングシステム製造元"), info.value("OsVendorName_str").toString());
                    rows << qMakePair(tr("オペレーティングシステムバージョン"), info.value("OsVersion_str").toString());
                    rows << qMakePair(tr("OS カーネル名"), info.value("KernelName_str").toString());
                    rows << qMakePair(tr("OS カーネルバージョン"), info.value("KernelVersion_str").toString());

                    // GetCaps の項目名は "b_xxx" (真偽値) / "i_xxx" (数値)。表示名は strtable の CT_* に対応する。
                    rpc->call(
                        QStringLiteral("GetCaps"), {},
                        [rows, deliver](const QJsonObject &result) mutable {
                            for (const QJsonValue &value : result.value("CapsList").toArray()) {
                                const QJsonObject caps = value.toObject();
                                const QString name = caps.value("CapsName_str").toString();
                                const QString label = CapsLabels::table().value(name, caps.value("CapsDescrption_utf").toString());
                                const int number = caps.value("CapsValue_u32").toInt();
                                rows << qMakePair(label.isEmpty() ? name : label,
                                                  name.startsWith(QLatin1String("b_")) ? yesNo(number != 0) : QString::number(number));
                            }
                            deliver(rows);
                        },
                        [rows, deliver](const RpcError &) { deliver(rows); });
                },
                [fail](const RpcError &error) { fail(error); });
        },
        parent);
}

} // namespace ServerInfoDialogs
