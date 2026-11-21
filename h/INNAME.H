/*****************************************************************************
 * File: INNAME.H
 * Purpose: Modal DOS 8.3 filename entry dialog used by Open and Save As.
 *****************************************************************************/
#ifndef _INNAME_H_
#define _INNAME_H_

struct EDITTYPE;

/* Prompt user for up to 12 ASCII filename chars into pEdit->spell.name */
void InName(struct EDITTYPE *pEdit);

/* Redraw the green filename input box with the current pEdit->spell.name */
void Frame1(int x1, int y1, int x2, int y2, struct EDITTYPE *pEdit);

#endif
