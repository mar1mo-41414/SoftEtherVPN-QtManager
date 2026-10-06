#!/usr/bin/env python3
"""SoftEther VPN Server の JSON-RPC をまねる最小のモックサーバー (レイアウト確認用)。

実サーバーに触れずにQt版の画面を確認するためのもの。値はすべて架空の固定データ。
usage: python3 tools/mock_vpnserver.py [cert.pem key.pem]
  https://127.0.0.1:5556/api/ で待ち受ける。証明書を指定しなければ openssl で自己署名証明書を
  一時ディレクトリに自動生成する。管理パスワードは検証しない (何を入れても接続できる)。
  接続設定は 127.0.0.1:5556 / 管理モード=サーバー全体 で作る。書き込み系APIは何もせず空の結果を返す。
"""
import base64
import http.server
import json
import ssl
import sys

NOW = "2026-10-06T13:40:00.000"

HUBS = [
    {"HubName_str": "HUB-A", "Online_bool": True, "HubType_u32": 0, "NumUsers_u32": 4, "NumGroups_u32": 0,
     "NumSessions_u32": 1, "NumMacTables_u32": 1, "NumIpTables_u32": 2, "LastCommTime_dt": NOW,
     "LastLoginTime_dt": NOW, "CreatedTime_dt": NOW, "NumLogin_u32": 6},
    {"HubName_str": "test", "Online_bool": True, "HubType_u32": 0, "NumUsers_u32": 1, "NumGroups_u32": 0,
     "NumSessions_u32": 0, "NumMacTables_u32": 0, "NumIpTables_u32": 0, "LastCommTime_dt": NOW,
     "LastLoginTime_dt": NOW, "CreatedTime_dt": NOW, "NumLogin_u32": 2},
    {"HubName_str": "HUB-B", "Online_bool": True, "HubType_u32": 0, "NumUsers_u32": 1, "NumGroups_u32": 0,
     "NumSessions_u32": 1, "NumMacTables_u32": 24, "NumIpTables_u32": 43, "LastCommTime_dt": NOW,
     "LastLoginTime_dt": NOW, "CreatedTime_dt": NOW, "NumLogin_u32": 354},
]

HUB_STATUS = {
    "HubName_str": "test", "Online_bool": True, "HubType_u32": 0, "NumSessions_u32": 0, "NumSessionsClient_u32": 0,
    "NumSessionsBridge_u32": 0, "NumAccessLists_u32": 1, "NumUsers_u32": 1, "NumGroups_u32": 0,
    "NumMacTables_u32": 0, "NumIpTables_u32": 0, "Recv.BroadcastBytes_u64": 1000, "Recv.BroadcastCount_u64": 10,
    "Recv.UnicastBytes_u64": 2000, "Recv.UnicastCount_u64": 20, "Send.BroadcastBytes_u64": 3000,
    "Send.BroadcastCount_u64": 30, "Send.UnicastBytes_u64": 4000, "Send.UnicastCount_u64": 40,
    "SecureNATEnabled_bool": False, "LastCommTime_dt": NOW, "LastLoginTime_dt": NOW, "CreatedTime_dt": NOW,
    "NumLogin_u32": 2,
}

SERVER_STATUS = {
    "ServerType_u32": 0, "NumTcpConnections_u32": 170, "NumHubTotal_u32": 3, "NumSessionsTotal_u32": 0,
    "NumMacTables_u32": 25, "NumIpTables_u32": 45, "NumUsers_u32": 6, "NumGroups_u32": 0,
    "AssignedBridgeLicenses_u32": 0, "AssignedClientLicenses_u32": 0,
    "Recv.BroadcastBytes_u64": 32034676122, "Recv.BroadcastCount_u64": 140159066,
    "Recv.UnicastBytes_u64": 1927365053668, "Recv.UnicastCount_u64": 2968392394,
    "Send.BroadcastBytes_u64": 40608286, "Send.BroadcastCount_u64": 176019,
    "Send.UnicastBytes_u64": 17958301425, "Send.UnicastCount_u64": 267592316,
    "CurrentTime_dt": NOW, "CurrentTick_u64": 123456789, "StartTime_dt": "2026-09-21T11:07:03.000",
    "TotalMemory_u64": 0, "UsedMemory_u64": 0, "FreeMemory_u64": 0, "TotalPhys_u64": 0, "UsedPhys_u64": 0,
    "FreePhys_u64": 0,
}

