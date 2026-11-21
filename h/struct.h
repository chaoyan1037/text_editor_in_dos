/*****************************************************************************
 * File: STRUCT.H
 * Purpose: Core data structures and state invariants shared by all modules.
 *
 * Document Linked-List Representation:
 *   The entire text buffer is stored as a doubly-linked list of `struct NODE`:
 *
 *     NULL <- [start] <-> ... <-> [pageFirst] <-> ... <-> [current] <-> ... <-> [last] -> NULL
 *               ^                     ^                       ^
 *       document head         first visible node      node immediately LEFT
 *                             on current page         of the blinking caret
 *
 * Caret Anchor Invariant (`current`):
 *   - `current != NULL`: caret sits immediately AFTER `current` (so inserting
 *     adds after `current`, and BackSpace deletes `current`).
 *   - `current == NULL`: caret sits immediately BEFORE `pageFirst` (top-left
 *     of the currently displayed page, even if scrolled down where
 *     `pageFirst != start`).
 *
 * Clipboard Ownership Invariant (`copyCutFlag`):
 *   - `copyCutFlag == 0`: `clipStart..clipLast` is either NULL or a LIVE
 *     selection range whose nodes STILL belong to `start..last`. Never call
 *     ClearClip() while `copyCutFlag == 0`.
 *   - `copyCutFlag == 1`: `clipStart..clipLast` is a DETACHED heap-allocated
 *     list created by CopySelection() (deep copy) or CopyCut() (unlinked span).
 *     Its endpoints have `clipStart->last == NULL` and `clipLast->next == NULL`,
 *     and it MUST be freed via ClearClip() before starting a new selection or
 *     on shutdown.
 *
 * Find/Replace Isolation (`findStart`, `findLast`):
 *   - Find() records the matched document span in `findStart..findLast` rather
 *     than reusing `clipStart..clipLast`, so searching never clobbers or leaks
 *     the user's clipboard.
 *****************************************************************************/
#ifndef _STRUCT_H_
#define _STRUCT_H_

#include<stdio.h>

/*
 * struct MOUSESTATE - Unified hardware input and command-dispatch state.
 * Holds the latest polled DOS mouse coordinates/button bits, the latest BIOS
 * keystroke, the pending command ID (`function`), and the active modal state
 * (`functionActive`).
 */
struct MOUSESTATE
{
    int x;              /* Mouse cursor X position in pixels (2..X_MAX-1) */
    int y;              /* Mouse cursor Y position in pixels (2..Y_MAX-1) */
    int mouseFlag;      /* 1 = XOR mouse pointer is currently visible on screen;
                         * 0 = screen was repainted over pointer, draw fresh */
    int button;         /* INT 33h/AX=3 button bitmask (bit 0 = left button) */
    int function;       /* One-shot command ID (1..25) consumed by SwitchFunction()
                         * in main() and reset to 0 immediately afterward */
    int functionActive; /* Persistent modal UI state (0 = normal editing):
                         *   0  = Normal text editing
                         *   1  = Dismissible alert/info popup (any key/click)
                         *   2  = Exit: "Save?" confirmation (Y=save&exit, N=exit)
                         *   5  = Find dialog ready (F=find next, C=cancel)
                         *   6  = Replace dialog ready (F=find next, C=cancel)
                         *   7  = Replace match highlighted (R=replace, C=cancel)
                         *   9  = Save confirmation (Y=save, N=cancel)
                         *   10 = Open filename entered (Y=proceed, N=cancel)
                         *   11 = Open: "Save current first?" (Y=save&open, N=open)
                         *   12 = New: "Save current first?" (Y=save&new, N=new)
                         *   13 = Select-All active (Ctrl+C, Ctrl+X, BackSpace)
                         *   15 = Save As filename entered (Y=saveas, N=cancel) */
    long int key;       /* Full 16-bit BIOS key word from bioskey(0) */
    int ascii;          /* Low 8-bit ASCII code (`key & 0x00ff`) */
};

/*
 * struct FILETYPE - Active document filename and deferred save-prompt flags.
 */
struct FILETYPE
{
    int fileflag;    /* 1 = redraw title-bar filename in ViewAll(), then reset to 0 */
    char fname[32];  /* Dedicated buffer for active filename (saved under SAVE/) */
    int modifyFlag;  /* Deferred modal promotion applied at top of main() loop:
                      *   0 = none
                      *   1 = promote to functionActive=11 (Save before Open)
                      *   2 = promote to functionActive=12 (Save before New) */
};

/*
 * struct NODE - Atomic element of the doubly-linked document list.
 * Each node stores either a newline, one ASCII character, or one 2-byte
 * GB2312 Chinese character.
 */
