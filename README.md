# Chinese Text Editor (Linux / POSIX Terminal Port)

A Chinese and English full-screen terminal text editor in C, originally built for DOS (Turbo C 3.0 + Borland BGI) and refactored to compile and run natively on Linux (and macOS) with **zero external dependencies**.

## Building & Running

```bash
make
./editor
```

> Run `./editor` in a UTF-8 terminal of at least **80×28**. Saved and opened files live in `SAVE/`.

### Tests & Clean
```bash
make test
make clean
```

## Chinese & English Input
- Type **English** and **Chinese** directly at any time using your OS/terminal's native UTF-8 input method (IME) or by pasting UTF-8 text — incoming UTF-8 Chinese characters and full-width punctuation are automatically converted to GB18030 via POSIX `iconv()` and stored as double-width document nodes.

## Where Saved Files Are Stored
- Files saved with `Ctrl+S` (Save) or `Ctrl+D` (Save As) are written to **`SAVE/<filename>`** (for example `SAVE/NONAME.TXT` or `SAVE/notes.txt`). The `SAVE/` directory is created automatically if missing.
- If you enter a path containing `/` (e.g. `./notes.txt`), it saves directly to that path.
- After saving, the bottom status bar displays the exact saved path (e.g. `Saved: SAVE/notes.txt`).
- `Ctrl+O` (Open) opens an interactive file browser listing existing files in `SAVE/` (navigate with `Up`/`Down`, mouse click, or type a filename).

## Controls & Shortcuts

| Key / Action | Function |
|---|---|
| `F1` / `F2` / `F3` / `F4` | Open / switch File, Edit, Tools, Help pull-down menus (`Left`/`Right` also switch) |
| Arrow keys | Move cursor / scroll viewport |
| `Ctrl+Left` / `Ctrl+Right` | Extend text selection (or `Shift+Left` / `Shift+Right`) |
| `Ctrl+A` | Select all visible text on current page |
| `Ctrl+C` / `Ctrl+X` / `Ctrl+V` | Copy / Cut / Paste |
| `Ctrl+F` / `Ctrl+R` | Find / Find & Replace |
| `Ctrl+N` / `Ctrl+O` / `Ctrl+S` / `Ctrl+D` | New / Open / Save / Save As |
| `Esc` | Cancel dialog/menu, or prompt Exit in editor |
| Mouse | Click menus (switch directly between open menus), dialogs, scrollbar arrows, or text cells |

## Project Layout
- `editer/` — Core C editor modules (linked list, renderer, menus, file I/O, mouse, commands)
- `h/` — Public module headers and shared data structures
- `platform/` — Zero-dependency Linux/POSIX terminal BGI & DOS compatibility shim (`bgi_tui.c`)
- `SAVE/` — Default directory for saved and sample text files
- `tests/` — Automated C unit test suite (`make test`)
