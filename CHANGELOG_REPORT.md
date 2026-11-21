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

#### 1. Use-After-Free & Double-Free in `FreeAllNode()` — [editer/NODE.C](editer/NODE.C)
- **Original Bug (`64a59d0`):**
  ```c
  for( ; pNode!=NULL; pNode=pTemp->next) /* reads pTemp->next AFTER free() */
  {
      pTemp=pNode;
      free(pNode);
  }
  ClearClip(pEdit); /* double-frees clipStart..clipLast if still in main list */
  ```
  - **Root Cause & Impact:** `pTemp` pointed to the same node as `pNode`, so the `for`-loop increment `pNode = pTemp->next` dereferenced freed heap memory on every single iteration. Furthermore, `ClearClip()` was called unconditionally even when `clipStart..clipLast` still pointed into the document list (e.g., after selecting or copying text), freeing those nodes a second time and corrupting the heap.
- **How It Was Fixed:**
  ```c
  while(pNode!=NULL)
  {
      pTemp=pNode->next; /* save next pointer BEFORE freeing */
      free(pNode);
      pNode=pTemp;
  }
  if(pEdit->copyCutFlag==1)
  {
      ClearClip(pEdit); /* only free detached clipboard nodes */
  }
  else
  {
      pEdit->clipStart=NULL;
      pEdit->clipLast=NULL;
  }
  ```
  Also resets `start`, `last`, `current`, `pageFirst`, `findStart`, `findLast`, and `clipFlag` to `NULL`/`0`.

#### 2. Use-After-Free in `ClearClip()` — [editer/NODE.C](editer/NODE.C)
- **Original Bug (`64a59d0`):**
  ```c
  while(pNode!=NULL)
  {
      pTemp=pNode;
      free(pNode);
      pNode=NULL;
      if(pTemp==pEdit->clipLast) break;
      pNode=pTemp->next; /* reads pTemp->next AFTER free(pNode) */
  }
  ```
  - **Root Cause & Impact:** Dereferenced `pTemp->next` immediately after `free(pNode)` for every node in a multi-character clipboard.
- **How It Was Fixed:** Save `pNext = (pNode == pEdit->clipLast) ? NULL : pNode->next;` **before** calling `free(pNode)`, then advance `pNode = pNext;` and clear `clipStart`, `clipLast`, `copyCutFlag`, `copyActive`, and `clipFlag`.

#### 3. Use-After-Free & Dangling Pointers in `DeleteNode()` — [editer/NODE.C](editer/NODE.C)
- **Original Bug (`64a59d0`):**
  ```c
  if(pNode->last==NULL)
  {
      pEdit->start=pNode->next;
      pEdit->start->last=NULL; /* NULL dereference if deleting the only node! */
      ...
  }
  else
  {
      pNode->next->last=pNode->last;
      pNode->last->next=pNode->next;
      free(pNode);
      pNode=NULL;
      pEdit->current=pNodeTemp->last; /* USE-AFTER-FREE: pNodeTemp == pNode! */
  }
  ```
  - **Root Cause & Impact:** Three separate crash bugs in one function:
    1. Deleting an interior node freed `pNode` and then read `pNodeTemp->last` from the just-freed block.
    2. Deleting the sole remaining node in the document set `pEdit->start = NULL` and immediately dereferenced `pEdit->start->last = NULL`.
    3. Deleting a node referenced by `pageFirst`, `clipStart`, `clipLast`, `findStart`, or `findLast` left dangling pointers across the editor.
- **How It Was Fixed:**
  1. Guard `if(pNode == NULL) return;` and cache `pPrev = pNode->last; pNext = pNode->next;` up front before mutating or freeing anything.
  2. Bridge `pPrev <-> pNext` safely with NULL checks for `start` and `last`.
  3. If `pNode == pEdit->pageFirst`, advance `pageFirst` to `pNext` (marking `firstInLine = 1`), or walk back from `pPrev` to the preceding line's `firstInLine` if deleting the tail of a scrolled page, or set `NULL` if empty.
  4. Null out `clipStart`/`clipLast` (when `copyCutFlag == 0`) and `findStart`/`findLast` if they matched `pNode`, update `current` using cached `pPrev`, and only then call `free(pNode)`.