struct NODE
{
    int flag;          /* Visual width & payload type:
                        *   0 = newline / ENTER (advances to next visual row)
                        *   1 = single-byte printable ASCII (width = 8 px)
                        *   2 = 2-byte GB2312 Hanzi or custom '垚' (width = 16 px) */
    int firstInLine;   /* 1 if this node begins a visual row (recomputed by ViewAll),
                        * 0 for all interior nodes on a row */
    char data[3];      /* NUL-terminated character bytes:
                        *   flag=1 -> data[0]=ascii, data[1]='\0'
                        *   flag=2 -> data[0]=b0, data[1]=b1, data[2]='\0'
                        *             (special ' ', ' ' renders custom glyph '垚') */
    struct NODE *last; /* Previous node in document (NULL at `start`) */
    struct NODE *next; /* Next node in document (NULL at `last`) */
};

/*
 * struct SPELLTYPE - Pinyin IME buffers and temporary filename prompt buffer.
 */
struct SPELLTYPE
{
    char name[32]; /* Temporary filename typed in InName() (up to 30 chars + NUL) */
    char pcode[3]; /* 2-byte GB2312 code queued for Gethanzi() insertion */
};

/*
 * struct MENUTYPE - State for the four F1..F4 pull-down menus.
 */
struct MENUTYPE
{
    int menuflag;      /* 0 = dropdown needs repainting; 1 = up to date on screen */
    int flag;          /* 1-based highlighted item index inside active dropdown */
    int SubmenuNum[4]; /* Item counts per top-level menu: F1=5, F2=4, F3=2, F4=2 */
};

/*
 * struct EDITTYPE - Master editor context owned by main() and passed by
 *                   pointer to every module (eliminates mutable globals).
 */
struct EDITTYPE
{
    /* Document, selection/clipboard, find-match, and viewport anchors */
    struct NODE *start;     /* Head of document list (NULL when empty) */
    struct NODE *current;   /* Node left of caret (NULL = before pageFirst) */
    struct NODE *last;      /* Tail of document list (NULL when empty) */
    struct NODE *clipStart; /* First node of selection or detached clipboard */
    struct NODE *clipLast;  /* Last node of selection or detached clipboard */
    struct NODE *findStart; /* First node of latest Find() match (non-owning) */
    struct NODE *findLast;  /* Last node of latest Find() match (non-owning) */
    struct NODE *pageFirst; /* First node drawn at top-left of viewport */

    /* Layout and status-bar display state */
    int lineCount; /* Count of visual line breaks from pageFirst to end of doc */
    int language;  /* 0 = English (ASCII) input; 1 = Chinese (Pinyin IME) input */
    int x;         /* Caret pixel X coordinate (EDITERSCREENX1 .. maxX) */
    int y;         /* Caret baseline pixel Y (EDITERSCREENY1+16 .. EDITERSCREENY2) */
    int line;      /* Last rendered 1-based absolute line number on status bar */
    int row;       /* Last rendered 1-based column number on status bar */

    /* Idle-frame change-detection caches for RollBar() and ShowPosition():
     * Avoid walking the linked list on every main-loop iteration when idle. */
    int tempLineCount;              /* Cached lineCount for RollBar() */
    int tempCurrentCount;           /* Cached scroll line count (0 forces draw) */
    struct NODE *lastPosCurrent;    /* Cached `current` for ShowPosition() */
    struct NODE *lastPosPageFirst;  /* Cached `pageFirst` for ShowPosition() */
    int lastPosX;                   /* Cached caret X (-1 forces initial draw) */
    int lastPosY;                   /* Cached caret Y (-1 forces initial draw) */
    struct NODE *lastRollPageFirst; /* Cached `pageFirst` for RollBar() */

    /* Interaction state and dirty/control flags */
    struct MOUSESTATE state;
    int cursorFlag;   /* 1 = recalculate (x, y) from `current` in DrawCursor() */
    int editFlag;     /* 1 = repaint entire text viewport in ViewAll() */
    int editActive;   /* 0 = editing/modal mode; 1..4 = pull-down F1..F4 open */
    int copyActive;   /* 1 = detached clipboard is populated and ready for Paste */
    int copyCutFlag;  /* 1 = clipStart..clipLast owns detached heap nodes */
    int clipFlag;     /* 1 = clear live selection pointers on next selection */
    int pageDownFlag; /* 1 or 2 = inject a synthetic DOWN step after scroll-down */
    char findGoal[14];    /* Search target string (max 13 chars + NUL) */
    char replaceGoal[14]; /* Replacement string (max 12 chars + NUL) */

    struct SPELLTYPE spell;
    struct MENUTYPE submenu;
    struct FILETYPE file;
};

#endif
