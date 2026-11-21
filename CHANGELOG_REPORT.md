# Detailed Change & Bug-Fix Report (`master` Branch)

This report documents every bug fix, optimization, refactoring, and documentation improvement committed on the **`master`** branch from the original DOS baseline (`64a59d0`) through `8906141`.

> **Note:** The `master` branch preserves full **DOS / Turbo C 3.0 + Borland BGI** compatibility (C89 syntax, `HZK16`, and `spell/`). The separate Linux/POSIX terminal port lives on the `main` branch.

## Commit History on `master`

| Commit | Summary | Scope |
|---|---|---|
| `7ace1b5` | `fix bug & code optimization` | 30 files (`14` `.C`, `15` `.H`, `README.md`) — all memory-safety, crash, I/O, logic, and performance fixes |
| `d0daddb` | `add comments` | Structured Chinese/English module & function comments across the codebase |
| `8906141` | `add more comments` | Comprehensive C89 API doc-blocks (`@param`, `@return`, `@pre`, `@post`, `@complexity`), ASCII architecture/state diagrams, and invariant specs across all 29 source/header files |

**Total delta (`64a59d0..8906141`):** 30 files changed, `+3,907` insertions / `-2,361` deletions.

---

## 1. Critical Bugs Fixed

### 1.1 Memory Safety: Use-After-Free, Double-Free & Leaks

