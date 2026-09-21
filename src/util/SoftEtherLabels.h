#pragma once

#include <QByteArray>
#include <QCoreApplication>
#include <QDateTime>
#include <QString>
#include <QStringList>

// 公式Manager (strtable_ja.stb) の表記に合わせた、複数画面で共有する変換関数。
namespace SoftEtherLabels {

inline QString onlineStatus(bool online)
{
    // SM_HUB_ONLINE / SM_HUB_OFFLINE
    return online ? QCoreApplication::translate("SoftEtherLabels", "オンライン")
                  : QCoreApplication::translate("SoftEtherLabels", "オフライン");
}

inline QString hubType(int hubType)
{
    // SM_HUB_STANDALONE / SM_HUB_STATIC / SM_HUB_DYNAMIC
    switch (hubType) {
    case 0:
        return QCoreApplication::translate("SoftEtherLabels", "スタンドアロン");
    case 1:
        return QCoreApplication::translate("SoftEtherLabels", "スタティック仮想 HUB");
    case 2:
        return QCoreApplication::translate("SoftEtherLabels", "ダイナミック仮想 HUB");
    default:
        return QCoreApplication::translate("SoftEtherLabels", "不明");
    }
}

inline QString authType(int authType)
{
    // SM_AUTHTYPE_0〜5
    switch (authType) {
    case 0:
        return QCoreApplication::translate("SoftEtherLabels", "匿名認証");
    case 1:
        return QCoreApplication::translate("SoftEtherLabels", "パスワード認証");
    case 2:
        return QCoreApplication::translate("SoftEtherLabels", "固有証明書認証");
    case 3:
        return QCoreApplication::translate("SoftEtherLabels", "署名済み証明書認証");
    case 4:
        return QCoreApplication::translate("SoftEtherLabels", "RADIUS 認証");
    case 5:
        return QCoreApplication::translate("SoftEtherLabels", "NT ドメイン認証");
    default:
        return QCoreApplication::translate("SoftEtherLabels", "不明");
    }
}

inline QString secureNatEnabled(bool enabled)
{
    // SM_HUB_SECURE_NAT_YES / SM_HUB_SECURE_NAT_NO
    return enabled ? QCoreApplication::translate("SoftEtherLabels", "有効")
                   : QCoreApplication::translate("SoftEtherLabels", "無効");
}

// "AA:BB:CC:DD:EE:FF" や "AA-BB-CC-DD-EE-FF" 形式の文字列をAPIが要求するbase64表現に変換する。
inline QString macAddressToBase64(const QString &text)
{
    QString hex = text;
    hex.remove(QLatin1Char(':'));
    hex.remove(QLatin1Char('-'));
    const QByteArray raw = QByteArray::fromHex(hex.toUtf8());
    return QString::fromLatin1(raw.toBase64());
}

inline QString macAddress(const QString &base64Bytes)
{
    const QByteArray raw = QByteArray::fromBase64(base64Bytes.toUtf8());
    QStringList parts;
    for (unsigned char byte : raw) {
        parts << QString::number(byte, 16).rightJustified(2, QLatin1Char('0')).toUpper();
    }
    return parts.join(QLatin1Char(':'));
}

inline QString sessionLocation(bool linkMode, bool secureNatMode, bool bridgeMode, bool layer3Mode, bool remoteSession,
                                const QString &remoteHostname)
{
    // SM_SESS_LINK / SM_SESS_SNAT / SM_SESS_BRIDGE / SM_SESS_NORMAL / SM_SESS_REMOTE
    if (linkMode) {
        return QCoreApplication::translate("SoftEtherLabels", "カスケード接続");
    }
    if (secureNatMode) {
        return QCoreApplication::translate("SoftEtherLabels", "SecureNAT セッション");
    }
    if (bridgeMode) {
        return QCoreApplication::translate("SoftEtherLabels", "ローカルブリッジセッション");
    }
    if (layer3Mode) {
        return QCoreApplication::translate("SoftEtherLabels", "仮想レイヤ 3 スイッチセッション");
    }
    if (remoteSession) {
        return QCoreApplication::translate("SoftEtherLabels", "%1 上").arg(remoteHostname);
    }
    return QCoreApplication::translate("SoftEtherLabels", "ローカルセッション");
}

inline QString protocolName(int protocol)
{
    switch (protocol) {
    case 0:
        return QCoreApplication::translate("SoftEtherLabels", "すべてのプロトコル");
    case 1:
        return QCoreApplication::translate("SoftEtherLabels", "ICMP");
    case 6:
        return QCoreApplication::translate("SoftEtherLabels", "TCP");
    case 17:
        return QCoreApplication::translate("SoftEtherLabels", "UDP");
    case 58:
        return QCoreApplication::translate("SoftEtherLabels", "ICMPv6");
    default:
        return QString::number(protocol);
    }
}

inline QString natProtocolName(int protocol)
{
    // EnumNATのProtocol_u32は0:TCP/1:UDP/2:DNS/3:ICMPという専用の列挙。
    switch (protocol) {
    case 0:
        return QCoreApplication::translate("SoftEtherLabels", "TCP");
    case 1:
        return QCoreApplication::translate("SoftEtherLabels", "UDP");
    case 2:
        return QCoreApplication::translate("SoftEtherLabels", "DNS");
    case 3:
        return QCoreApplication::translate("SoftEtherLabels", "ICMP");
    default:
        return QString::number(protocol);
    }
}

inline QString macIpLocation(bool remoteItem, const QString &remoteHostname)
{
    // SM_MACIP_LOCAL / SM_MACIP_SERVER
    return remoteItem ? QCoreApplication::translate("SoftEtherLabels", "%1 上").arg(remoteHostname)
                       : QCoreApplication::translate("SoftEtherLabels", "このサーバー上");
}

inline QString dateTime(const QString &isoString)
{
    const QDateTime dt = QDateTime::fromString(isoString, Qt::ISODateWithMs);
    if (!dt.isValid()) {
        return QStringLiteral("-");
    }
    return dt.toLocalTime().toString(QStringLiteral("yyyy/MM/dd HH:mm:ss"));
}

} // namespace SoftEtherLabels
