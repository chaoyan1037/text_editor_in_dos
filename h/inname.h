/*****************************************************************************
 * File: INNAME.H
 * Purpose: Modal filename entry and SAVE/ directory file-browser dialogs
 *          used by Open (Ctrl+O) and Save As (Ctrl+D).
 *****************************************************************************/
#ifndef _INNAME_H_
#define _INNAME_H_

struct EDITTYPE;

/* Prompt for a filename to Save As (lists existing files in SAVE/) */
void InName(struct EDITTYPE *pEdit);

/* Browse existing files in SAVE/ (UP/DOWN/mouse) or type a filename to Open */
void InNameOpen(struct EDITTYPE *pEdit);

/* Redraw the green filename input box with the current pEdit->spell.name */
void Frame1(int x1, int y1, int x2, int y2, struct EDITTYPE *pEdit);

#endif