SERVER_INFO = {
    "ServerProductName_str": "SoftEther VPN Server (64 bit)", "ServerVersionString_str": "Version 4.42 Build 9798   (English)",
    "ServerBuildInfoString_str": "Compiled 2023/06/30 11:06:58", "ServerVerInt_u32": 442, "ServerBuildInt_u32": 9798,
    "ServerHostName_str": "vpn-example", "ServerType_u32": 0, "ServerBuildDate_dt": NOW, "ServerFamilyName_str": "",
    "OsType_u32": 3100, "OsServicePack_u32": 0, "OsSystemName_str": "Linux", "OsProductName_str": "Linux",
    "OsVendorName_str": "Unknown Vendor", "OsVersion_str": "Unknown Linux Version", "KernelName_str": "Linux Kernel",
    "KernelVersion_str": "Linux Kernel",
}

CAPS = [
    {"CapsName_str": "b_beta_version", "CapsValue_u32": 0}, {"CapsName_str": "b_bridge", "CapsValue_u32": 0},
    {"CapsName_str": "b_cluster_controller", "CapsValue_u32": 0}, {"CapsName_str": "i_max_hubs", "CapsValue_u32": 4096},
    {"CapsName_str": "b_is_in_vm", "CapsValue_u32": 1}, {"CapsName_str": "b_is_softether", "CapsValue_u32": 1},
]