#### 4. Live-Document Clipboard Aliasing (`Ctrl+C`) -> Detached Deep Copy — [editer/NODE.C](editer/NODE.C), [editer/EDITER2.C](editer/EDITER2.C)
- **Original Bug (`64a59d0`):** Pressing `Ctrl+C` merely set `copyActive = 1` while leaving `clipStart..clipLast` pointing directly into the live document linked list.
  - **Root Cause & Impact:**
    - Pasting (`Ctrl+V`) while the caret sat inside or next to the copied range inserted new nodes between `clipStart` and `clipLast`, causing an **infinite self-paste loop** until OOM.
    - Editing or deleting the copied text silently mutated or freed the clipboard, causing **use-after-free** on subsequent `Paste()`.
- **How It Was Fixed:** Implemented `CopySelection()` in [editer/NODE.C](editer/NODE.C) and wired `SwitchFunction()` case `12` (`Ctrl+C`) to it. `CopySelection()` allocates a detached deep copy of every node in `[clipStart..clipLast]` with `newStart->last = NULL` and `newLast->next = NULL`, sets `copyCutFlag = 1` and `copyActive = 1`, and establishes a strict ownership invariant (`copyCutFlag == 0` = live non-owning selection; `copyCutFlag == 1` = detached heap-owned clipboard).

#### 5. Use-After-Free & Clipboard Clobbering in `ReplaceFunction()` — [editer/FUNCKEY.C](editer/FUNCKEY.C), [h/STRUCT.H](h/STRUCT.H)
- **Original Bug (`64a59d0`):**
  ```c
  while(pNode!=pEdit->clipLast)
  {
      DeleteNode(pNode, pEdit); /* frees pNode */
      pNode=pNode->next;        /* USE-AFTER-FREE! */
  }
  ...
  pEdit->clipLast=NULL;         /* clobbers/leaks user's clipboard! */
  pEdit->clipStart=NULL;
  ```
  - **Root Cause & Impact:**
    1. Replacing a matched word with a shorter or empty string freed `pNode` via `DeleteNode()` and immediately read `pNode->next` from freed memory.
    2. `Find()` and `ReplaceFunction()` reused `clipStart`/`clipLast`, overwriting or leaking the user's detached clipboard whenever Find/Replace ran.
- **How It Was Fixed:**
  - Added dedicated non-owning `findStart` / `findLast` fields to `struct EDITTYPE` in [h/STRUCT.H](h/STRUCT.H).
  - Rewrote `ReplaceFunction()` in two explicit phases:
    - **Phase 1:** Overwrite existing matched nodes in-place for the shared prefix.
    - **Phase 2a:** If replacement is longer, append remaining characters via `GetEnglish()`.
    - **Phase 2b:** If replacement is shorter/empty, save `pNext = pNode->next` and `isLast = (pNode == pEnd)` **before** calling `DeleteNode(pNode, pEdit)`.

#### 6. Uninitialized Struct Free on `InitMouse()` Failure — [editer/EDITER2.C](editer/EDITER2.C)
- **Original Bug (`64a59d0`):**
  ```c
  if(!InitMouse())
  {
      closegraph();
      printf("InitMouse failed...\n");
      getch();
      ExitEditer(pEdit); /* calls FreeAllNode() BEFORE start/clipStart are set to NULL! */
  }
  pEdit->start=NULL;
  ```
  - **Root Cause & Impact:** `struct EDITTYPE edit` is stack-allocated in `main()`. If DOS mouse detection failed, `ExitEditer()` called `FreeAllNode()` on random uninitialized stack garbage pointers (instant crash/heap corruption) and called `closegraph()` before `initgraph()` had even run.
- **How It Was Fixed:** Added `memset(pEdit, 0, sizeof(struct EDITTYPE))` as the first line of `InitStruct()`, initialized all struct fields first, moved the `InitMouse()` check to the end of `InitStruct()`, and terminated cleanly with `exit(1)` without calling `closegraph()` or `ExitEditer()`.

#### 7. Shutdown Double-Free, Handle Leak & Missing Exit — [editer/FUNCKEY.C](editer/FUNCKEY.C), [editer/MALLOC.C](editer/MALLOC.C)
- **Original Bug (`64a59d0`):** `ExitEditer()` called `FreeAllNode()` followed by `ClearClip()`, never closed `pEdit->spell.phz` (`HZK16`), and returned to `main()`'s infinite `while(1)` loop instead of exiting; `MallocNode()` called `exit(0)` on OOM without closing `HZK16`.
- **How It Was Fixed:** `ExitEditer()` calls `closegraph()`, `FreeAllNode(pEdit)`, `close_hz(pEdit)`, and `exit(0)`. `MallocNode()` calls `close_hz(pEdit)` and `exit(1)` on allocation failure.

