/*****************************************************************************
 * File: LANGUAGE.H
 * Purpose: ASCII character insertion, newline insertion, and EN/ZH toggle.
 *****************************************************************************/
#ifndef _LANGUAGE_H_
#define _LANGUAGE_H_

struct EDITTYPE;

void GetEnglish(struct EDITTYPE *pEdit);       /* Insert printable ASCII node */
void GetEnter(struct EDITTYPE *pEdit);         /* Insert newline (flag=0) node */
void ExchangeLanguage(struct EDITTYPE *pEdit); /* Toggle language between 0 and 1 */

#endif
