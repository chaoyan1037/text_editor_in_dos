#ifndef _SPELL_H_
#define _SPELL_H_

struct EDITTYPE;

void Spell(struct EDITTYPE *pEdit);
void Gethanzi(struct EDITTYPE *pEdit);
void OutputYao(int x0, int y0, int color);
void ShowLanguage(struct EDITTYPE *pEdit);

#endif
