/*****************************************************************************
 * File: LANGUAGE.H
 * Purpose: Printable ASCII insertion, newline insertion, and EN/ZH toggle.
 *****************************************************************************/
#ifndef _LANGUAGE_H_
#define _LANGUAGE_H_

struct EDITTYPE;

/* Insert pEdit->state.ascii as a flag=1 ASCII node at the caret */
void GetEnglish(struct EDITTYPE *pEdit);

/* Insert a flag=0 newline node at the caret (triggers scroll at row 25) */
void GetEnter(struct EDITTYPE *pEdit);

/* Toggle pEdit->language between 0 (English) and 1 (Chinese Pinyin) */
void ExchangeLanguage(struct EDITTYPE *pEdit);

#endif