---

### 1.2 NULL Dereferences, Crashes & Infinite Loops

#### 8. NULL Dereference When Typing at Top-Left of Page (`current == NULL`) — [editer/LANGUAGE.C](editer/LANGUAGE.C), [editer/SPELL.C](editer/SPELL.C)
- **Original Bug (`64a59d0`):**
  ```c
  /* In GetEnglish(), GetEnter(), and Gethanzi() */
  else if( (pEdit->start!=NULL) && (pEdit->current->next==NULL) )
  ```
  - **Root Cause & Impact:** By editor convention, placing the caret at column 1 of row 1 sets `pEdit->current = NULL` (meaning "before `pageFirst`"). Whenever the document was non-empty (`start != NULL`) and the user moved left to top-left and typed any ASCII char, Enter, or Hanzi, `pEdit->current->next` dereferenced `NULL`.
- **How It Was Fixed:** Changed the branch condition in all three functions (`GetEnglish()`, `GetEnter()`, `Gethanzi()`) to:
  ```c
  else if( pEdit->current!=NULL && pEdit->current->next==NULL )
  ```
  so `current == NULL` falls through cleanly to `InsertNode()`.

#### 9. NULL Dereferences & Broken Scrolled Links in `AddNode()` / `InsertNode()` — [editer/NODE.C](editer/NODE.C)
- **Original Bug (`64a59d0`):**
  - `AddNode()` unconditionally executed `pEdit->current->next = pNode` (crashed when `current == NULL` or list empty) and overwrote `pEdit->last = pNode` even when inserting in the middle.
  - `InsertNode()` executed `pEdit->current->next->last = pNode` without checking `current->next != NULL`, executed `pEdit->pageFirst->last = pNode` without checking `pageFirst != NULL`, and when `start != pageFirst` (scrolled down) overwrote `pNode->last = NULL`, **severing all off-screen lines above `pageFirst`** and leaking them.
- **How It Was Fixed:** Handled all list positions cleanly in both functions: empty list (`start == NULL`), insertion before `pageFirst` (preserving `pPrev = pageFirst->last` and bridging `pPrev->next = pNode` when scrolled!), interior insertion, and tail insertion, plus guaranteed `pNode->data[2] = '\0'`.

#### 10. Infinite Loop & Corrupt Head in `Paste()` — [editer/FUNCKEY.C](editer/FUNCKEY.C)
- **Original Bug (`64a59d0`):**
  ```c
  if(pEdit->current==NULL)
  {
      struct NODE *p=MallocNode(pEdit);
      *p=*(pEdit->clipStart); /* copies raw next/last pointers from clipboard! */
      pEdit->current=pEdit->pageFirst=pEdit->last=p; /* start stays NULL, pNode never advances! */
  }
  ```
  - **Root Cause & Impact:** Pasting into an empty document:
    1. Copied raw `next`/`last` pointers from the clipboard node into the new document node (aliasing the clipboard into the document).
    2. Never set `pEdit->start = p` (leaving `start == NULL`).
    3. Never checked `pNode == clipLast` or advanced `pNode = pNode->next` — on a 1-char clipboard or empty doc, `pNode` stayed stuck on `clipStart` (duplicating the first char or looping forever).
- **How It Was Fixed:** Require `copyActive == 1 && copyCutFlag == 1 && clipStart != NULL`. When `start == NULL`, copy only `data[0..2]` and `flag`, set `firstInLine = 1`, and initialize `start = pageFirst = last = current = p`. Unify loop termination (`if(pNode == pEdit->clipLast) break; pNode = pNode->next;`) at the bottom of the loop for every branch.