DB = {
    "EnumHub": {"NumHub_u32": 3, "HubList": HUBS},
    "GetServerInfo": SERVER_INFO,
    "GetServerStatus": SERVER_STATUS,
    "GetHubStatus": HUB_STATUS,
    "GetHub": {"HubName_str": "test", "Online_bool": True, "MaxSession_u32": 0, "NoEnum_bool": False, "HubType_u32": 0,
               "AdminPasswordPlainText_str": ""},
    "GetCaps": {"CapsList": CAPS},
    "EnumListener": {"ListenerList": [
        {"Ports_u32": 992, "Enables_bool": True, "Errors_bool": False},
        {"Ports_u32": 1194, "Enables_bool": True, "Errors_bool": False},
        {"Ports_u32": 4500, "Enables_bool": True, "Errors_bool": False},
        {"Ports_u32": 5555, "Enables_bool": True, "Errors_bool": False}]},
    "GetFarmSetting": {"ServerType_u32": 0, "NumPort_u32": 0, "Ports_u32": [], "PublicIp_ip": "0.0.0.0",
                       "ControllerName_str": "", "ControllerPort_u32": 443, "MemberPasswordPlaintext_str": "",
                       "Weight_u32": 100, "ControllerOnly_bool": False},
    "GetDDnsClientStatus": {"Err_IPv4_u32": 0, "ErrStr_IPv4_utf": "", "Err_IPv6_u32": 1, "ErrStr_IPv6_utf": "x",
                            "CurrentHostName_str": "example", "CurrentFqdn_str": "example.softether.net",
                            "DnsSuffix_str": ".softether.net", "CurrentIPv4_str": "203.0.113.5", "CurrentIPv6_str": ""},
    "GetAzureStatus": {"IsEnabled_bool": True, "IsConnected_bool": True},
    "GetConfig": {"FileName_str": "vpn_server.config", "FileData_bin": base64.b64encode(
        b"declare root\n{\n\tdeclare DDnsClient\n\t{\n\t\tbool Disabled false\n\t\tbyte Key AAECAwQFBgcICQoLDA0ODxAREhM=\n\t}\n}\n").decode()},
    "EnumConnection": {"NumConnection_u32": 1, "ConnectionList": [
        {"Name_str": "CID-1", "Hostname_str": "192.0.2.50", "Ip_ip": "192.0.2.50", "Port_u32": 51234,
         "ConnectedTime_dt": NOW, "Type_u32": 5}]},
    "EnumUser": {"HubName_str": "test", "UserList": [
        {"Name_str": "a", "GroupName_str": "", "Realname_utf": "", "Note_utf": "", "AuthType_u32": 1,
         "NumLogin_u32": 2, "LastLoginTime_dt": NOW, "DenyAccess_bool": False, "IsTrafficFilled_bool": False,
         "IsExpiresFilled_bool": False, "Expires_dt": NOW}]},
    "EnumGroup": {"HubName_str": "test", "GroupList": [
        {"Name_str": "grp", "Realname_utf": "テストグループ", "Note_utf": "メモ", "NumUsers_u32": 1}]},
    "GetUser": {"HubName_str": "test", "Name_str": "a", "GroupName_str": "grp", "Realname_utf": "ユーザー例", "Note_utf": "n",
                "CreatedTime_dt": NOW, "UpdatedTime_dt": NOW, "ExpireTime_dt": "2030-01-02T03:04:05.000",
                "AuthType_u32": 3, "CommonName_utf": "*.example.com", "Serial_bin": "AVXN", "NumLogin_u32": 2,
                "Recv.UnicastCount_u64": 135, "Recv.UnicastBytes_u64": 13915, "Send.UnicastCount_u64": 140,
                "Send.UnicastBytes_u64": 8847, "UsePolicy_bool": True, "policy:Access_bool": True,
                "policy:MaxConnection_u32": 8, "policy:TimeOut_u32": 20, "policy:MaxUpload_u32": 1000000,
                "policy:NoBridge_bool": True},
    "GetGroup": {"HubName_str": "test", "Name_str": "grp", "Realname_utf": "テストグループ", "Note_utf": "メモ",
                 "UsePolicy_bool": False, "Recv.UnicastCount_u64": 5, "Send.UnicastCount_u64": 6},
    "EnumSession": {"HubName_str": "test", "SessionList": []},
    "EnumAccess": {"HubName_str": "test", "AccessList": [
        {"Id_u32": 1, "Note_utf": "テスト", "Active_bool": True, "Priority_u32": 1000, "Discard_bool": False,
         "IsIPv6_bool": False, "SrcIpAddress_ip": "192.0.2.2", "SrcSubnetMask_ip": "255.255.255.255",
         "DestIpAddress_ip": "0.0.0.0", "DestSubnetMask_ip": "0.0.0.0", "Protocol_u32": 6,
         "SrcPortStart_u32": 99, "SrcPortEnd_u32": 99, "DestPortStart_u32": 99, "DestPortEnd_u32": 99,
         "SrcUsername_str": "", "DestUsername_str": "", "CheckSrcMac_bool": False, "CheckDstMac_bool": False,
         "CheckTcpState_bool": True, "Established_bool": True, "Delay_u32": 0, "Jitter_u32": 0, "Loss_u32": 0,
         "RedirectUrl_str": ""}]},
    "EnumLink": {"HubName_str": "test", "NumLink_u32": 3, "LinkList": [
        {"AccountName_utf": "link1", "Online_bool": True, "Connected_bool": True, "LastError_u32": 0,
         "ConnectedTime_dt": NOW, "Hostname_str": "10.0.0.1", "TargetHubName_str": "HUB-B"},
        {"AccountName_utf": "link2", "Online_bool": True, "Connected_bool": False, "LastError_u32": 1,
         "ConnectedTime_dt": NOW, "Hostname_str": "10.0.0.2", "TargetHubName_str": "HUB-B"},
        {"AccountName_utf": "link3", "Online_bool": False, "Connected_bool": False, "LastError_u32": 0,
         "ConnectedTime_dt": NOW, "Hostname_str": "10.0.0.3", "TargetHubName_str": "HUB-B"}]},
    "GetLink": {"AccountName_utf": "link1", "Hostname_str": "10.0.0.1", "Port_u32": 992, "HubName_str": "HUB-B",
                "ProxyType_u32": 0, "AuthType_u32": 1, "Username_str": "u1", "HashedPassword_bin": "AAECAwQFBgcICQoLDA0ODxAREhM=",
                "CheckServerCert_bool": True, "MaxConnection_u32": 4, "UseEncrypt_bool": True,
                "policy:MaxUpload_u32": 1000000, "policy:DHCPFilter_bool": True},
    "EnumL3Switch": {"L3SWList": []},
    "EnumEtherIpId": {"Settings": []},
    "EnumLocalBridge": {"LocalBridgeList": []},
    "EnumEthernet": {"EthList": [{"DeviceName_str": "eth0", "NetworkConnectionName_utf": "eth0"}]},
    "EnumLogFile": {"LogFiles": []},
    "EnumMacTable": {"HubName_str": "test", "MacTable": []},
    "EnumIpTable": {"HubName_str": "test", "IpTable": []},
    "GetSysLog": {"SaveType_u32": 0, "Hostname_str": "", "Port_u32": 514},
    "GetKeep": {"UseKeepConnect_bool": False, "KeepConnectHost_str": "", "KeepConnectPort_u32": 80,
                "KeepConnectProtocol_u32": 0, "KeepConnectInterval_u32": 50},
    "GetServerCipher": {"String_str": "ECDHE-RSA-AES128-GCM-SHA256"},
    "GetServerCert": {"Cert_bin": "", "Key_bin": ""},
    "GetIPsecServices": {"L2TP_Raw_bool": False, "L2TP_IPsec_bool": True, "EtherIP_IPsec_bool": False,
                         "IPsec_Secret_str": "vpn", "L2TP_DefaultHub_str": "HUB-B"},
    "GetOpenVpnSstpConfig": {"EnableOpenVPN_bool": True, "OpenVPNPortList_str": "1194", "EnableSSTP_bool": False},
    "GetSpecialListener": {"VpnOverIcmpListener_bool": False, "VpnOverDnsListener_bool": False},
    "GetDDnsInternetSetting": {"ProxyType_u32": 0, "ProxyHostName_str": "", "ProxyPort_u32": 0,
                               "ProxyUsername_str": "", "ProxyPassword_str": ""},
    "GetSecureNATOption": {"RpcHubName_str": "test", "MacAddress_bin": "XkLhHyIz", "Ip_ip": "10.0.0.1",
                           "Mask_ip": "255.255.255.0", "UseNat_bool": True, "Mtu_u32": 1500, "NatTcpTimeout_u32": 1800,
                           "NatUdpTimeout_u32": 60, "UseDhcp_bool": True, "DhcpLeaseIPStart_ip": "10.0.0.10",
                           "DhcpLeaseIPEnd_ip": "10.0.0.200", "DhcpSubnetMask_ip": "255.255.255.0",
                           "DhcpExpireTimeSpan_u32": 7200, "DhcpGatewayAddress_ip": "10.0.0.1",
                           "DhcpDnsServerAddress_ip": "10.0.0.1", "DhcpDnsServerAddress2_ip": "",
                           "DhcpDomainName_str": "", "SaveLog_bool": True, "ApplyDhcpPushRoutes_bool": False,
                           "DhcpPushRoutes_str": ""},
}


