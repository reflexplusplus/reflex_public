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

## Editing the grammar

The grammar is `Syntaxes/glx.tmLanguage.json`. After editing, reload it in JetBrains: remove and re-add the bundle in TextMate Bundles preferences (JetBrains caches parsed grammars).
