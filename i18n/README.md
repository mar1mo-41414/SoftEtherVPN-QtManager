# i18n

UI文言の原文 (`tr()` の第一引数) は日本語。`SoftEtherVPN-QtManager_en.ts` が英語訳。

- 実行時は `src/main.cpp` が OS のロケールに応じて自動的に読み込む
  (`SEQTM_LANG=en` 環境変数で強制指定も可能。テスト用)。
- ビルド時に CMake (`qt_add_translations`) が `.ts` を `.qm` にコンパイルし、
  `:/i18n/` リソースとして実行ファイルに埋め込む。
- 未訳の文字列は実行時に日本語のまま表示される (Qtの通常のフォールバック動作)。

## 更新・翻訳の追加手順

```bash
# 1. ソースを再スキャンして .ts に新しい文字列を追加する
find src -name '*.cpp' -o -name '*.h' | sort | lupdate @/dev/stdin -ts i18n/SoftEtherVPN-QtManager_en.ts

# 2. 公式の文字列テーブル (docs/upstream-reference/strtable_ja/en.stb) と突き合わせて
#    機械的に訳文を流し込む (一致しない分はそのまま日本語で残る)
python3 tools/gen_translations.py
```

手動で訳文を追加・修正したい場合は、Qt Linguist (`linguist i18n/SoftEtherVPN-QtManager_en.ts`)
で直接編集するか、`tools/gen_translations.py` の `EXTRA_TRANSLATIONS` に追記してから
再実行する（再実行すると `.ts` の訳文は上書きされるため、手動編集はこのスクリプトでは
覆されない箇所に限るか、`EXTRA_TRANSLATIONS` に移しておくこと）。
