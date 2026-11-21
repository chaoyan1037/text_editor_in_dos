/*****************************************************************************
 * File: SUBMENUS.H
 * Purpose: Pull-down submenu rendering, keyboard navigation, and Help popups.
 *****************************************************************************/
#ifndef _SUBMENUS_H_
#define _SUBMENUS_H_

struct EDITTYPE;

void Submenu(struct EDITTYPE *pEdit);
void GetSubmenu(struct EDITTYPE *pEdit);
void ExitSubmenu(struct EDITTYPE *pEdit);

void Explanation(struct EDITTYPE *pEdit);
void OperationDirection(struct EDITTYPE *pEdit);

#endif