| File | Function(s) | Original Bug | Fix |
|---|---|---|---|
| [editer/NODE.C](editer/NODE.C) | `FreeAllNode()` | Loop header `for(; pNode!=NULL; pNode=pTemp->next)` read `pTemp->next` **after** `free(pNode)` (use-after-free) and unconditionally called `ClearClip()` even when `clipStart..clipLast` pointed into the already-freed main list (double-free). | Saved `pNext = pNode->next` before `free(pNode)`. Only calls `ClearClip()` when `copyCutFlag == 1` (detached clipboard); otherwise clears `clipStart`/`clipLast`/`findStart`/`findLast` safely. |
| [editer/NODE.C](editer/NODE.C) | `ClearClip()` | Used `pTemp = pNode` and freed `pNode` before checking `pTemp == pEdit->clipLast` or advancing `pNode = pNode->next` (use-after-free). | Saves `pNext = (pNode == pEdit->clipLast) ? NULL : pNode->next` before `free(pNode)` and resets `clipStart`, `clipLast`, `copyCutFlag`, `copyActive`, `clipFlag`. |
| [editer/NODE.C](editer/NODE.C) | `CopySelection()` *(new)* | `Ctrl+C` originally kept `clipStart..clipLast` pointing directly into the live document list, causing **infinite self-paste loops** when pasting inside a selection and **use-after-free/double-free** if copied text was edited or deleted. | Deep-copies `clipStart..clipLast` into a detached heap-allocated list (`clipStart->last == NULL`, `clipLast->next == NULL`) and sets `copyCutFlag = 1`, `copyActive = 1`. |
| [editer/NODE.C](editer/NODE.C) | `DeleteNode()` | Deleting a node referenced by `clipStart`, `clipLast`, `findStart`, or `findLast` left dangling pointers; deleting `pageFirst` or the last node on a scrolled page left `pageFirst`/`current` invalid. | Guards `pNode == NULL`, bridges `pPrev <-> pNext`, repairs `start`, `last`, `pageFirst` (backing up to the previous line's `firstInLine` if deleting the tail of a scrolled page), `current`, and nulls out invalidated live selection/find pointers before `free(pNode)`. |
| [editer/FUNCKEY.C](editer/FUNCKEY.C) | `ReplaceFunction()` | When replacement text was shorter than the matched span, `DeleteNode()` was called while advancing through already-freed nodes; also hijacked `clipStart`/`clipLast`, leaking or clobbering the user's clipboard. | Switched Find/Replace to dedicated non-owning `findStart`/`findLast` fields in [h/STRUCT.H](h/STRUCT.H). Saves `pNext = pNode->next` before `DeleteNode()` in Phase 2b; handles equal, longer, shorter, and empty (`""`) replacements cleanly. |
| [editer/EDITER2.C](editer/EDITER2.C) | `InitStruct()` | Called `InitMouse()` and exited on failure **before** initializing linked-list pointers (`start`, `clipStart`, `phz`), risking freeing uninitialized stack garbage. | Added `memset(pEdit, 0, sizeof(struct EDITTYPE))` at the very start of `InitStruct()` and moved the `InitMouse()` check to the end, exiting cleanly with `exit(1)`. |
| [editer/FUNCKEY.C](editer/FUNCKEY.C), [editer/MALLOC.C](editer/MALLOC.C) | `ExitEditer()`, `MallocNode()` | `ExitEditer()` called `ClearClip()` after `FreeAllNode()` (double-free risk), leaked the `HZK16` file handle, and did not call `exit(0)`; `MallocNode()` exited with status `0` without closing `HZK16`. | `ExitEditer()` calls `FreeAllNode()`, `close_hz()`, and `exit(0)`; `MallocNode()` closes `HZK16` and exits with `exit(1)` on OOM. |

---

### 1.2 NULL Dereferences, Crashes & Infinite Loops

| File | Function(s) | Original Bug | Fix |
|---|---|---|---|
| [editer/LANGUAGE.C](editer/LANGUAGE.C), [editer/SPELL.C](editer/SPELL.C) | `GetEnglish()`, `GetEnter()`, `Gethanzi()` | Checked `(pEdit->start != NULL) && (pEdit->current->next == NULL)` — crashed with a **NULL dereference** whenever typing or pressing Enter at the top-left of a non-empty page (`current == NULL`). | Changed guard to `pEdit->current != NULL && pEdit->current->next == NULL` so `current == NULL` safely routes to `InsertNode()`. |
| [editer/NODE.C](editer/NODE.C) | `AddNode()`, `InsertNode()` | `AddNode()` unconditionally dereferenced `pEdit->current->next`; `InsertNode()` dereferenced `pEdit->current->next->last` (crashing at tail) and `pEdit->pageFirst->last` (crashing on empty doc) and broke links when inserting before `pageFirst` on a scrolled page. | Added full empty-list, head, scrolled-`pageFirst`, interior, and tail handling with `data[2] = '\0'` termination in both functions. |
| [editer/FUNCKEY.C](editer/FUNCKEY.C) | `Paste()` | Pasting into an empty document (`start == NULL`) left `pEdit->start == NULL` while setting `current`/`pageFirst`/`last` to the new node (corrupting the list head), and crashed on `current->next` when `current == NULL`. | Explicitly initializes `start = pageFirst = last = current = p` with `firstInLine = 1` when `start == NULL`, guards `copyActive == 1 && copyCutFlag == 1 && clipStart != NULL`, and safely handles `current == NULL`. |
| [editer/FUNCKEY.C](editer/FUNCKEY.C) | `CopyCut()` | Assumed `clipStart == pageFirst` implied `pageFirst->next != NULL` (crashing when cutting the entire document), broke `start`/`pageFirst` when cutting a span crossing `pageFirst`, and allowed repeated `Ctrl+X` to corrupt an already-detached clipboard. | Guards `copyCutFlag == 0`, scans whether `pageFirst` lies inside `clipStart..clipLast`, stitches `pPrev <-> pNext`, repairs `start`, `last`, `pageFirst`, and `current`, and severs outer links (`clipStart->last = NULL`, `clipLast->next = NULL`). |
| [editer/FUNCKEY.C](editer/FUNCKEY.C) | `AllChosen()` | Dereferenced `pageFirst` when document was empty (`pageFirst == NULL`), leaked any previous detached clipboard, and miscounted Hanzi widths (`flag == 2`) causing `count == CHARINLINE` to miss wraps. | Early-returns on `pageFirst == NULL`, frees any existing detached clip via `ClearClip()`, counts visual width with `count >= CHARINLINE`, and guards `current == NULL` inside the highlight loop. |
| [editer/FUNCKEY.C](editer/FUNCKEY.C) | `MoveCursor()` | Scrolling `UP` at row 1 dereferenced `pEdit->pageFirst->last` without checking `pageFirst != NULL`; scrolling `DOWN` at row 25 did not guard `pageFirst != NULL`. | Added `pEdit->pageFirst != NULL && pEdit->pageFirst->last != NULL` guards before traversing backward/forward. |
| [editer/DRAWMENU.C](editer/DRAWMENU.C) | `LocateRow()` | Checked `pNode->firstInLine` before checking `pNode != NULL`, risking a NULL dereference if `firstInLine` metadata was cleared. | Changed loop condition to `pNode != NULL && pNode->firstInLine == 0` with safe fallback `*pRow = 1`. |
| [editer/HANZI.C](editer/HANZI.C) | `Get_hz()`, `Displayhz()`, `Printfhz()` | Did not check `phz == NULL` or `s == NULL`, and `Printfhz()` only checked `*s != '\0'` (could read past NUL on odd-length strings). | Added NULL guards, lazy `open_hz()` retry, `memset(bytes, 0, 32)` on read error, and `s[0] != '\0' && s[1] != '\0'` termination. |

---

### 1.3 Buffer Overflows, Integer Overflow & Bounds Bugs

| File | Function(s) | Original Bug | Fix |
|---|---|---|---|
| [h/STRUCT.H](h/STRUCT.H), [editer/INNAME.C](editer/INNAME.C) | `struct SPELLTYPE`, `struct FILETYPE`, `InName()` | `spell.name` was only `char name[10]`, yet `InName()` allowed up to `12` characters (**buffer overflow**). `file.fname` was a `char *` aliased directly to `spell.name`, so opening/saving another file silently corrupted the active filename. | Expanded both to dedicated fixed buffers (`char name[16]`, `char fname[16]`), enforced `i < 12` before writing, fixed off-by-one `BACKSPACE`, and added `ESC` cancel handling. |
| [editer/SPELL.C](editer/SPELL.C) | `InputPinYin()`, `OutputHanZi()`, `GetCode()` | `GetCode()` read dictionary bytes into a stack buffer with **no length limit** (overflowing on large pinyin files). `OutputHanZi()` used raw pointer arithmetic (`s -= 8`, `s += 8`) that could underflow before `code[]`, read past the end on partial last pages, and crashed when pressing `'1'..'4'` on an empty slot; also had no `ESC` cancel. | Passed explicit `maxLen` to `GetCode()` (`i < maxLen - 1`), added `ESC` cancel in both `InputPinYin()` and `OutputHanZi()`, replaced raw pointer math with bounds-checked integer page/slot indexing (`idx = page * 4 + choice`), and preserved the special `'垚'` glyph on `yao` page 8 slot 3. |
| [editer/HANZI.C](editer/HANZI.C) | `Get_hz()` | Computed `qh = incode[0] - 0xa0` and `offset = (94*(qh-1)+(wh-1))*32l` using signed `char` / unvalidated bytes; any byte `< 0xa1` underflowed `unsigned char` and sought to an invalid file offset. | Cast to `unsigned char`, validated `b0 >= 0xa1 && b1 >= 0xa1`, and used `unsigned long` arithmetic `(94UL*(b0-0xa1) + (b1-0xa1)) * 32UL`. |
| [editer/DRAWMENU.C](editer/DRAWMENU.C) | `RollBar()` | Thumb size and offset math could overflow 16-bit `int` on Turbo C or divide by zero when `totalLines == CHARINROW`, and the thumb could paint outside the track over the up/down arrow buttons. | Promoted ratio calculations to 32-bit `long`, guarded `maxScroll > 0`, and clamped `thumbHeight` to `[14, trackHeight]` and `thumbY` to `[trackTop, trackBottom - thumbHeight]`. |
| [editer/FUNCKEY.C](editer/FUNCKEY.C) | `FindFunction()`, `FindAndReplace()` | Missing `ESC` cancellation (trapped user until `ENTER`), stale previous `findGoal`/`replaceGoal` buffers, and `BACKSPACE` off-by-one clearing. | Zeroed buffers on entry, added `ESC` -> `CancleFind()`/`CancleReplace()`, and fixed `i--` / `j--` before NUL termination. |

---

### 1.4 File I/O Bugs

| File | Function(s) | Original Bug | Fix |
|---|---|---|---|
| [editer/SAVE.C](editer/SAVE.C) | `Save()`, `Saveas()` | Did not check `fopen() == NULL` (crashing if directory missing). Inside the write loop, `if(p->data[0] == 0) { ... fclose(fp); }` **closed `fp` mid-loop** and kept writing to a closed `FILE*`! `Saveas()` aliased `file.fname = spell.name`. | Added `filename` & `fp == NULL` checks, relative `SAVE/` fallback if `C://TC30//BIN//SAVE//` fails, clean `flag == 0 / 1 / 2` serialization (`fputc`), and `strncpy` into `file.fname`. |
| [editer/OPEN.C](editer/OPEN.C) | `Open()`, `New()` | Used the classic `while(feof(fp) == 0)` anti-pattern (processing an extra garbage `EOF` byte at end of file), failed to handle `\r` (CRLF) or `\t` (tabs mis-synced GB2312 pairs), did not set `functionActive = 1` on invalid-filename alert, and aliased `file.fname = spell.name`. | Replaced with `while((r = fgetc(fp)) != EOF)`, skipped `\r`, expanded `\t` to 4 spaces, checked `r2 != EOF` for GB2312 pairs, copied filename into `file.fname`, added `SAVE/` fallback, and invalidated scrollbar/status caches in `New()`. |
| [editer/HANZI.C](editer/HANZI.C) | `open_hz()`, `close_hz()` | Reopened `HZK16` repeatedly or leaked the file handle; only checked hardcoded `C://TC30//BIN//hanziku//HZK16`. | Kept a single persistent `FILE *phz` with relative `hanziku/HZK16` fallback and clean shutdown via `close_hz()`. |

---

### 1.5 UI, Mouse & State-Machine Bugs

| File | Function(s) | Original Bug | Fix |
|---|---|---|---|
| [editer/MOUSE.C](editer/MOUSE.C) | `MouseKey()`, `ChooseSubmenu()` | Tested modal button coordinates (`246..306 x 214..232`, etc.) **before** checking `fa != 0 && b == 1`, creating dead zones over the text viewport and flashing `button3d()` merely on hover. Clicking the top menu bar did not clear `state.key`, so a stale `F1`–`F4` keypress could override the clicked menu. | Gated all modal dialog hit-testing behind `if(fa != 0 && b == 1)` with early `return`, excluded the 20 px scrollbar from viewport text clicks, gated scrollbar arrows on `fa == 0 && editActive == 0`, and cleared `pEdit->state.key = 0` on menu clicks. |
| [editer/VIEW.C](editer/VIEW.C) | `ViewAll()` | Never cleared stale `firstInLine = 1` flags on subsequent nodes after edits/deletions shifted line wraps, corrupting `UP`/`DOWN` scrolling, `LocateRow()`, and `RollBar()`. | Explicitly sets `pNode->firstInLine = 1` on `pageFirst` and resets `pNode->next->firstInLine = 0` before each wrap/newline test across the full tail of the document. |
| [editer/SUBMENUS.C](editer/SUBMENUS.C) | `Submenu()` | Indexed `SubmenuNum[pEdit->editActive - 1]` without bounds checking `editActive` in `1..4`. | Added `pEdit->editActive >= 1 && pEdit->editActive <= 4` bounds check. |
| [editer/EDITER2.C](editer/EDITER2.C) | `FunctionActiveSwitch()` | Pressing `ESC` in modal confirmations (`Exit`, `Save`, `Open`, `New`) had no escape path; pressing a non-copy/cut key after `Ctrl+A` (`functionActive == 13`) left selection highlight stuck on screen. | Added `ESC` cancel handling across modal dialogs and clean highlight dismissal in state `13`. |

---

## 2. Performance & Architecture Optimizations

1. **O(75 × N²) → O(page) Cursor & Click Mapping ([editer/NODE.C](editer/NODE.C), [editer/FUNCKEY.C](editer/FUNCKEY.C))**
   - Previously, `MoveCursor()` (`UP`/`DOWN`) stepped `x` leftward pixel-by-pixel (up to 75×), calling `ConvertNodePosi()`, which itself called `GetNodePosi()` inside a loop — resulting in O(75 × N²) scans on every vertical arrow press.
   - Replaced with single-pass `ConvertNodePosi()` and `MoveCursorVertical()`, each walking `pageFirst` **once** (O(visible page)).
2. **ASCII Run Batching in Viewport Renderer ([editer/VIEW.C](editer/VIEW.C))**
   - Previously called BGI `outtextxy()` once per individual ASCII character.
   - Now accumulates contiguous `flag == 1` ASCII nodes per row into `asciiBuf[CHARINLINE + 2]` and flushes per line/Hanzi/wrap — up to **75× fewer BGI draw calls per line**.
3. **O(1) Idle-Frame Caching for Status Bar & Scrollbar ([editer/DRAWMENU.C](editer/DRAWMENU.C), [h/STRUCT.H](h/STRUCT.H))**
   - `ShowPosition()` and `RollBar()` run on every main-loop iteration. Added change-detection caches (`lastPosCurrent`, `lastPosPageFirst`, `lastPosX`, `lastPosY`, `lastRollPageFirst`) so idle frames return immediately in O(1) without walking the document list or redrawing BGI graphics.
4. **Unified Search Engine ([editer/FUNCKEY.C](editer/FUNCKEY.C))**
   - Replaced 3 separate, partially broken search functions (`FindChar`, `FindString`, `FindHanziString`) with a single unified `Find()` supporting ASCII, GB2312 Hanzi, and mixed ASCII+Hanzi substrings, with wrap-around search and single-pass highlight rendering.
5. **Deduplicated Menu Rendering & Persistent Font Handle ([editer/SUBMENUS.C](editer/SUBMENUS.C), [editer/HANZI.C](editer/HANZI.C))**
   - Consolidated `SubmenuF1`–`SubmenuF4` behind a shared `DrawSubmenuBox()` helper.
   - Replaced repeated `HZK16` `fopen`/`fclose` calls with a persistent handle (`open_hz()`/`close_hz()`) and eliminated the 8-byte mask lookup table in `Displayhz()` in favor of bit shifts.
6. **Strict C89 Compliance & Header Hygiene (All 15 `h/*.H` & 14 `editer/*.C`)**
   - Changed `void main(void)` to standard `int main(void)`, replaced `//` C++ comments and `h//...` include paths with C89 `/* ... */` and `h/...`, added missing standard includes and forward `struct EDITTYPE;`/`struct NODE;` declarations in all headers.

---

## 3. Comprehensive Documentation (`d0daddb` & `8906141`)

Every source file (`editer/*.C`, 14 files) and header (`h/*.H`, 15 files) was documented in strict C89 style (`/* ... */`):
- **Architecture & protocol diagrams:** 640×480 VGA screen layout map in [h/COMMON.H](h/COMMON.H), linked-list / caret / clipboard ownership invariants in [h/STRUCT.H](h/STRUCT.H), event-loop and modal state-machine diagrams in [editer/EDITER2.C](editer/EDITER2.C), and Pinyin IME state flow in [editer/SPELL.C](editer/SPELL.C).
- **Function doc-blocks:** `@param`, `@return`, `@pre`, `@post`, `@note`, and `@complexity` annotations for all public and helper routines.

---

## 4. Per-File Change Summary on `master`

| File | Key Changes |
|---|---|
| [editer/EDITER2.C](editer/EDITER2.C) | `int main(void)`, `memset` in `InitStruct()`, safe `InitMouse()` failure order, `strcpy` for `file.fname`, `CopySelection()` dispatch, modal `ESC` & Select-All cancel, full docs |
| [editer/NODE.C](editer/NODE.C) | Safe `AddNode()`/`InsertNode()`/`DeleteNode()`/`FreeAllNode()`/`ClearClip()`, new deep-copy `CopySelection()`, O(page) `ConvertNodePosi()` & `MoveCursorVertical()` |
| [editer/FUNCKEY.C](editer/FUNCKEY.C) | Safe `Paste()`/`CopyCut()`/`AllChosen()`/`MoveCursor()`, unified `Find()` + isolated `findStart`/`findLast`, safe `ReplaceFunction()`, `ESC`/backspace fixes in Find/Replace dialogs, clean `ExitEditer()` |
| [editer/VIEW.C](editer/VIEW.C) | ASCII run batching, full `firstInLine` resynchronization across document tail |
| [editer/DRAWMENU.C](editer/DRAWMENU.C) | O(1) idle caches in `ShowPosition()` and `RollBar()`, 32-bit `long` scrollbar clamping, NULL-safe `LocateRow()` |
| [editer/MOUSE.C](editer/MOUSE.C) | Fixed dead modal click zones & hover bevel flicker, excluded scrollbar from viewport clicks, cleared `state.key` on menu clicks |
| [editer/SUBMENUS.C](editer/SUBMENUS.C) | Shared `DrawSubmenuBox()`, `editActive` bounds check, normalized `#include` paths |
| [editer/SPELL.C](editer/SPELL.C) | Bounded `GetCode()`, `ESC` cancel in IME, bounds-checked page/slot indexing in `OutputHanZi()`, NULL-safe `Gethanzi()` |
| [editer/HANZI.C](editer/HANZI.C) | Persistent `HZK16` handle (`open_hz()`/`close_hz()`), relative path fallback, unsigned GB2312 offset validation, bit-shift glyph rendering |
| [editer/LANGUAGE.C](editer/LANGUAGE.C) | Fixed `current == NULL` crash in `GetEnglish()` and `GetEnter()` |
| [editer/INNAME.C](editer/INNAME.C) | Fixed `spell.name` buffer overflow, `BACKSPACE` off-by-one, added `ESC` cancel |
| [editer/OPEN.C](editer/OPEN.C) | Replaced `feof()` loop with `fgetc() != EOF`, handled `\r` and `\t`, `SAVE/` fallback, `strncpy` to `file.fname`, cache resets in `New()` |
| [editer/SAVE.C](editer/SAVE.C) | Fixed premature `fclose()` inside write loop, added `fopen == NULL` check and `SAVE/` fallback, safe `Saveas()` filename copy |
| [editer/MALLOC.C](editer/MALLOC.C) | Close `HZK16` and exit with code `1` on OOM |
| `h/*.H` (15 headers) | Dedicated `fname[16]`/`name[16]`, `findStart`/`findLast`, idle cache fields, forward declarations, complete prototypes & doc-blocks |
| [README.md](README.md) | Minor formatting cleanup |
