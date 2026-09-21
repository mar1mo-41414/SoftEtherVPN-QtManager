#pragma once

#include "ConnectionProfile.h"

#include <QList>

// 接続設定の永続化 (QSettings: macOSはplist、Linuxはiniに保存される)。
// 保存するパスワードはBase64+XORによる簡易的な難読化のみを行う。これは公式Managerと
// 同程度の保護レベル(暗号学的な安全性はない)であり、真に機密性が必要ならOSの
// キーチェーン連携を別途検討すること。
class ConnectionProfileStore
{
public:
    static QList<ConnectionProfile> loadAll();
    static void saveAll(const QList<ConnectionProfile> &profiles);
};
