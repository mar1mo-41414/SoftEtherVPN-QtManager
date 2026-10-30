// win-settings-import: 公式 VPN Server Manager (Windows専用) の接続設定一覧を、
// このSoftEtherVPN-QtManagerの設定ストア(QSettings)に取り込むミニツール。
// GUI本体(SoftEtherVPN-QtManager)とは別バイナリ。GUIは起動せず、CUIのみで完結する。
//
// 仕組み:
//   公式Managerは接続設定一覧をWindowsレジストリ
//   HKEY_CURRENT_USER\Software\SoftEther VPN Developer Edition\SoftEther VPN\Server Manager\Settings
//   (旧版は \SoftEther Corporation\PacketiX VPN\Server Manager\Settings) に、
//   1件ごとREG_BINARYの生C構造体(SETTING, 13420バイト固定)として保存している
//   (公式ソース: src/Cedar/SM.c の SmWriteSettingList/SmLoadSettingList、
//   src/Cedar/SMInner.h の SETTING構造体、src/Cedar/Connection.h の CLIENT_OPTION構造体)。
//
//   このツールは、Windows側で下記のように対象キーを.regファイルへエクスポートしたものを
//   読み込み、REG_BINARYの16進数列をSETTING構造体として手動でデコードして取り込む。
//
//     reg export "HKCU\Software\SoftEther VPN Developer Edition\SoftEther VPN\Server Manager\Settings" settings.reg
//
// 重要な制約 (パスワードは移行できない):
//   公式Managerは管理パスワードを「平文」でも「復号可能な形」でもなく、SHA-0ハッシュ
//   (HashedPassword[20]) としてのみ保存している。これは公式Manager自身も復号して
//   表示することはできない一方向ハッシュであり、このツールも同様に復元不可能。
//   そのためパスワードは常に空(要再入力)として取り込む。
//   ただし、プロキシ認証パスワード(ProxyPassword)は平文で保存されているため、
//   こちらは正しく移行できる。

#include "model/ConnectionProfile.h"
#include "model/ConnectionProfileStore.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QDebug>

