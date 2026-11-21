/*****************************************************************************
 * File: MOUSE.H
 * Purpose: DOS INT 33h mouse driver interface, XOR software cursor rendering,
 *          UI hit-testing, and 3D bevel button helpers.
 *****************************************************************************/
#ifndef _MOUSE_H_
#define _MOUSE_H_

#define MOUSEID 0X33 /* DOS mouse driver software interrupt vector */

struct EDITTYPE;

int InitMouse(void);                            /* Reset mouse and clamp bounds */
void ReadMouse(struct EDITTYPE *pEdit);         /* Poll mouse X/Y/button state */
void MouseKey(struct EDITTYPE *pEdit);          /* Dispatch clicks on UI regions */
void ShowMouse(struct EDITTYPE *pEdit);         /* Redraw XOR arrow on movement */
void WhiteBlack(int x0, int y0, int x1, int y1, int color); /* Draw top-left bevel */
void button3d(int x0, int y0, int x1, int y1);  /* Briefly animate 3D press */

#endif
