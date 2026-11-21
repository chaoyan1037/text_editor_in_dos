/*****************************************************************************
 * File: COMMON.H
 * Purpose: Global screen geometry constants, character grid metrics, and
 *          16-bit BIOS keyboard scan-code / ASCII key definitions.
 *
 * VGA 640x480 Screen Coordinate Map (BGI VGAHI mode: X=0..639, Y=0..479):
 *
 *   Y=0   +--------------------------------------------------+--------+
 *         | Title Bar: active filename (left), app title     | Exit   | Y=25
 *   Y=26  +-------+-------+-------+-------+------------------+--------+
 *         | File  | Edit  | Tools | Help  | (light-gray menu strip)   | Y=49
 *   Y=50  +-------+-------+-------+-------+---------------------+-----+
 *         |                                                     | [^] | Y=64
 *         |  Text Viewport: X=5..614 (75 ASCII cols = 600 px)   |  |  |
 *         |                 Y=50..450 (25 rows x 16 px = 400px) | [ ] |
 *         |                 (EDITERSCREENX1..EDITERSCREENX2-20) |  |  |
 *         |                                                     | [v] | Y=450
 *   Y=451 +------+-------------------+-----------------+--------+-----+
 *         | [EN/ | Pinyin Box        | Candidate Bar   | Line:N Col:M | Y=479
 *         |  ZH] | (100..175)        | (200..400)      | (401..639)   |
 *         +------+-------------------+-----------------+--------------+
 *         X=0    X=5                 X=200             X=401   X=614 X=639
 *
 * BIOS Keyboard Encoding (bioskey(0) / INT 16h AH=00h):
 *   - High byte (bits 15..8): IBM PC keyboard hardware scan code.
 *   - Low byte  (bits  7..0): ASCII character code, or 0x00 for extended keys
 *                             (arrows, F1..F8, PageUp/PageDown, Ctrl+Arrows).
 *****************************************************************************/
#ifndef _COMMON_H_
#define _COMMON_H_

/* Full VGA 640x480 raster bounds (inclusive, 0-based pixel coordinates) */
#define X_MAX 639
#define Y_MAX 479

/* Client rectangle of the text editing area (rightmost 20px = scrollbar) */
#define EDITERSCREENX1 5
#define EDITERSCREENY1 50
#define EDITERSCREENX2 634
#define EDITERSCREENY2 450

/* Character cell metrics inside the text viewport:
 *   - CHARINLINE (75) * CHARSIZE (8)  = 600 px usable text width
 *     (EDITERSCREENX1 + 600 = 605 <= EDITERSCREENX2 - 20 = 614).
 *   - CHARINROW  (25) * CHARHIGH (16) = 400 px usable text height
 *     (EDITERSCREENY1 + 400 = 450 == EDITERSCREENY2).
 *   - Each ASCII character takes 1 column (8x16 px, baseline at row bottom).
 *   - Each GB2312 Hanzi takes 2 columns (16x16 px, top-left at y - CHARHIGH). */
#define CHARINLINE   75
#define CHARINROW    25
#define CHARSIZE     8
#define CHARHIGH     16

/* Ctrl+<key> editor shortcuts (BIOS scan code in high byte, Ctrl code in low) */
#define CTRL_N 0x310e /* Ctrl+N: New document (prompts to save if non-empty) */
#define CTRL_O 0x180f /* Ctrl+O: Open document by filename */
#define CTRL_D 0x2004 /* Ctrl+D: Save As (prompt for new filename) */
#define CTRL_S 0x1f13 /* Ctrl+S: Save current document */
#define CTRL_E 0x1205 /* Ctrl+E: Reserved */
#define CTRL_F 0x2106 /* Ctrl+F: Find substring (ASCII or Hanzi) */
#define CTRL_R 0x1312 /* Ctrl+R: Find and Replace */
#define CTRL_Q 0x1011 /* Ctrl+Q: Reserved */
#define CTRL_C 0x2e03 /* Ctrl+C: Copy highlighted selection to clipboard */
#define CTRL_X 0x2d18 /* Ctrl+X: Cut highlighted selection to clipboard */
#define CTRL_V 0x2f16 /* Ctrl+V: Paste clipboard at caret position */
#define CTRL_A 0x1e01 /* Ctrl+A: Select all visible text on current page */

/* Ctrl+Arrow selection keys (low byte is 0x00 -> extended BIOS scan codes) */
#define CTRL_LEFT  0x7300 /* Extend selection one node to the left */
#define CTRL_RIGHT 0x7400 /* Extend selection one node to the right */

/* Function keys: F1..F4 activate top-bar menus; F8 toggles English/Chinese */
#define F1  0x3b00 /* Open File pull-down menu */
#define F2  0x3c00 /* Open Edit pull-down menu */
#define F3  0x3d00 /* Open Tools pull-down menu */
#define F4  0x3e00 /* Open Help pull-down menu */
#define F5  0x3f00 /* Unused */
#define F6  0x4000 /* Unused */
#define F7  0x4100 /* Unused */
#define F8  0x4200 /* Toggle English (0) <-> Chinese Pinyin (1) input mode */

/* Cursor navigation keys (extended scan codes, low byte = 0x00) */
#define PAGEDOWN   0x5100
#define UP         0x4800 /* Move caret up one line (scrolls up at row 1) */
#define DOWN       0x5000 /* Move caret down one line (scrolls down at row 25) */
#define LEFT       0x4b00 /* Move caret left one node */
#define RIGHT      0x4d00 /* Move caret right one node */

/* Standard keys carrying both hardware scan code and ASCII code */
#define ESC        0x11b  /* Cancel dialog/menu/IME, or prompt Exit in editor */
#define BACKSPACE  0x0e08 /* Delete node before caret, or delete active selection */
#define PAGEUP     0x4900
#define ENTER      0x1c0d /* Insert newline (flag=0), or confirm dialog/menu */
#define SPACE      0x3920 /* Confirm pinyin syllable / pick candidate #1 */

/* '-' and '=' keys for Hanzi candidate bar pagination */
#define JIANHAO    0x0c2d /* '-' key: previous page of 4 Hanzi candidates */
#define DENGHAO    0x0d3d /* '=' key: next page of 4 Hanzi candidates */

#endif
