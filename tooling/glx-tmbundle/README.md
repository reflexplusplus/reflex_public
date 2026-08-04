# GLX TextMate Bundle

TextMate grammar for Reflex `.glx` stylesheet files. Covers comments, `@Font`/`@Bitmap`/`@Colour`/`@Alias`/`@State` declarations, `#option` directives, `$RRGGBB[AA]` hex colours, numbers, `&ref` property lookups, function-call values (`Fill(...)`, `Text(...)`, etc.), selectors / key-value pairs, strings, and block punctuation.

## Install in JetBrains IDEs (CLion, IntelliJ, etc.)

1. **Settings** → **Editor** → **TextMate Bundles**
2. Click **+** and select this directory (`$REFLEX_PATH/tooling/glx-tmbundle`)
3. **Apply**

Files with the `.glx` extension are now highlighted. No restart required.

If `.glx` is also being recognised as plain text, force the file type:
**Settings** → **Editor** → **File Types** → find "GLX" in the TextMate-registered list, ensure `*.glx` is listed.

## What's covered

- `// line comments`
- `@Font Name: { ... };`, `@Bitmap`, `@Colour name: $hex;`, `@Alias`, `@State foo: { ... };`
- `#option margin_syntax css` style directives
- `$RRGGBB` / `$RRGGBBAA` palette literals
- Numeric literals (incl. negatives, decimals)
- `&property_name` runtime-property references
- `Function(arg: value; ...)` call expressions
- `Selector:` and `key:` highlighted as tags
- `"strings"` with escapes
- `{ } ; : ,` punctuation

## What it doesn't do

Pure syntactic colouring — no diagnostics, completion, refactor, go-to-definition. For that, a Language Server (VS Code) or full IntelliJ plugin (JetBrains) would be needed; this bundle is a one-file declarative grammar only.

## Bundle layout

This directory is a multi-format bundle so it loads in every editor:

- `package.json` — VS Code-style manifest. **This is what JetBrains (CLion/IntelliJ) uses.** The IDE classifies a bundle directory by looking for `package.json` *before* `info.plist`; finding it, it loads as a VS Code bundle and reads the JSON grammar named under `contributes.grammars`. Without it, an `info.plist`-only directory whose grammar is JSON is reported as having an "unknown format".
- `info.plist` + `Syntaxes/glx.tmLanguage` (XML plist) — the classic TextMate.app format. Inert in JetBrains (the manifest takes precedence) but kept so TextMate.app can load the bundle too.
- `Syntaxes/glx.tmLanguage.json` — JSON grammar, the source of truth, referenced by `package.json`.

## Editing the grammar

Edit `Syntaxes/glx.tmLanguage.json`, then regenerate the classic plist grammar so the two stay in sync:

```sh
python3 -c "import json,plistlib; d=json.load(open('Syntaxes/glx.tmLanguage.json')); d.pop('\$schema',None); plistlib.dump(d, open('Syntaxes/glx.tmLanguage','wb'), sort_keys=False)"
```

After editing, reload in JetBrains: remove and re-add the bundle in TextMate Bundles preferences (JetBrains caches parsed grammars).
