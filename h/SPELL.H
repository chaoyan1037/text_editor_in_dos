/*****************************************************************************
 * File: SPELL.H
 * Purpose: Pinyin-to-Hanzi input method editor (IME), candidate pagination,
 *          input-mode status indicator, and custom bitmap for 'àê'.
 *****************************************************************************/
#ifndef _SPELL_H_
#define _SPELL_H_

struct EDITTYPE;

void Spell(struct EDITTYPE *pEdit);        /* Read pinyin and pick a Hanzi */
void Gethanzi(struct EDITTYPE *pEdit);     /* Insert pEdit->spell.pcode node */
void OutputYao(int x0, int y0, int color); /* Draw hardcoded 16x16 bitmap 'àê' */
void ShowLanguage(struct EDITTYPE *pEdit); /* Draw bottom-left EN/ZH toggle box */

#endif