namespace {

// SETTING構造体 (src/Cedar/SMInner.h) のバイトオフセット。構造体のアイテム順序・
// サイズは公式ソースのコメントで「変更禁止、合計13420バイト固定」とされているため、
// 現行のStable/Developer Editionビルドであれば安定して一致する。
constexpr int kSettingSize = 13420;
constexpr int kOffTitle = 0;                 // wchar_t[512]
constexpr int kOffServerAdminMode = 1024;    // bool(1) + pad[3]
constexpr int kOffHubName = 1028;            // char[256]
constexpr int kOffHashedPassword = 1284;     // UCHAR[20]
constexpr int kOffClientOption = 1304;       // CLIENT_OPTION本体

// CLIENT_OPTION構造体 (src/Cedar/Connection.h) のkOffClientOptionからの相対オフセット。
constexpr int kCoOffHostname = 512;          // char[256] (AccountName[256*2]の次)
constexpr int kCoOffPort = 768;              // UINT
constexpr int kCoOffProxyType = 776;         // UINT
constexpr int kCoOffProxyName = 780;         // char[256]
constexpr int kCoOffProxyPort = 1036;        // UINT
constexpr int kCoOffProxyUsername = 1040;    // char[256]
constexpr int kCoOffProxyPassword = 1296;    // char[256]
constexpr int kCoOffHubName = 1560;          // char[256] (ClientOption側。通常SETTING.HubNameと同値)

quint32 readU32LE(const QByteArray &buf, int offset)
{
    const auto *p = reinterpret_cast<const uchar *>(buf.constData() + offset);
    return quint32(p[0]) | (quint32(p[1]) << 8) | (quint32(p[2]) << 16) | (quint32(p[3]) << 24);
}

// Null終端のASCII/UTF-8前提のchar[]フィールドを読む。
// 注意: 公式ManagerはこれらをWindowsのANSI(ローカルコードページ)関数で読み書きしており、
// ASCII範囲外の文字(日本語ホスト名/HUB名等)が含まれる場合は文字化けし得る。
// ホスト名・HUB名は通常ASCIIのため実用上は問題にならない想定。
QString readAnsiField(const QByteArray &buf, int offset, int maxLen)
{
    QByteArray raw = buf.mid(offset, maxLen);
    int nul = raw.indexOf('\0');
    if (nul >= 0) {
        raw.truncate(nul);
    }
    QString s = QString::fromUtf8(raw);
    if (s.toUtf8() != raw) {
        // UTF-8として往復しない = ASCII外の可能性が高い。Latin-1でフォールバック
        // (正しいデコードではないが、少なくとも文字化けはLatin-1相当で収まる)。
        s = QString::fromLatin1(raw);
    }
    return s;
}

// Null終端のUTF-16LE(wchar_t[])フィールドを読む。公式ManagerのTitleはこちらで
// 正しくUnicode(日本語含む)が保存されている。
QString readWideField(const QByteArray &buf, int offset, int maxWchars)
{
    const auto *p = reinterpret_cast<const char16_t *>(buf.constData() + offset);
    int len = 0;
    while (len < maxWchars && p[len] != 0) {
        ++len;
    }
    return QString::fromUtf16(p, len);
}

struct ImportedEntry
{
    ConnectionProfile profile;
    bool hadSavedPassword = false;
};

ImportedEntry decodeSetting(const QByteArray &buf)
{
    ImportedEntry entry;
    ConnectionProfile &p = entry.profile;

    p.name = readWideField(buf, kOffTitle, 512);

    const bool serverAdminMode = buf.at(kOffServerAdminMode) != 0;
    p.hubAdminMode = !serverAdminMode; // SM.c: Check(R_SERVER_ADMIN, ServerAdminMode) / R_HUB_ADMIN はその否定
    p.hubName = readAnsiField(buf, kOffHubName, 256);

    // HashedPassword はSHA-0の一方向ハッシュであり復元不可能。保存の有無だけ判定する。
    QByteArray hashed = buf.mid(kOffHashedPassword, 20);
    entry.hadSavedPassword = !hashed.isEmpty() && hashed != QByteArray(20, '\0');
    p.noSavePassword = true; // パスワードは常に再入力してもらう
    p.password.clear();

    const int co = kOffClientOption;
    p.host = readAnsiField(buf, co + kCoOffHostname, 256);
    p.port = quint16(readU32LE(buf, co + kCoOffPort));

    const quint32 proxyType = readU32LE(buf, co + kCoOffProxyType);
    // PROXY_DIRECT=0, PROXY_HTTP=1, PROXY_SOCKS(4)=2, PROXY_SOCKS5=3 (src/Cedar/Cedar.h)
    // 本アプリはSOCKS4/5を区別しないため両方とも2に丸める。
    p.proxyType = proxyType == 1 ? 1 : (proxyType >= 2 ? 2 : 0);
    p.proxyHost = readAnsiField(buf, co + kCoOffProxyName, 256);
    p.proxyPort = quint16(readU32LE(buf, co + kCoOffProxyPort));
    p.proxyUser = readAnsiField(buf, co + kCoOffProxyUsername, 256);
    // ProxyPasswordはハッシュ化されず平文で保存されているため、正しく移行できる。
    p.proxyPassword = readAnsiField(buf, co + kCoOffProxyPassword, 256);

    if (p.hubName.isEmpty()) {
        QString coHubName = readAnsiField(buf, co + kCoOffHubName, 256);
        if (!coHubName.isEmpty()) {
            p.hubName = coHubName;
        }
    }

    return entry;
}

// .regファイルのテキストを読み込む。regedit/reg.exeの標準出力はUTF-16LE(BOM付き)。
QString readRegFileText(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qCritical().noquote() << QStringLiteral("ファイルを開けません: %1").arg(path);
        return {};
    }
    QByteArray data = f.readAll();