#### 11. NULL Dereference & Broken `pageFirst` in `CopyCut()` — [editer/FUNCKEY.C](editer/FUNCKEY.C)
- **Original Bug (`64a59d0`):**
  ```c
  if(pEdit->clipStart==pEdit->pageFirst)
  {
      pEdit->pageFirst=pEdit->clipLast->next;
      pEdit->pageFirst->last=NULL; /* NULL dereference when cutting to end of doc! */
      pEdit->current=NULL;
      if(pEdit->pageFirst==NULL)   /* check comes TOO LATE! */
      {
          pEdit->last=pEdit->current=NULL;
      }
  }
  ```
  - **Root Cause & Impact:** Cutting from `pageFirst` through the end of the document set `pageFirst = NULL` and immediately dereferenced `pageFirst->last = NULL`. Additionally, `start` was never updated when cutting from the beginning, cutting a span that crossed `pageFirst` left `pageFirst` pointing into the detached clipboard, and pressing `Ctrl+X` twice re-cut an already-detached clipboard.
- **How It Was Fixed:** Guard `copyCutFlag == 0`, scan `[clipStart..clipLast]` to detect `pageFirstCut`, stitch `pPrev <-> pNext` with NULL checks, repair `start`, `last`, `pageFirst`, and `current`, sever outer links on the detached clip (`clipStart->last = NULL`, `clipLast->next = NULL`), and set `copyCutFlag = 1`.

#### 12. NULL Dereference & Hanzi Width Miscalculation in `AllChosen()` — [editer/FUNCKEY.C](editer/FUNCKEY.C)
- **Original Bug (`64a59d0`):** Did not check `pEdit->pageFirst == NULL` (crashing on `Ctrl+A` in an empty document), did not free any prior detached clipboard (`ClearClip()`), and tested `count == CHARINLINE` instead of `>=` while incrementing by `1` per node (missing line wraps when double-width `flag == 2` Hanzi appeared on the last row).
- **How It Was Fixed:** Return immediately if `pageFirst == NULL`, call `ClearClip()` if `copyCutFlag == 1`, accumulate visual columns via `count += (pNode->flag == 0) ? 1 : pNode->flag` with `count >= CHARINLINE`, and guard `pEdit->current == NULL` inside the highlight loop.

#### 13. NULL Dereferences in `MoveCursor()` & `LocateRow()` — [editer/FUNCKEY.C](editer/FUNCKEY.C), [editer/DRAWMENU.C](editer/DRAWMENU.C)
- **Original Bug (`64a59d0`):**
  - `MoveCursor()` on `UP` at row 1 executed `struct NODE *pNode = pEdit->pageFirst->last;` without checking `pageFirst != NULL` (crashing on empty doc), and `DOWN` at row 25 did not guard `pageFirst != NULL`.
  - `LocateRow()` evaluated `pNode->firstInLine == 0` before checking `pNode != NULL`.
- **How It Was Fixed:** Added explicit `pEdit->pageFirst != NULL && pEdit->pageFirst->last != NULL` guards in `MoveCursor()` and changed `LocateRow()` to `for( ; pNode != NULL && pNode->firstInLine == 0; pNode = pNode->last)` with safe `*pRow = 1` fallback.

#### 14. NULL Dereferences & Odd-Length Overread in `HANZI.C` — [editer/HANZI.C](editer/HANZI.C)
- **Original Bug (`64a59d0`):** `Get_hz()` passed `pEdit->spell.phz` directly to `fseek()`/`fread()` even if `HZK16` failed to open (`NULL`); `Printfhz()` only checked `*s != '\0'` and stepped `s += 2`, reading past the NUL terminator on odd-length strings.
- **How It Was Fixed:** Added `phz == NULL` guards and lazy `open_hz()` retry, zeroed `bytes[32]` on short read, guarded `s == NULL`, and changed `Printfhz()`'s loop condition to `(s[0] != '\0') && (s[1] != '\0')`.

---

### 1.3 Buffer Overflows, Integer Overflow & Bounds Bugs

#### 15. Filename Buffer Overflow & Pointer Aliasing — [h/STRUCT.H](h/STRUCT.H), [editer/INNAME.C](editer/INNAME.C)
- **Original Bug (`64a59d0`):**
  - `struct SPELLTYPE` declared `char name[10];`, yet `InName()` allowed `i < 12` and wrote `pEdit->spell.name[i + 1] = '\0'` — writing up to **3 bytes past the end of `name[10]`** directly into `pcode[]` and adjacent struct fields!
  - `struct FILETYPE` declared `char *fname;`, initialized to string literal `"NONAME.TXT"` and later aliased via `pEdit->file.fname = pEdit->spell.name`. Opening or prompting another filename silently overwrote the active document's filename.
  - `InName()` had no `ESC` cancel handling and broke out of the loop as soon as `i >= 12` without letting the user backspace.
