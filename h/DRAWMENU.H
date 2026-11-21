/*****************************************************************************
 * File: DRAWMENU.H
 * Purpose: Main window chrome, menu bar buttons, XOR caret, scrollbar, and
 *          line/column status indicator rendering interfaces.
 *****************************************************************************/
#ifndef _DRAWMENU_H_
#define _DRAWMENU_H_

struct EDITTYPE;

void DrawMainMenu(struct EDITTYPE *pEdit);  /* Initialize VGA BGI and draw shell */
void DrawMenu1(struct EDITTYPE *pEdit);     /* Draw File (<F1>) menu button */
void DrawMenu2(struct EDITTYPE *pEdit);     /* Draw Edit (<F2>) menu button */
void DrawMenu3(struct EDITTYPE *pEdit);     /* Draw Tools (<F3>) menu button */
void DrawMenu4(struct EDITTYPE *pEdit);     /* Draw Help (<F4>) menu button */
void DrawCursor(struct EDITTYPE *pEdit);    /* Toggle XOR underline caret */
void RollBar(struct EDITTYPE *pEdit);       /* Render proportional right scrollbar */
void ShowPosition(struct EDITTYPE *pEdit);  /* Display 1-based line/column status */

#endif
