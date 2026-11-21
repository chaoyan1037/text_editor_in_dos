/*****************************************************************************
 * File: OPEN.H
 * Purpose: File loading (ASCII + GB2312) and new blank document creation.
 *****************************************************************************/
#ifndef _OPEN_H_
#define _OPEN_H_

struct EDITTYPE;

/* Load file named pEdit->spell.name from SAVE/ into the document list */
void Open(struct EDITTYPE *pEdit);

/* Clear current document and reset state to blank "NONAME.TXT" */
void New(struct EDITTYPE *pEdit);

#endif
