/*****************************************************************************
 * File: HANZI.H
 * Purpose: GB2312 Chinese character rendering interface (delegates to the
 *          native UTF-8 terminal renderer in platform/bgi_tui.c).
 *****************************************************************************/
#ifndef _HANZI_H_
#define _HANZI_H_

struct EDITTYPE;

/* Render one GB2312 Hanzi pair at (x0, y0) in `color` */
void Displayhz(int x0, int y0, char code[], int color, struct EDITTYPE *pEdit);

/* Render a NUL-terminated GB2312 Chinese string horizontally from (x, y) */
void Printfhz(int x, int y, char *s, int color, struct EDITTYPE *pEdit);

#endif
