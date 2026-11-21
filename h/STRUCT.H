/*****************************************************************************
 * File: STRUCT.H
 * Purpose: Core data structures shared across all editor modules.
 *
 * Architectural Overview:
 *   - Document storage is a doubly-linked list of `struct NODE` elements
 *     headed by `start` and terminated by `last`.
 *   - `pageFirst` marks the first node rendered on the current screen page.
 *   - `current` points to the node immediately BEFORE the caret. A value of
 *     `current == NULL` means the caret sits immediately before `pageFirst`
 *     (top-left of the current page).
 *   - Clipboard ownership is governed by `copyCutFlag`:
 *       * copyCutFlag == 0: `clipStart..clipLast` is a live selection range
 *         pointing to nodes inside `start..last` (do NOT free separately).
 *       * copyCutFlag == 1: `clipStart..clipLast` owns a detached heap list
 *         created by CopySelection() or CopyCut() (must be freed via ClearClip).
 *   - Find/Replace uses separate `findStart..findLast` pointers so searching
 *     never overwrites or leaks the user's clipboard.
 *****************************************************************************/
#ifndef _STRUCT_H_
#define _STRUCT_H_

#include<stdio.h>

/* Input state: mouse coordinates/buttons, pending command, and last key */
struct MOUSESTATE
{
    int x;              /* Current mouse X coordinate (pixels) */
    int y;              /* Current mouse Y coordinate (pixels) */
    int mouseFlag;      /* 1 if XOR mouse arrow is currently drawn on screen */
    int button;         /* Mouse button bitmask from INT 33h/AX=3 (1=left) */
    int function;       /* Queued command ID dispatched by SwitchFunction() */
    int functionActive; /* Active modal dialog state (0 = normal editing):
                         *   1  = Info/warning alert (any key/click dismisses)
                         *   2  = Save-before-Exit confirmation (Y/N)
                         *   5  = Find dialog awaiting F (Find) / C (Cancel)
                         *   6  = Replace dialog awaiting F (Find) / C (Cancel)
                         *   7  = Replace match found, awaiting R / C
                         *   9  = Save confirmation (Y/N)
                         *   10 = Open filename confirmation (Y/N)
                         *   11 = Save-before-Open confirmation (Y/N)
                         *   12 = Save-before-New confirmation (Y/N)
                         *   13 = Select-All active (Ctrl+C/X/Backspace)
                         *   15 = Save-As filename confirmation (Y/N) */
    long int key;       /* Full 16-bit BIOS key code from bioskey(0) */
    int ascii;          /* Low-byte ASCII code extracted from key */
};

/* Active document filename and pending confirmation workflow flags */
struct FILETYPE
{
    int fileflag;    /* Set to 1 when title bar filename needs redrawing */
    char fname[16];  /* Active DOS 8.3 filename buffer (decoupled from spell.name) */
    int modifyFlag;  /* Deferred modal prompt: 1=Save-before-Open, 2=Save-before-New */
};

/* Single character unit in the doubly-linked document list */
struct NODE
{
    int flag;        /* Node kind: 0=newline (ENTER), 1=ASCII char, 2=GB2312 Hanzi */
    int firstInLine; /* 1 if this node starts a visual line on screen, else 0 */
    char data[3];    /* Null-terminated payload: 1 byte (ASCII) or 2 bytes (GB2312) */
    struct NODE *last; /* Previous node in document (NULL for head) */
    struct NODE *next; /* Next node in document (NULL for tail) */
};

/* State for Chinese pinyin input, filename input buffer, and HZK16 font file */
struct SPELLTYPE
{
    char name[16];   /* Temporary filename input buffer (max 12 chars + NUL) */
    char pcode[3];   /* 2-byte GB2312 code of the next Hanzi to insert */
    FILE *phz;       /* Persistent FILE* handle to HZK16 16x16 bitmap font */
};

/* Pull-down menu state */
struct MENUTYPE
{
    int menuflag;      /* 0 = active submenu needs redraw; 1 = already drawn */
    int flag;          /* 1-based index of currently highlighted submenu item */
    int SubmenuNum[4]; /* Item count for menus F1..F4: {5, 4, 2, 2} */
};

/* Master editor context passed to all subsystems */
struct EDITTYPE
{
    /* Document, clipboard, find-match, and viewport linked-list pointers */
    struct NODE *start;     /* First node in the entire document */
    struct NODE *current;   /* Node immediately left of caret (NULL = before pageFirst) */
    struct NODE *last;      /* Last node in the entire document */
    struct NODE *clipStart; /* Start of selection or detached clipboard list */
    struct NODE *clipLast;  /* End of selection or detached clipboard list */
    struct NODE *findStart; /* Start of most recent Find() match in document */
    struct NODE *findLast;  /* End of most recent Find() match in document */
    struct NODE *pageFirst; /* First node displayed at top-left of viewport */

    /* Layout and caret state */
    int lineCount; /* Number of visual line breaks from pageFirst to end of doc */
    int language;  /* Input mode: 0 = English (ASCII), 1 = Chinese (Pinyin) */
    int x;         /* Caret pixel X coordinate */
    int y;         /* Caret baseline pixel Y coordinate */
    int line;      /* Cached 1-based line number shown on status bar */
    int row;       /* Cached 1-based column number shown on status bar */

    /* Scrollbar & status-bar change-detection caches (avoid idle traversals) */
    int tempLineCount;
    int tempCurrentCount;
    struct NODE *lastPosCurrent;
    struct NODE *lastPosPageFirst;
    int lastPosX;
    int lastPosY;
    struct NODE *lastRollPageFirst;

    /* Editor interaction state and flags */
    struct MOUSESTATE state;
    int cursorFlag;   /* 1 = recompute caret (x, y) from current node */
    int editFlag;     /* 1 = clear and redraw editor viewport via ViewAll() */
    int editActive;   /* 0 = editing mode; 1..4 = pull-down menu F1..F4 open */
    int copyActive;   /* 1 = clipboard holds valid content ready for Paste() */
    int copyCutFlag;  /* 1 = clipStart..clipLast owns detached heap memory */
    int clipFlag;     /* 1 = reset selection pointers on next selection start */
    int pageDownFlag; /* 1 or 2 = defer a DOWN cursor step after page scroll */
    char findGoal[14];    /* Search query buffer (up to 13 ASCII chars + NUL) */
    char replaceGoal[14]; /* Replacement string buffer (up to 12 chars + NUL) */

    struct SPELLTYPE spell;
    struct MENUTYPE submenu;
    struct FILETYPE file;
};

#endif
