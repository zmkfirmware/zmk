# TextMate grammars

Shiki, which powers Expressive Code, does not bundle grammars for Devicetree
or Kconfig. These grammar files are registered through
`expressiveCode.shiki.langs` in `docs/astro.config.mjs`.

## `dts.tmLanguage.json`

- Source: <https://github.com/kylebonnici/dts-lsp/blob/70e48f3f31278bdb74b85b11cea79f7f0fc5b6f9/syntax/dts.tmLanguage.json>
- License: Apache-2.0, see `LICENSE-dts-lsp.txt`.
- Local changes: added the Shiki `name` and `aliases` fields.

## `kconfig.tmLanguage.json`

- Source: <https://github.com/trond-snekvik/vscode-kconfig/blob/548263965f1d4c894801ff4171dd15606db422cc/syntaxes/kconfig.tmGrammar.json>
- License: MIT, see `LICENSE-vscode-kconfig.txt`.
- Local changes: added the Shiki `name` and `aliases` fields, and added the
  `#conf-entry` rule so that `.conf`/`_defconfig` assignments such as
  `CONFIG_ZMK_SLEEP=y` are highlighted in addition to Kconfig definition
  syntax.
