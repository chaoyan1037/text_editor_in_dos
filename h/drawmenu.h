/*****************************************************************************
 * File: DRAWMENU.H
 * Purpose: Public interface for window shell initialization, top menu buttons,
 *          XOR caret blinking, proportional scrollbar, and status bar readout.
 *****************************************************************************/
#ifndef _DRAWMENU_H_
#define _DRAWMENU_H_

struct EDITTYPE;

/* Enter VGA 640x480 BGI mode and paint title bar, menus, viewport, and IME toggle */
void DrawMainMenu(struct EDITTYPE *pEdit);

/* Repaint individual top-level menu buttons (used on startup and submenu close) */
void DrawMenu1(struct EDITTYPE *pEdit); /* File  <F1> (X=4..70)   */
void DrawMenu2(struct EDITTYPE *pEdit); /* Edit  <F2> (X=94..160) */
void DrawMenu3(struct EDITTYPE *pEdit); /* Tools <F3> (X=184..250)*/
void DrawMenu4(struct EDITTYPE *pEdit); /* Help  <F4> (X=274..340)*/

/* Recompute caret (x,y) when cursorFlag==1 and XOR-toggle 8px underline caret */
void DrawCursor(struct EDITTYPE *pEdit);

/* Render proportional right-hand scrollbar (cached to skip idle frames) */
void RollBar(struct EDITTYPE *pEdit);

/* Render 1-based line/column coordinates on bottom-right bar (cached on idle) */
void ShowPosition(struct EDITTYPE *pEdit);

#endif
