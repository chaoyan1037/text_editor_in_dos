/*****************************************************************************
 * File: FUNCKEY.H
 * Purpose: Public interface for high-level editor commands: shutdown, caret
 *          navigation, selection, clipboard cut/paste, find/replace, and
 *          mouse drag-selection detection.
 *****************************************************************************/
#ifndef _FUNCKEY_H_
#define _FUNCKEY_H_

struct EDITTYPE;

/* Close BGI graphics, free all memory, close HZK16 handle, and call exit(0) */
void ExitEditer(struct EDITTYPE *pEdit);

/* Move caret LEFT/RIGHT/UP/DOWN and scroll pageFirst at top/bottom bounds */
void MoveCursor(struct EDITTYPE *pEdit);

/* Extend highlighted selection leftward (Ctrl+Left) or rightward (Ctrl+Right) */
void BlackWhiteChoosen(struct EDITTYPE *pEdit);

/* Insert deep copies of detached clipboard nodes at active caret (Ctrl+V) */
void Paste(struct EDITTYPE *pEdit);

/* Delete single node immediately before caret (BackSpace) */
void BackSpace(struct EDITTYPE *pEdit);

/* Detach live selection into clipboard (Ctrl+X) or delete it (BackSpace) */
void CopyCut(struct EDITTYPE *pEdit);

/* Show Find modal dialog (Ctrl+F) and read search query into findGoal */
void FindFunction(struct EDITTYPE *pEdit);

/* Run Find() for active Find/Replace dialog and update dialog status/buttons */
void FingGoal(struct EDITTYPE *pEdit);

/* Clear Find state and redraw viewport to remove highlights */
void CancleFind(struct EDITTYPE *pEdit);

/* Replace matched node span (findStart..findLast) with replaceGoal */
void ReplaceFunction(struct EDITTYPE *pEdit);

/* Show Find & Replace modal dialog (Ctrl+R) and read findGoal + replaceGoal */
void FindAndReplace(struct EDITTYPE *pEdit);

/* Clear Replace state and redraw viewport to remove highlights */
void CancleReplace(struct EDITTYPE *pEdit);

/* Select all visible nodes on the current viewport page (Ctrl+A) */
void AllChosen(struct EDITTYPE *pEdit);

/* Detect ~100ms mouse button hold inside viewport to trigger selection */
void MouseBWChosen(struct EDITTYPE *pEdit);

#endif
