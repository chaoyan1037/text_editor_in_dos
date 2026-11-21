/*****************************************************************************
 * File: COMMON.H
 * Purpose: Screen geometry constants, character grid metrics, and BIOS
 *          16-bit keyboard scan-code / ASCII key definitions (bioskey(0)).
 *
 * Notes:
 *   - Target graphics mode is VGA 640x480 (X_MAX=639, Y_MAX=479).
 *   - BIOS key values returned by bioskey(0) pack the hardware scan code in
 *     the high byte (bits 8..15) and the ASCII code in the low byte (0..7).
 *****************************************************************************/
#ifndef _COMMON_H_
#define _COMMON_H_

/* VGA 640x480 screen bounds (0-indexed pixel coordinates) */
#define X_MAX 639
#define Y_MAX 479

/* Client rectangle of the text editing viewport (right 20px is scrollbar) */
#define EDITERSCREENX1 5
#define EDITERSCREENY1 50
#define EDITERSCREENX2 634
#define EDITERSCREENY2 450

/* Character grid dimensions:
 *   - Up to 75 ASCII columns per line (600px) and 25 visible rows (400px).
 *   - Each ASCII glyph occupies 8x16 px; each GB2312 Hanzi occupies 16x16 px. */
#define CHARINLINE   75
#define CHARINROW    25
#define CHARSIZE     8
#define CHARHIGH     16

/* Control / extended shortcut keys (BIOS scan code << 8 | ASCII/0) */
#define CTRL_N 0x310e /* New file */
#define CTRL_O 0x180f /* Open file */
#define CTRL_D 0x2004 /* Save As */
#define CTRL_S 0x1f13 /* Save file */
#define CTRL_E 0x1205 /* Reserved */
#define CTRL_F 0x2106 /* Find */
#define CTRL_R 0x1312 /* Find & Replace */
#define CTRL_Q 0x1011 /* Reserved */
#define CTRL_C 0x2e03 /* Copy selection */
#define CTRL_X 0x2d18 /* Cut selection */
#define CTRL_V 0x2f16 /* Paste clipboard */
#define CTRL_A 0x1e01 /* Select all on current page */

/* Ctrl + Left/Right: extend text selection one node left or right */
#define CTRL_LEFT  0x7300
#define CTRL_RIGHT 0x7400

/* Function keys: F1..F4 open top-level menus, F8 toggles EN/ZH input mode */
#define F1  0x3b00
#define F2  0x3c00
#define F3  0x3d00
#define F4  0x3e00
#define F5  0x3f00
#define F6  0x4000
#define F7  0x4100
#define F8  0x4200

/* Navigation keys (low byte is 0x00 for extended BIOS keys) */
#define PAGEDOWN   0x5100
#define UP         0x4800
#define DOWN       0x5000
#define LEFT       0x4b00
#define RIGHT      0x4d00

/* Standard keys carrying both scan code and ASCII value */
#define ESC        0x11b  /* Cancel dialog/menu or trigger Exit prompt */
#define BACKSPACE  0x0e08 /* Delete node before cursor or selected range */
#define PAGEUP     0x4900
#define ENTER      0x1c0d /* Insert newline / confirm menu or dialog */
#define SPACE      0x3920 /* Finish pinyin / select candidate #1 */

/* '-' and '=' keys used to page backward/forward in Hanzi candidate bar */
#define JIANHAO    0x0c2d
#define DENGHAO    0x0d3d

#endif
