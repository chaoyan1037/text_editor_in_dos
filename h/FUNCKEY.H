/*****************************************************************************
 * File: FUNCKEY.H
 * Purpose: Editor command handlers: cursor motion, selection, clipboard
 *          operations, find/replace dialogs, and clean shutdown.
 *****************************************************************************/
#ifndef _FUNCKEY_H_
#define _FUNCKEY_H_

struct EDITTYPE;

void ExitEditer(struct EDITTYPE *pEdit);        /* Free resources, close BGI, exit(0) */
void MoveCursor(struct EDITTYPE *pEdit);        /* Handle UP/DOWN/LEFT/RIGHT and scroll */
void BlackWhiteChoosen(struct EDITTYPE *pEdit); /* Extend selection via Ctrl+Left/Right */
void Paste(struct EDITTYPE *pEdit);             /* Insert copies of clipboard at caret */
void BackSpace(struct EDITTYPE *pEdit);         /* Delete single node before caret */
void CopyCut(struct EDITTYPE *pEdit);           /* Cut selection to detached clipboard */
void FindFunction(struct EDITTYPE *pEdit);      /* Show Find dialog and read query */
void FingGoal(struct EDITTYPE *pEdit);          /* Execute Find() and update dialog UI */
void CancleFind(struct EDITTYPE *pEdit);        /* Dismiss Find dialog and clear state */
void ReplaceFunction(struct EDITTYPE *pEdit);   /* Replace matched range with replaceGoal */
void FindAndReplace(struct EDITTYPE *pEdit);    /* Show Find & Replace dialog */
void CancleReplace(struct EDITTYPE *pEdit);     /* Dismiss Replace dialog and clear state */
void AllChosen(struct EDITTYPE *pEdit);         /* Select all visible nodes on current page */
void MouseBWChosen(struct EDITTYPE *pEdit);     /* Detect mouse hold for drag selection */

#endif
