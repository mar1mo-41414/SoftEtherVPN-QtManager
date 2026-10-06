#pragma once

// 自動生成: docs/upstream-reference/strtable_ja.stb の POL_n / POL_EX_n から。
// 公式Managerのセキュリティポリシー一覧 (SoftEther の POLICY 構造体の並び順)。

namespace PolicyTable {

enum class Kind { Bool, Int };
enum class Unit { None, Count, Sec, Bps, Vlan };

struct Def {
    const char *key;  // JSON-RPC上のキー ("policy:" 以降、_bool/_u32 の前まで)
    Kind kind;
    Unit unit;
    unsigned min;
    unsigned max;
    bool canDisable;
    const char *name;
    const char *description;
};

inline const Def *defs(int *count)
{
    static const Def table[] = {
        {"Access", Kind::Bool, Unit::None, 0u, 0u, false,
         "アクセスを許可",
         "このポリシーが設定されているユーザーは、VPN Server に VPN 接続することを許可されます。"},
        {"DHCPFilter", Kind::Bool, Unit::None, 0u, 0u, false,
         "DHCP パケットをフィルタリング (IPv4)",
         "このポリシーが設定されているセッションにおける IPv4 における DHCP パケットをすべてフィルタリングします。"},
        {"DHCPNoServer", Kind::Bool, Unit::None, 0u, 0u, false,
         "DHCP サーバーの動作を禁止 (IPv4)",
         "このポリシーが設定されているセッションに接続しているコンピュータが DHCP サーバーとなり IPv4 アドレスや DNS サーバーの情報などを IPv4 DHCP クライアントに配布することを禁止します。"},
        {"DHCPForce", Kind::Bool, Unit::None, 0u, 0u, false,
         "DHCP が割り当てた IP アドレスを強制 (IPv4)",
         "このポリシーが設定されているセッション内の IPv4 コンピュータは、仮想ネットワーク側の DHCP サーバーが割り当てを行った IPv4 アドレスしか利用できないようにします。"},
        {"NoBridge", Kind::Bool, Unit::None, 0u, 0u, false,
         "ブリッジを禁止",
         "このポリシーが設定されているユーザーのセッションでは、ブリッジ接続を禁止します。ユーザーのクライアント側に Ethernet ブリッジが設定されていても、通信ができなくなります。"},
        {"NoRouting", Kind::Bool, Unit::None, 0u, 0u, false,
         "ルータ動作を禁止 (IPv4)",
         "このポリシーが設定されているセッションでは、IPv4 ルーティングを禁止します。ユーザーのクライアント側で IP ルータが動作していても、通信ができなくなります。"},
        {"CheckMac", Kind::Bool, Unit::None, 0u, 0u, false,
         "MAC アドレスの重複を禁止",
         "このポリシーが設定されているセッションでは、別のセッションのコンピュータが使用中の MAC アドレスを使用することができないようにします。"},
        {"CheckIP", Kind::Bool, Unit::None, 0u, 0u, false,
         "IP アドレスの重複を禁止 (IPv4)",
         "このポリシーが設定されているセッションでは、別のセッションのコンピュータが使用中の IPv4 アドレスを重複して使用することができないようにします。"},
        {"ArpDhcpOnly", Kind::Bool, Unit::None, 0u, 0u, false,
         "ARP・DHCP・ICMPv6 以外のブロードキャストを禁止",
         "このポリシーが設定されているセッションでは、仮想ネットワークに対して IPv4 における ARP プロトコルと DHCP プロトコルおよび IPv6 における ICMPv6 プロトコルによるブロードキャストパケット以外のすべてのブロードキャストパケットの送受信を禁止します。"},
        {"PrivacyFilter", Kind::Bool, Unit::None, 0u, 0u, false,
         "プライバシーフィルタモード",
         "プライバシーフィルタモードポリシーが設定されているセッション間における直接的な通信をすべてフィルタリングします。"},
        {"NoServer", Kind::Bool, Unit::None, 0u, 0u, false,
         "TCP/IP サーバーとしての動作を禁止 (IPv4)",
         "このポリシーが設定されているセッションのコンピュータが TCP/IP プロトコルにおけるサーバーとしての動作を行うことを禁止します。"},
        {"NoBroadcastLimiter", Kind::Bool, Unit::None, 0u, 0u, false,
         "ブロードキャスト数を制限しない",
         "このポリシーが設定されているセッションのコンピュータが通常は考えられないような異常な数のブロードキャストパケットを仮想ネットワークに送出しても自動的に制限しないようにします。"},
        {"MonitorPort", Kind::Bool, Unit::None, 0u, 0u, false,
         "モニタリングモードを許可",
         "このポリシーが設定されているユーザーはモニタリングモードで仮想 HUB に接続することができます。モニタリングモードのセッションは仮想 HUB 内を流れるすべてのパケットをモニタリング (傍受) することができます。"},
        {"MaxConnection", Kind::Int, Unit::Count, 1u, 32u, false,
         "TCP コネクション数の最大値",
         "このポリシーが設定されているセッションのセッション１つあたりに割り当てることができる物理的な TCP コネクション数の最大数を設定します。"},
        {"TimeOut", Kind::Int, Unit::Sec, 5u, 60u, false,
         "通信タイムアウト時間",
         "このポリシーが設定されているセッションにおいて VPN Client / VPN Server 間の通信に障害が発生した場合、セッションを切断するまでのタイムアウト時間を秒単位で設定します。"},
        {"MaxMac", Kind::Int, Unit::Count, 1u, 65535u, true,
         "MAC アドレスの上限数",
         "このポリシーが設定されているセッションの１セッションあたりに登録することができる MAC アドレスの数を指定します。"},
        {"MaxIP", Kind::Int, Unit::Count, 1u, 65535u, true,
         "IP アドレスの上限数 (IPv4)",
         "このポリシーが設定されているセッションの１セッションあたりに登録することができる IPv4 アドレスの数を指定します。"},
        {"MaxUpload", Kind::Int, Unit::Bps, 1u, 2147483647u, true,
         "アップロード帯域幅",
         "このポリシーが設定されているセッションにおける仮想 HUB の外側から仮想 HUB の内側方向に入ってくるトラフィックの帯域幅を制限します。"},
        {"MaxDownload", Kind::Int, Unit::Bps, 1u, 2147483647u, true,
         "ダウンロード帯域幅",
         "このポリシーが設定されているセッションにおける仮想 HUB の内側から仮想 HUB の外側方向に出ていくトラフィックの帯域幅を制限します。"},
        {"FixPassword", Kind::Bool, Unit::None, 0u, 0u, false,
         "ユーザーはパスワードを変更できない",
         "このポリシーが設定されているユーザーがパスワード認証の場合、ユーザーが VPN クライアント接続マネージャなどから自分のパスワードを変更することを禁止します。"},
        {"MultiLogins", Kind::Int, Unit::Count, 1u, 65535u, true,
         "多重ログイン制限数",
         "このポリシーが設定されているユーザーが設定されている数以上の同時ログインを行うことを禁止します。ブリッジモードセッションにはこの制限は適用されません。このセキュリティポリシーは、VPN Server 3.0 以降、または多重ログイン制限機能が搭載されている VPN Server 2.0 でのみ有効です。"},
        {"NoQoS", Kind::Bool, Unit::None, 0u, 0u, false,
         "VoIP / QoS 対応機能の使用を禁止",
         "このポリシーが設定されているユーザーの VPN 接続セッションにおいて VoIP / QoS 対応機能の使用を禁止します。このセキュリティポリシーは、VPN Server 3.0 以降、または VoIP / QoS 対応機能が搭載されている VPN Server 2.0 でのみ有効です。"},
        {"RSandRAFilter", Kind::Bool, Unit::None, 0u, 0u, false,
         "ルータ要請/広告パケットをフィルタリング (IPv6)",
         "このポリシーが設定されているセッションにおける IPv6 における ICMPv6 パケットのうち、メッセージの種類が 133 (ルータ要請) および 134 (ルータ広告) であるすべてのパケットをフィルタリングします。これにより、IPv6 クライアントは IPv6 における IP アドレスプレフィックス自動検出機能およびデフォルトゲートウェイ自動検出機能を利用することができなくなります。"},
        {"RAFilter", Kind::Bool, Unit::None, 0u, 0u, false,
         "ルータ広告パケットをフィルタリング (IPv6)",
         "このポリシーが設定されているセッションに接続されている IPv6 ルータが仮想 HUB に対して発信したすべての ICMPv6 パケットのうち、メッセージの種類が 134 (ルータ広告) であるすべてのパケットをフィルタリングします。これにより、悪意のあるユーザーが不正なプレフィックスおよびデフォルトゲートウェイ情報をネットワークに流すことを禁止できます。"},
        {"DHCPv6Filter", Kind::Bool, Unit::None, 0u, 0u, false,
         "DHCP パケットをフィルタリング (IPv6)",
         "このポリシーが設定されているセッションにおける IPv6 における DHCP パケットをすべてフィルタリングします。"},
        {"DHCPv6NoServer", Kind::Bool, Unit::None, 0u, 0u, false,
         "DHCP サーバーの動作を禁止 (IPv6)",
         "このポリシーが設定されているセッションに接続しているコンピュータが DHCP サーバーとなり IPv6 アドレスや DNS サーバーの情報などを IPv6 DHCP クライアントに配布することを禁止します。"},
        {"NoRoutingV6", Kind::Bool, Unit::None, 0u, 0u, false,
         "ルータ動作を禁止 (IPv6)",
         "このポリシーが設定されているセッションでは、IPv6 ルーティングを禁止します。ユーザーのクライアント側で IP ルータが動作していても、通信ができなくなります。"},
        {"CheckIPv6", Kind::Bool, Unit::None, 0u, 0u, false,
         "IP アドレスの重複を禁止 (IPv6)",
         "このポリシーが設定されているセッションでは、別のセッションのコンピュータが使用中の IPv6 アドレスを重複して使用することができないようにします。"},
        {"NoServerV6", Kind::Bool, Unit::None, 0u, 0u, false,
         "TCP/IP サーバーとしての動作を禁止 (IPv6)",
         "このポリシーが設定されているセッションのコンピュータが TCP/IP プロトコルにおけるサーバーとしての動作を行うことを禁止します。"},
        {"MaxIPv6", Kind::Int, Unit::Count, 1u, 65535u, true,
         "IP アドレスの上限数 (IPv6)",
         "このポリシーが設定されているセッションの１セッションあたりに登録することができる IPv6 アドレスの数を指定します。IPv6 クライアントは一般的に複数個の IPv6 一時アドレスを利用することがあるため、1 セッションあたりに接続するコンピュータの台数が 1 台だけであったとしても、この値は少なくとも 20 以上に設定することを推奨します。"},
        {"NoSavePassword", Kind::Bool, Unit::None, 0u, 0u, false,
         "VPN Client でパスワードの保存を禁止",
         "このポリシーが設定されているユーザーとして VPN 接続してきた VPN Client は、ユーザー認証の方式がパスワード認証である場合において、パスワードを記憶して保存することができなくなります。これにより、ユーザーは VPN 接続を行う都度パスワードの入力を求められるようになり、セキュリティが向上します。なお、このポリシーが有効な場合は、VPN Client のバージョン 2.0 の古いクライアント PC は接続を拒否されるようになります。"},
        {"AutoDisconnect", Kind::Int, Unit::Sec, 1u, 2147483647u, true,
         "VPN Client を一定時間で自動切断",
         "このポリシーが設定されている場合、VPN 接続してきた VPN Client は、接続後、指定された秒数が経過すると、自動的に VPN 接続を切断します。この場合は、自動再接続は実施されません。これにより、アクティブでないユーザーによる大量の VPN 接続を禁止することができます。なお、このポリシーが有効な場合は、VPN Client のバージョン 2.0 の古いクライアント PC は接続を拒否されるようになります。"},
        {"FilterIPv4", Kind::Bool, Unit::None, 0u, 0u, false,
         "IPv4 パケットをすべてフィルタリング",
         "このポリシーが設定されているセッションでは、すべての IPv4 パケットの送受信がフィルタリングされ遮断されます。また、ARP パケットの送受信も禁止されます。"},
        {"FilterIPv6", Kind::Bool, Unit::None, 0u, 0u, false,
         "IPv6 パケットをすべてフィルタリング",
         "このポリシーが設定されているセッションでは、すべての IPv6 パケットの送受信がフィルタリングされ遮断されます。"},
        {"FilterNonIP", Kind::Bool, Unit::None, 0u, 0u, false,
         "非 IP パケットをすべてフィルタリング",
         "このポリシーが設定されているセッションでは、すべての非 IP パケット (IPv4, ARP, IPv6 以外の種類のパケット) の送受信がフィルタリングされ遮断されます。なお、仮想 HUB を通過するすべてのタグ VLAN パケットは非 IP パケットとしてみなされます。"},
        {"NoIPv6DefaultRouterInRA", Kind::Bool, Unit::None, 0u, 0u, false,
         "IPv6 ルータ広告からデフォルトルータ指定を削除",
         "このポリシーが設定されているセッションに対して、仮想 HUB の他のセッションの IPv6 ルータが発信する IPv6 ルータ広告メッセージのルータ有効期間の値が 0 以外の数値の場合、この値を強制的に 0 に書き換えて伝送します。これにより、VPN クライアントコンピュータが VPN 接続した先のネットワークに存在するルータをデフォルトルータとして利用することにより物理的な IPv6 通信が途切れてしまう誤作動を防止することができます。"},
        {"NoIPv6DefaultRouterInRAWhenIPv6", Kind::Bool, Unit::None, 0u, 0u, false,
         "IPv6 ルータ広告からデフォルトルータ指定を削除 (IPv6 接続時自動有効化)",
         "[IPv6 ルータ広告からデフォルトルータ指定を削除] ポリシーが無効である場合でも、VPN Client または VPN Bridge から VPN Server に対する接続および通信に利用する物理的なプロトコルが IPv6 の場合には自動的に [IPv6 ルータ広告からデフォルトルータ指定を削除] ポリシーが有効に設定されているものとみなして動作するようにします。"},
        {"VLanId", Kind::Int, Unit::Vlan, 1u, 4095u, true,
         "VLAN ID (IEEE802.1Q)",
         "このポリシーで VLAN ID を設定することができます。VLAN ID ポリシーが設定されているセッションでは、そのセッションのユーザーが仮想 HUB に対して送信するすべての Ethernet フレームに自動的に VLAN タグ (IEEE 802.1Q 準拠) が付加されます。また、そのセッションのユーザーは同一の VLAN ID が書き込まれた VLAN タグ付きのフレームのみを受信することができます (受信の際には、自動的に VLAN タグは除去されます)。他の ID の VLAN タグが付いているか、または VLAN タグが付いていないフレームは受信できません。VLAN ID ポリシーが設定されていないセッションでは、すべての Ethernet フレームが送受信でき、VLAN タグの自動付与や除去は実施されません。なお、仮想 HUB を通過するすべてのタグ VLAN パケットは非 IP パケットとしてみなされます。また、タグ VLAN パケットは仮想 HUB における IPv4 / IPv6 に関係するセキュリティポリシー、アクセスリストおよびその他の IPv4 / IPv6 パケット固有の処理の適用対象となりません。"},
    };
    *count = static_cast<int>(sizeof(table) / sizeof(table[0]));
    return table;
}

} // namespace PolicyTable
