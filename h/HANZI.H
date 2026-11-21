/*****************************************************************************
 * File: HANZI.H
 * Purpose: GB2312 16x16 bitmap font (HZK16) file management and pixel
 *          rendering routines for Chinese characters in BGI graphics mode.
 *****************************************************************************/
#ifndef _HANZI_H_
#define _HANZI_H_

struct EDITTYPE;

void open_hz(struct EDITTYPE *pEdit);  /* Open persistent HZK16 font handle */
void close_hz(struct EDITTYPE *pEdit); /* Close HZK16 handle on shutdown */
void Get_hz(char incode[], char bytes[], struct EDITTYPE *pEdit);
void Displayhz(int x0, int y0, char code[], int color, struct EDITTYPE *pEdit);
void Printfhz(int x, int y, char *s, int color, struct EDITTYPE *pEdit);

#endif
