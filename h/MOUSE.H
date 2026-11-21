/*****************************************************************************
 * File: MOUSE.H
 * Purpose: DOS INT 33h mouse driver wrapper, XOR pointer renderer, UI
 *          click hit-testing, and 3D beveled rectangle helpers.
 *****************************************************************************/
#ifndef _MOUSE_H_
#define _MOUSE_H_

#define MOUSEID 0X33 /* Standard DOS mouse driver software interrupt (INT 33h) */

struct EDITTYPE;

/* Reset DOS mouse driver and clamp movement bounds; returns 0 on failure */
int InitMouse(void);

/* Poll INT 33h/AX=3 to refresh state.x, state.y, and state.button */
void ReadMouse(struct EDITTYPE *pEdit);

/* Handle clicks on modal dialogs, viewport, title/status buttons, and menus */
void MouseKey(struct EDITTYPE *pEdit);

/* Erase previous XOR arrow and draw new arrow whenever pointer moves */
void ShowMouse(struct EDITTYPE *pEdit);

/* Draw top and left edges of box (x0,y0)-(x1,y1) for 3D bevel shading */
void WhiteBlack(int x0, int y0, int x1, int y1, int color);

/* Briefly invert and restore bevel borders on click for tactile feedback */
void button3d(int x0, int y0, int x1, int y1);

#endif