- **How It Was Fixed:** Expanded both to dedicated fixed arrays (`char name[16]`, `char fname[16]`), enforced `i < 12` before writing, fixed `BACKSPACE` to decrement `i` before setting `name[i] = '\0'`, added `ESC` cancel (setting `name[0] = '\0'`), and copied filenames with `strcpy`/`strncpy`.

#### 16. Stack Buffer Overflow, Pointer Underflow & Guaranteed NULL Crash in Pinyin IME — [editer/SPELL.C](editer/SPELL.C)
- **Original Bug (`64a59d0`):**
  ```c
  /* 1. GetCode(): unbounded read into caller's stack buffer */
  while( (tcode[i] = fgetc(fp)) != EOF ) { i++; }

  /* 2. OutputHanZi(): writes past end of code[] on partial last page */
  while(i<4) { s+=2; *s=0; i++; }

  /* 3. OutputHanZi(): sets s=NULL and IMMEDIATELY dereferences s[0]! */
  if(*s=='\0') { s=NULL; }
  pEdit->spell.pcode[0]=s[0];
  pEdit->spell.pcode[1]=s[1];
  ```
  - **Root Cause & Impact:**
    1. `GetCode()` could overflow `code[300]` on large pinyin dictionaries and stored `fgetc()`'s `int` return into a `char` before comparing against `EOF`.
    2. Pagination (`=` / `-`) mutated raw pointer `s` with `s += 2` and `s -= 16`, corrupting memory past `code[]` on partial last pages and crashing whenever the user pressed `'1'..'4'` on an empty slot (it literally assigned `s = NULL` and read `s[0]` on the next line!).
    3. Neither `InputPinYin()` nor `OutputHanZi()` supported `ESC` cancellation.
- **How It Was Fixed:**
  - Passed explicit `maxLen` to `GetCode()` (`int ch; while(i < maxLen - 1 && (ch = fgetc(fp)) != EOF)`), validated inputs, and added relative `spell/<pin>.txt` fallback.
  - Replaced all raw pointer arithmetic in `OutputHanZi()` with bounds-checked integer page/slot indexing (`totalChars = len / 2`, `idx = page * 4 + choice`, only committing when `idx < totalChars` or on the special `yao` page 8 slot 3 `'垚'` glyph).
  - Added `ESC` cancel to both `InputPinYin()` (returning `0` to abort cleanly) and `OutputHanZi()`.

#### 17. Signed `char` Underflow in GB2312 `HZK16` Offset Math — [editer/HANZI.C](editer/HANZI.C)
- **Original Bug (`64a59d0`):**
  ```c
  qh = incode[0] - 0xa0;
  wh = incode[1] - 0xa0;
  offset = (94*(qh-1)+(wh-1))*32l;
  ```
  - **Root Cause & Impact:** Any byte `< 0xa1` underflowed `unsigned char` (`qh - 1` became `255`), computing a bogus negative/out-of-range offset for `fseek()`, and `94 * (qh - 1)` was evaluated in 16-bit `int` before multiplying by `32l` on Turbo C.
- **How It Was Fixed:** Cast both bytes to `unsigned char`, validate `if(b0 < 0xa1 || b1 < 0xa1) return;`, and compute `offset = (94UL * (unsigned long)(b0 - 0xa1) + (unsigned long)(b1 - 0xa1)) * 32UL;`.

#### 18. Divide-by-Zero & 16-Bit Overflow in `RollBar()` — [editer/DRAWMENU.C](editer/DRAWMENU.C)
- **Original Bug (`64a59d0`):**
  ```c
  if(y>=EDITERSCREENY2-76)
  {
      static i=0;
      y=EDITERSCREENY2-76+(i/pEdit->tempLineCount); /* DIVIDE BY ZERO when tempLineCount==0! */
      height=height-(i/pEdit->tempLineCount);
      if(height>=10) i++;
  }
  ```
  - **Root Cause & Impact:** Divided by `pEdit->tempLineCount` without guarding `!= 0`, used a monotonically growing `static i` that never reset on New/Open, and allowed the thumb to drift over the scrollbar arrows.
