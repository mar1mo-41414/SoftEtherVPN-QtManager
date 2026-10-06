#!/usr/bin/env python3
"""i18n/*.ts の英語訳を、docs/upstream-reference/strtable_ja.stb / strtable_en.stb
(公式SoftEtherの文字列テーブル、Apache-2.0) を突き合わせて機械的に流し込むスクリプト。

UI文言の原文 (tr() の第一引数) は日本語で、かつ多くは strtable_ja.stb の文言をそのまま
使っている (各ダイアログのコメントに "// D_SM_XXX" 等のキーを書いてある)。このスクリプトは
strtable_ja.stb の同じ日本語文字列に対応する strtable_en.stb の英語訳を探し、一致すれば
.ts にその訳文を書き込む。

使い方:
    1. lupdate で .ts を最新化する (CMakeの update_translations ターゲットでも可):
         lupdate $(find src -name '*.cpp' -o -name '*.h') -ts i18n/SoftEtherVPN-QtManager_en.ts
    2. このスクリプトを実行して訳文を流し込む:
         python3 tools/gen_translations.py
    3. ビルドすると lrelease で .qm にコンパイルされ、リソースとして埋め込まれる。

完全自動では一致しない文字列 (このスクリプト作成時点で約2割) が残る。それらは実行時に
日本語のまま表示される (Qtの通常のフォールバック)。精度を上げたい場合は、このスクリプトの
EXTRA_TRANSLATIONS に追記するか、i18n/*.ts をQt Linguistで直接編集してよい
(このスクリプトは次回実行時に既存の訳文を上書きするので、手動編集した分はEXTRA側に
移しておくこと)。
"""
import json
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
STB_JA = ROOT / "docs/upstream-reference/strtable_ja.stb"
STB_EN = ROOT / "docs/upstream-reference/strtable_en.stb"
TS_FILE = ROOT / "i18n/SoftEtherVPN-QtManager_en.ts"

# 公式の文字列テーブルに無い、アプリ独自の汎用UI文言の手動補完。
EXTRA_TRANSLATIONS = {
    "OK": "OK",
    "エラー": "Error",
    "確認": "Confirm",
    "入力エラー": "Input Error",
    "完了": "Done",
    "情報": "Information",
    "警告": "Warning",
    "未実装": "Not implemented",
    "未対応": "Not supported",
    "はい": "Yes",
    "いいえ": "No",
    "ユーザー名(U):": "User Name(&U):",
    "パスワード(P):": "Password(&P):",
    "優先順位(R):": "Priority(&R):",
    "確認入力(C):": "Confirm Input(&C):",
    "ホスト名(H):": "Host Name(&H):",
    "ポート番号(P):": "Port Number(&P):",
    "本名(R):": "Real Name(&R):",
    "説明(N):": "Note(&N):",
    "TCP コネクション数:": "Number of TCP Connections:",
    "接続開始時刻:": "Connection Start Time:",
    "証明書の指定": "Specify Certificate",
    "ファイルを開けませんでした: %1": "Could not open the file: %1",
    "ファイルを保存できませんでした。": "Could not save the file.",
    "X.509 証明書として読み込めませんでした。": "Could not load this as an X.509 certificate.",
    "X.509 証明書ファイル (*.cer *.crt *.pem *.der);;すべてのファイル (*)":
        "X.509 Certificate Files (*.cer *.crt *.pem *.der);;All Files (*)",
    "%1 のプロパティ": "Properties of %1",
    "%1 / %2": "%1 / %2",
    "管理マネージャの終了(&X)": "E&xit Manager",
    "仮想 HUB の管理 - %1": "Manage Virtual Hub - %1",
}


def parse_stb(path):
    """戻り値: (flat: {key: text} PREFIXブロックの外側), (blocks: {prefix: {key: text}})"""
    flat, blocks = {}, {}
    current_block = None
    for raw in path.open(encoding="utf-8"):
        line = raw.rstrip("\n").rstrip("\r")
        if not line.strip() or line.strip().startswith("#"):
            continue
        m = re.match(r"^([A-Za-z0-9_]+)\t+(.*)$", line)
        if not m:
            continue
        key, text = m.group(1), m.group(2).strip()
        if key == "PREFIX":
            current_block = {}
            blocks[text] = current_block
            continue
        (current_block if current_block is not None else flat)[key] = text
    return flat, blocks


def build_mapping():
    ja_flat, ja_blocks = parse_stb(STB_JA)
    en_flat, en_blocks = parse_stb(STB_EN)
    mapping = {}

    def add_pairs(ja_kv, en_kv):
        for k, ja_text in ja_kv.items():
            en_text = en_kv.get(k)
            if en_text and ja_text and ja_text not in mapping:
                mapping[ja_text] = en_text

    add_pairs(ja_flat, en_flat)
    for prefix, ja_kv in ja_blocks.items():
        add_pairs(ja_kv, en_blocks.get(prefix, {}))

    mapping.update(EXTRA_TRANSLATIONS)
    return mapping


def amp_variants(s):
    """Windowsニーモニック表記 "(&X)" の有無・前のスペースの有無のゆらぎを吸収する。"""
    yield s
    with_amp = re.sub(r"\((?!&)([A-Za-z0-9])\)", r"(&\1)", s)
    if with_amp != s:
        yield with_amp
    spaced = re.sub(r"(?<!\s)(\(&[A-Za-z0-9]\))$", r" \1", with_amp)
    if spaced != with_amp:
        yield spaced
    spaced2 = re.sub(r"(?<!\s)(\([A-Za-z0-9]\))$", r" \1", s)
    if spaced2 != s:
        yield spaced2
    stripped = re.sub(r"\s*\(&?[A-Za-z0-9]\)$", "", s)
    if stripped != s:
        yield stripped


def qt_to_stb_placeholders(s):
    """Qtの %1,%2,... を stb の %S に順番に置き換える。該当無しなら None。"""
    count = len(re.findall(r"%\d", s))
    if count == 0:
        return None
    out = s
    for i in range(1, count + 1):
        out = out.replace(f"%{i}", "%S", 1)
    return out, count


def stb_to_qt_placeholders(en_text, count):
    out = en_text
    for i in range(1, count + 1):
        out = out.replace("%S", f"%{i}", 1)
        out = out.replace("%s", f"%{i}", 1)
    return out


def main():
    mapping = build_mapping()
    tree = ET.parse(TS_FILE)
    root = tree.getroot()

    total = matched = 0
    unmatched = []
    for context in root.findall("context"):
        for message in context.findall("message"):
            source_el = message.find("source")
            if source_el is None or not source_el.text:
                continue
            total += 1
            src = source_el.text.strip()
            en = None
            for cand in amp_variants(src):
                en = mapping.get(cand)
                if en:
                    break
            if en is None:
                conv = qt_to_stb_placeholders(src)
                if conv:
                    stb_src, count = conv
                    for cand in amp_variants(stb_src):
                        hit = mapping.get(cand)
                        if hit:
                            en = stb_to_qt_placeholders(hit, count)
                            break
            if en is None:
                unmatched.append(src)
                continue
            matched += 1
            trans_el = message.find("translation")
            if trans_el is None:
                trans_el = ET.SubElement(message, "translation")
            trans_el.text = en
            trans_el.attrib.pop("type", None)

    tree.write(TS_FILE, encoding="utf-8", xml_declaration=True)
    print(f"{matched}/{total} messages translated ({total - matched} left in Japanese)",
          file=sys.stderr)


if __name__ == "__main__":
    main()
