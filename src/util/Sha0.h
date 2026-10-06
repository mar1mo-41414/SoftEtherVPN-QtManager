#pragma once

#include <QByteArray>
#include <QString>

// SHA-0 (SHA-1 からメッセージスケジュールの1ビット回転を除いたもの)。
// SoftEther のパスワード認証 (HashedPassword = SHA0(パスワード + 大文字ユーザー名)) で使う。
namespace Sha0 {

inline QByteArray hash(const QByteArray &data)
{
    quint32 h0 = 0x67452301, h1 = 0xEFCDAB89, h2 = 0x98BADCFE, h3 = 0x10325476, h4 = 0xC3D2E1F0;

    QByteArray message = data;
    const quint64 bitLength = static_cast<quint64>(data.size()) * 8;
    message.append(char(0x80));
    while (message.size() % 64 != 56) {
        message.append(char(0));
    }
    for (int i = 7; i >= 0; --i) {
        message.append(static_cast<char>((bitLength >> (i * 8)) & 0xff));
    }

    const auto rotl = [](quint32 value, int bits) { return (value << bits) | (value >> (32 - bits)); };

    for (int offset = 0; offset < message.size(); offset += 64) {
        quint32 w[80];
        for (int i = 0; i < 16; ++i) {
            const auto *p = reinterpret_cast<const quint8 *>(message.constData()) + offset + i * 4;
            w[i] = (quint32(p[0]) << 24) | (quint32(p[1]) << 16) | (quint32(p[2]) << 8) | quint32(p[3]);
        }
        for (int i = 16; i < 80; ++i) {
            w[i] = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]; // SHA-1 との違い: rotl(…, 1) を行わない
        }
        quint32 a = h0, b = h1, c = h2, d = h3, e = h4;
        for (int i = 0; i < 80; ++i) {
            quint32 f, k;
            if (i < 20) {
                f = (b & c) | (~b & d);
                k = 0x5A827999;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }
            const quint32 temp = rotl(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = rotl(b, 30);
            b = a;
            a = temp;
        }
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    QByteArray digest;
    for (quint32 value : {h0, h1, h2, h3, h4}) {
        for (int i = 3; i >= 0; --i) {
            digest.append(static_cast<char>((value >> (i * 8)) & 0xff));
        }
    }
    return digest;
}

// SoftEther 形式のパスワードハッシュ: SHA0(パスワード + UPPER(ユーザー名))。
// ※ JSON-RPC の API ドキュメントには「ユーザー名 + パスワード」の順と書かれているが誤りで、
//    本体の Cedar/Account.c HashPassword() は パスワード → 大文字ユーザー名 の順に連結する。
inline QByteArray passwordHash(const QString &userName, const QString &password)
{
    return hash(password.toUtf8() + userName.toUpper().toUtf8());
}

} // namespace Sha0