- **How It Was Fixed:** Replaced with true proportional thumb geometry using 32-bit `long` arithmetic (`thumbHeight = trackHeight * CHARINROW / totalLines`, `thumbY = trackTop + linesBefore * (trackHeight - thumbHeight) / maxScroll`), guarding `maxScroll > 0` and clamping both `thumbHeight` (`[14, trackHeight]`) and `thumbY` (`[trackTop, trackBottom - thumbHeight]`).

#### 19. Trapped Dialog Loops & Backspace Off-by-One in Find/Replace — [editer/FUNCKEY.C](editer/FUNCKEY.C)
- **Original Bug (`64a59d0`):** `FindFunction()` and `FindAndReplace()` did not handle `ESC` (trapping the user in the modal loop until pressing `ENTER`), did not clear previous `findGoal`/`replaceGoal` strings on open, and cleared `findGoal[i-1]` before decrementing `i` cleanly.
- **How It Was Fixed:** Reset buffers on entry, added `key == ESC` -> `CancleFind()`/`CancleReplace()` + `break`, and standardized `i--` / `j--` before NUL termination.

---

### 1.4 File I/O Bugs

#### 20. Mid-Loop `fclose(fp)` Infinite Loop & Missing `fopen` Guard in `Save()` — [editer/SAVE.C](editer/SAVE.C)
- **Original Bug (`64a59d0`):**
  ```c
  fp=fopen(routine,"wt"); /* NO NULL check! */
  while(p!=NULL)
  {
      if(p->data[0]==0)
      {
          fprintf(fp,"%c",p->data[0]);
          fclose(fp); /* closes fp mid-loop AND does not advance p=p->next -> INFINITE LOOP! */
      }
      else
      {
          if(p->flag==0) { p->data[0]=10; fprintf(fp,"%c",p->data[0]); }
          ...
          p=p->next;
      }
  }
  fclose(fp);
  ```
  - **Root Cause & Impact:**
    1. If `C://TC30//BIN//SAVE//` did not exist, `fp` was `NULL` and `fprintf(NULL, ...)` crashed immediately.
    2. If any node had `data[0] == 0`, the `if` branch closed `fp` and **never executed `p = p->next`**, creating an infinite loop writing to a closed `FILE*`!
    3. `flag == 0` mutated `p->data[0] = 10` in-place during save.
- **How It Was Fixed:** Validate `filename`, try `C://TC30//BIN//SAVE//` then fall back to relative `SAVE/`, return cleanly if `fp == NULL`, serialize with `fputc('\n', fp)` (`flag == 0`), `fputc(data[0], fp)` (`flag == 1`), and two `fputc` calls (`flag == 2`), always advancing `p = p->next`, and call `fclose(fp)` once at the end.

#### 21. `while(feof(fp) == 0)` Extra-Byte Bug & Missing `\r`/`\t` Handling in `Open()` — [editer/OPEN.C](editer/OPEN.C)
- **Original Bug (`64a59d0`):**
  ```c
  while(feof(fp)==0)
  {
      r=fgetc(fp); /* returns EOF (-1) on final iteration! */
      if((r>=0x20)&&(r<=0x7e)) ...
      else if(r==10) ...
      else { pEdit->spell.pcode[0]=r; r=fgetc(fp); ... } /* misparses \r, \t, and EOF as Hanzi! */
  }
  pEdit->file.fname=pEdit->spell.name; /* aliases shared input buffer! */
  ```
  - **Root Cause & Impact:**
    1. Testing `feof(fp)` before `fgetc()` ran an extra iteration after the last byte, feeding `EOF` (`-1`) into the Hanzi branch.
    2. DOS CRLF (`\r`, `0x0D`) and tabs (`\t`, `0x09`) fell into the `else` Hanzi branch, consuming the following ASCII character as the second byte of a garbage Chinese glyph and corrupting the rest of the line!
    3. On open failure, the error alert did not set `state.functionActive = 1`, so clicking/pressing a key failed to dismiss it cleanly.
    4. `New()` called `Savewarning(pEdit)` and immediately executed `FreeAllNode(pEdit)` anyway without waiting for the user's Y/N answer!
- **How It Was Fixed:**
  - Replaced with `while((r = fgetc(fp)) != EOF)`, explicitly skipped `'\r'`, expanded `'\t'` to 4 spaces, and only paired `r >= 0x80` with `r2 != EOF` for GB2312 Hanzi.
  - Added relative `SAVE/` fallback, set `state.functionActive = 1` on open error, copied `spell.name` into `file.fname` via `strncpy`, and removed the broken `Savewarning()` inside `New()` (handled properly by modal state `12`).