class Handler(http.server.BaseHTTPRequestHandler):
    def do_POST(self):
        length = int(self.headers.get("Content-Length", 0))
        request = json.loads(self.rfile.read(length) or b"{}")
        method = request.get("method", "")
        result = DB.get(method, {})
        body = json.dumps({"jsonrpc": "2.0", "id": request.get("id"), "result": result}).encode()
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)
        sys.stderr.write(f"RPC {method}\n")

    def log_message(self, *args):
        pass


def make_certificate():
    """openssl で自己署名証明書を一時ディレクトリに作る。"""
    import subprocess
    import tempfile
    directory = tempfile.mkdtemp(prefix="mock_vpnserver_")
    cert, key = f"{directory}/mock.crt", f"{directory}/mock.key"
    subprocess.run(["openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes", "-keyout", key, "-out", cert,
                    "-days", "30", "-subj", "/CN=mock-vpn-server"], check=True, capture_output=True)
    return cert, key


if __name__ == "__main__":
    cert_path, key_path = (sys.argv[1], sys.argv[2]) if len(sys.argv) > 2 else make_certificate()
    server = http.server.ThreadingHTTPServer(("127.0.0.1", 5556), Handler)
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(cert_path, key_path)
    server.socket = context.wrap_socket(server.socket, server_side=True)
    print("mock VPN server: https://127.0.0.1:5556/api/", file=sys.stderr)
    server.serve_forever()
