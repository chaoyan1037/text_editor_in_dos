/*****************************************************************************
 * File: SPELL.H
 * Purpose: Chinese character (GB18030/GB2312) node insertion, bottom-left
 *          "中英" indicator, and custom '垚' glyph helper.
 *****************************************************************************/
#ifndef _SPELL_H_
#define _SPELL_H_

struct EDITTYPE;

/* Insert GB18030/GB2312 pair in pEdit->spell.pcode as a flag=2 document node */
void Gethanzi(struct EDITTYPE *pEdit);

/* Render custom glyph '垚' at (x0, y0) */
void OutputYao(int x0, int y0, int color);

/* Render bottom-left "中英" input indicator */
void ShowLanguage(struct EDITTYPE *pEdit);

#endif