#### 22. Persistent `HZK16` Font Handle Leak — [editer/HANZI.C](editer/HANZI.C)
- **Original Bug (`64a59d0`):** Opened `HZK16` only via hardcoded `C://TC30//BIN//hanziku//HZK16`, reopened it redundantly inside `OutputHanZi()` and closed it prematurely (breaking subsequent `Displayhz()` calls until reopened), and never closed it on shutdown.
- **How It Was Fixed:** Added idempotent `open_hz()` (no-op if `phz != NULL`, with relative `hanziku/HZK16` fallback) and `close_hz()` (called on clean exit and OOM).

---

### 1.5 UI, Mouse & State-Machine Bugs

#### 23. Dead Click Zones Over Viewport & Hover Bevel Flicker in `MouseKey()` — [editer/MOUSE.C](editer/MOUSE.C)
- **Original Bug (`64a59d0`):**
  ```c
  if(x<=306 && x>=246 && y>=214 && y<=232) /* matched EVEN WHEN fa==0 (no dialog open)! */
  {
      if(fa==2)
      {
          button3d(246, 214, 306, 232); /* flashed on hover even when b==0! */
          if(b==1) { ... }
      }
      ...
  }
  else if(x>=5 && x<=634 && y>=50 && y<=450 && b==1 && pEdit->editActive==0)
  ```
  - **Root Cause & Impact:** Because the invisible modal button rectangles (`246..379 x 214..232`) were tested **before** `else if (viewport)` without checking `fa != 0`, clicking on normal text inside that central rectangle of the screen did nothing! Moving the mouse over an open dialog flashed `button3d()` continuously even without clicking, and clicking the scrollbar fell into the viewport text-click branch.
  - In `ChooseSubmenu()`, clicking a top-bar menu set `state.function = 6` (`GetSubmenu()`) without clearing `state.key`, so a stale `F1`–`F4` scan code in `state.key` overrode the clicked menu!
- **How It Was Fixed:** Wrapped all modal dialog hit-testing in `if(fa != 0 && b == 1)` with early `return`, restricted viewport clicks to `fa == 0 && editActive == 0 && x <= EDITERSCREENX2 - 20`, gated scrollbar arrow clicks on `fa == 0 && editActive == 0`, and cleared `pEdit->state.key = 0` before queuing `function = 6` on menu clicks.

#### 24. Stale `firstInLine` Flags After Edits in `ViewAll()` — [editer/VIEW.C](editer/VIEW.C)
- **Original Bug (`64a59d0`):** `ViewAll()` set `pNode->next->firstInLine = 1` at line wraps/newlines, but **never cleared old `firstInLine = 1` flags** on nodes that were no longer at column 0 after an insertion or deletion shifted wrap boundaries.
  - **Root Cause & Impact:** Stale `firstInLine = 1` markers caused `RollBar()` to overcount lines, `LocateRow()` to stop mid-line (reporting wrong column numbers), and `MoveCursor()` (`UP`/`DOWN` scrolling) to jump to mid-line positions.
- **How It Was Fixed:** Explicitly sets `pNode->firstInLine = 1` on `pageFirst` and clears `pNode->next->firstInLine = 0` on every node before evaluating hard newlines or soft wraps (`x >= maxX`) through the entire tail of the document.

#### 25. Out-of-Bounds Menu Array Indexing & Modal Cancel Gaps — [editer/SUBMENUS.C](editer/SUBMENUS.C), [editer/EDITER2.C](editer/EDITER2.C)
- **Original Bug (`64a59d0`):**
  - `Submenu()` indexed `SubmenuNum[pEdit->editActive - 1]` without verifying `editActive` was in `1..4`.
  - Modal confirmation states (`Exit`, `Save`, `Open`, `New`) had no `ESC` cancel branch; pressing an unrelated key after `Ctrl+A` (`functionActive == 13`) left the blue selection highlight stuck on screen.
- **How It Was Fixed:** Added `pEdit->editActive >= 1 && pEdit->editActive <= 4` bounds check in `Submenu()`, added `ESC` cancellation to modal dialogs in `FunctionActiveSwitch()`, and cleared live selection highlight cleanly on any non-copy/cut key in state `13`.

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
