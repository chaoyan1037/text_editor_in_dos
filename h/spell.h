/*****************************************************************************
 * File: SPELL.H
 * Purpose: Chinese Pinyin IME (composition, dictionary lookup, candidate
 *          pagination), Hanzi node insertion, EN/ZH button, and '垚' glyph.
 *****************************************************************************/
#ifndef _SPELL_H_
#define _SPELL_H_

struct EDITTYPE;

/* Run full pinyin input + candidate selection cycle in Chinese mode */
void Spell(struct EDITTYPE *pEdit);

/* Insert GB2312 pair in pEdit->spell.pcode as a flag=2 document node */
void Gethanzi(struct EDITTYPE *pEdit);

/* Render hardcoded 16x16 monochrome bitmap for custom glyph '垚' */
void OutputYao(int x0, int y0, int color);

/* Render bottom-left EN ("英") / ZH ("汉") input-mode indicator button */
void ShowLanguage(struct EDITTYPE *pEdit);

#endif
