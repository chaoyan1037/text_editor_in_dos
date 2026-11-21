# Chinese Text Editor (Linux / POSIX Terminal Port)

A Chinese and English full-screen terminal text editor in C, originally built for DOS (Turbo C 3.0 + Borland BGI) and refactored to compile and run natively on Linux (and macOS) with **zero external dependencies**.

## Building & Running

```bash
make
./editor
```

> Run `./editor` from the repository root in a UTF-8 terminal of at least **80×28** so it can load the Pinyin dictionary (`spell/`) and save/open files in `SAVE/`.

### Tests & Clean
```bash
make test
make clean
```

## Where Saved Files Are Stored
- Files saved with `Ctrl+S` (Save) or `Ctrl+D` (Save As) are written to **`SAVE/<filename>`** (for example `SAVE/NONAME.TXT` or `SAVE/notes.txt`). The `SAVE/` directory is created automatically if missing.
- If you enter a path containing `/` (e.g. `./notes.txt`), it saves directly to that path.
- After saving, the bottom status bar displays the exact saved path (e.g. `Saved: SAVE/notes.txt`).
- `Ctrl+O` (Open) checks `SAVE/<filename>` first, then `<filename>` directly.

## Controls & Shortcuts

| Key / Action | Function |
|---|---|
| `F1` / `F2` / `F3` / `F4` | Open File / Edit / Tools / Help pull-down menus |
| `F8` | Toggle English (`英`) / Chinese Pinyin (`汉`) input mode |
| Arrow keys | Move cursor / scroll viewport |
| `Ctrl+Left` / `Ctrl+Right` | Extend text selection (or `Shift+Left` / `Shift+Right`) |
| `Ctrl+A` | Select all visible text on current page |
| `Ctrl+C` / `Ctrl+X` / `Ctrl+V` | Copy / Cut / Paste |
| `Ctrl+F` / `Ctrl+R` | Find / Find & Replace |
| `Ctrl+N` / `Ctrl+O` / `Ctrl+S` / `Ctrl+D` | New / Open / Save / Save As |
| `Esc` | Cancel dialog/menu/IME, or prompt Exit in editor |
| Mouse | Click menus, dialogs, scrollbar arrows, EN/ZH button, or text cells |

### Chinese Pinyin Input (`F8`)
1. Press `F8` to switch to `汉` mode.
2. Type a lowercase pinyin syllable (e.g. `zhong`, `guo`, `ni`, `hao`) and press `Space` to load candidates from `spell/<pinyin>.txt`.
3. Press `-` / `=` to page backward/forward through candidates, `1`–`4` (or `Space` for #1) to insert, or `Esc` to cancel.

## Project Layout
- `editer/` — Core C editor modules (linked list, renderer, menus, IME, file I/O, mouse, commands)
- `h/` — Public module headers and shared data structures
- `platform/` — Zero-dependency Linux/POSIX terminal BGI & DOS compatibility shim (`bgi_tui.c`)
- `spell/` — Pinyin → GB2312 candidate tables (`*.txt`) for the built-in `F8` Chinese IME
- `SAVE/` — Default directory for saved and sample text files
- `tests/` — Automated C unit test suite (`make test`)
