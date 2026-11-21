/*****************************************************************************
 * File: HANZI.H
 * Purpose: Persistent GB2312 16x16 bitmap font (HZK16) file management and
 *          BGI pixel-plotting routines for Chinese characters.
 *****************************************************************************/
#ifndef _HANZI_H_
#define _HANZI_H_

struct EDITTYPE;

/* Lazily open HZK16 into pEdit->spell.phz (no-op if already open) */
void open_hz(struct EDITTYPE *pEdit);

/* Close pEdit->spell.phz and reset pointer to NULL (safe if already NULL) */
void close_hz(struct EDITTYPE *pEdit);

/* Load 32-byte 16x16 glyph bitmap for GB2312 pair incode[0..1] into bytes[] */
void Get_hz(char incode[], char bytes[], struct EDITTYPE *pEdit);

/* Render one 16x16 GB2312 Hanzi at top-left pixel (x0, y0) in `color` */
void Displayhz(int x0, int y0, char code[], int color, struct EDITTYPE *pEdit);

/* Render a NUL-terminated GB2312 Chinese string horizontally from (x, y) */
void Printfhz(int x, int y, char *s, int color, struct EDITTYPE *pEdit);

#endif