    if (data.size() >= 2 && uchar(data[0]) == 0xFF && uchar(data[1]) == 0xFE) {
        const auto *p = reinterpret_cast<const char16_t *>(data.constData() + 2);
        return QString::fromUtf16(p, (data.size() - 2) / 2);
    }
    if (data.size() >= 3 && uchar(data[0]) == 0xEF && uchar(data[1]) == 0xBB && uchar(data[2]) == 0xBF) {
        return QString::fromUtf8(data.constData() + 3, data.size() - 3);
    }
    return QString::fromUtf8(data);
}

QList<QByteArray> extractSettingBlobs(const QString &text)
{
    QList<QByteArray> blobs;

    static const QRegularExpression sectionRe(
        QStringLiteral(R"(^\[HKEY_CURRENT_USER\\Software\\SoftEther(?: VPN Developer Edition\\SoftEther VPN|\s*Corporation\\PacketiX VPN)\\Server Manager\\Settings\]\s*$)"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression valueStartRe(
        QStringLiteral(R"(^"Setting\d+"\s*=\s*hex\(3\):(.*)$)"));

    const QStringList rawLines = text.split(QRegularExpression(QStringLiteral("\r\n|\n|\r")));

    bool inTargetSection = false;
    QString pendingHex;
    bool continuing = false;

    auto flushPending = [&]() {
        if (pendingHex.isEmpty()) {
            return;
        }
        QByteArray bytes;
        const QStringList parts = pendingHex.split(QLatin1Char(','), Qt::SkipEmptyParts);
        bytes.reserve(parts.size());
        for (const QString &tok : parts) {
            const QString t = tok.trimmed();
            if (t.isEmpty()) {
                continue;
            }
            bool ok = false;
            const uint v = t.toUInt(&ok, 16);
            if (ok) {
                bytes.append(char(v));
            }
        }
        if (bytes.size() == kSettingSize) {
            blobs.append(bytes);
        } else if (bytes.size() > 0) {
            qWarning().noquote() << QStringLiteral(
                "警告: サイズが想定(%1バイト)と異なる値を1件スキップしました(実際: %2バイト)。"
                "公式Managerの非常に古いバージョン形式の可能性があります。")
                .arg(kSettingSize).arg(bytes.size());
        }
        pendingHex.clear();
    };

    for (QString line : rawLines) {
        if (line.startsWith(QLatin1Char('['))) {
            flushPending();
            continuing = false;
            inTargetSection = sectionRe.match(line).hasMatch();
            continue;
        }
        if (!inTargetSection) {
            continue;
        }

        if (continuing) {
            QString l = line;
            continuing = l.endsWith(QLatin1Char('\\'));
            if (continuing) {
                l.chop(1);
            }
            pendingHex += l.trimmed();
            if (!continuing) {
                flushPending();
            }
            continue;
        }

        const QRegularExpressionMatch m = valueStartRe.match(line);
        if (m.hasMatch()) {
            flushPending();
            QString l = m.captured(1);
            continuing = l.endsWith(QLatin1Char('\\'));
            if (continuing) {
                l.chop(1);
            }
            pendingHex = l.trimmed();
            if (!continuing) {
                flushPending();
            }
        }
    }
    flushPending();

    return blobs;
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    // ConnectionProfileStoreが使うQSettingsの保存先を、GUI本体(main.cpp)と完全に
    // 一致させる。これが違うと別の設定ファイルに書き込まれてGUI側から見えなくなる。
    app.setApplicationName(QStringLiteral("SoftEtherVPN-QtManager"));
    app.setOrganizationName(QStringLiteral("SoftEtherVPN-QtManager"));

    const QStringList args = app.arguments();
    QString regPath;
    bool dryRun = false;
    bool replace = false;

    for (int i = 1; i < args.size(); ++i) {
        const QString &a = args.at(i);
        if (a == QLatin1String("--dry-run")) {
            dryRun = true;
        } else if (a == QLatin1String("--replace")) {
            replace = true;
        } else if (a == QLatin1String("-h") || a == QLatin1String("--help")) {
            regPath.clear();
            break;
        } else if (regPath.isEmpty()) {
            regPath = a;
        }
    }

    if (regPath.isEmpty()) {
        QTextStream out(stdout);
        out << "win-settings-import — 公式 VPN Server Manager (Windows) の接続設定一覧を、\n"
               "SoftEtherVPN-QtManagerの設定ストアへ取り込みます。\n\n"
               "使い方:\n"
               "  1. Windows側で対象レジストリキーを.regファイルへエクスポート:\n"
               "       reg export \"HKCU\\Software\\SoftEther VPN Developer Edition\\SoftEther VPN\\Server Manager\\Settings\" settings.reg\n"
               "  2. そのsettings.regをこちらの環境へコピー\n"
               "  3. 実行:\n"
               "       win-settings-import settings.reg\n\n"
               "オプション:\n"
               "  --dry-run   実際には保存せず、取り込める内容を表示するだけ\n"
               "  --replace   同名の既存接続設定があれば上書きする(デフォルトはスキップ)\n\n"
               "注意: 管理パスワードは公式Managerがハッシュでしか保存していないため移行できません。\n"
               "      取り込み後は各接続設定でパスワードを入力し直してください。\n"
               "      プロキシ認証パスワードは平文保存のため正しく移行されます。\n";
        return regPath.isEmpty() && args.size() <= 1 ? 1 : 0;
    }

    const QString text = readRegFileText(regPath);
    if (text.isEmpty()) {
        return 1;
    }

    const QList<QByteArray> blobs = extractSettingBlobs(text);
    if (blobs.isEmpty()) {
        qCritical().noquote() << "接続設定が見つかりませんでした。"
                                  "対象キー(...\\Server Manager\\Settings)をエクスポートした.regファイルか確認してください。";
        return 1;
    }

    QTextStream out(stdout);
    out << QStringLiteral("%1件の接続設定を検出しました。\n").arg(blobs.size());

    QList<ConnectionProfile> existing = ConnectionProfileStore::loadAll();
    int imported = 0, skipped = 0;

    for (const QByteArray &blob : blobs) {
        ImportedEntry entry = decodeSetting(blob);
        const ConnectionProfile &p = entry.profile;

        int existingIndex = -1;
        for (int i = 0; i < existing.size(); ++i) {
            if (existing.at(i).name == p.name) {
                existingIndex = i;
                break;
            }
        }

        out << QStringLiteral("- %1  (%2:%3, %4%5)\n")
                   .arg(p.name, p.host)
                   .arg(p.port)
                   .arg(p.hubAdminMode ? QStringLiteral("仮想HUB管理: %1").arg(p.hubName) : QStringLiteral("サーバー管理"))
                   .arg(entry.hadSavedPassword ? QStringLiteral(" / パスワードは移行不可のため再入力が必要") : QString());

        if (existingIndex >= 0 && !replace) {
            out << QStringLiteral("    → 同名の設定が既にあるためスキップ (--replace で上書き可能)\n");
            ++skipped;
            continue;
        }

        if (existingIndex >= 0) {
            existing[existingIndex] = p;
        } else {
            existing.append(p);
        }
        ++imported;
    }

    out << QStringLiteral("\n取り込み: %1件 / スキップ: %2件\n").arg(imported).arg(skipped);

    if (dryRun) {
        out << "--dry-run のため保存はしていません。\n";
        return 0;
    }

    if (imported > 0) {
        ConnectionProfileStore::saveAll(existing);
        out << "保存しました。SoftEtherVPN-QtManagerを起動して確認してください。\n";
    }

    return 0;
}
