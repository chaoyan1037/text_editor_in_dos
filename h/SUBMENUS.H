/*****************************************************************************
 * File: SUBMENUS.H
 * Purpose: Pull-down submenu rendering, keyboard navigation, command queuing,
 *          and Help popup dialogs (Operation Guide & About/Credits).
 *****************************************************************************/
#ifndef _SUBMENUS_H_
#define _SUBMENUS_H_

struct EDITTYPE;

/* Render active pull-down menu and process UP/DOWN/ENTER/ESC keys */
void Submenu(struct EDITTYPE *pEdit);

/* Activate pull-down menu F1..F4 based on state.key (or mouse click) */
void GetSubmenu(struct EDITTYPE *pEdit);

/* Close open pull-down menu, repaint its top button, and redraw viewport */
void ExitSubmenu(struct EDITTYPE *pEdit);

/* Display "制作人员" credits popup (dismissed by any key or click) */
void Explanation(struct EDITTYPE *pEdit);

/* Display "操作指南" usage popup (dismissed by any key or click) */
void OperationDirection(struct EDITTYPE *pEdit);

#endif
