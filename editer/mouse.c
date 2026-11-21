/*****************************************************************************
 * File: MOUSE.C
 * Purpose: DOS INT 33h mouse driver wrapper, XOR software cursor renderer,
 *          modal/viewport/button/menu click hit-testing, and 3D bevel helpers.
 * Programmer: XiaoYao (2011.9.10-2011.11.4)
 *
 * DOS Mouse Driver (INT 33h) Functions Used:
 *   - AX = 0x0000: Reset driver & query presence (returns AX != 0 if present).
 *   - AX = 0x0003: Read position (CX=X, DX=Y) and button bitmask (BX, 1=left).
 *   - AX = 0x0007: Set horizontal cursor clamp [CX .. DX].
 *   - AX = 0x0008: Set vertical cursor clamp [CX .. DX].
 *
 * XOR Software Pointer Protocol:
 *   - Hardware mouse cursors do not work cleanly over BGI VGAHI graphics, so
 *     ShowMouse() draws a 9x17 arrow outline in XOR_PUT mode.
 *   - Any routine that repaints part of the screen over the pointer resets
 *     `state.mouseFlag = 0` so ShowMouse() will NOT XOR-erase a missing arrow.
 *****************************************************************************/
#include<graphics.h>
#include<stdlib.h>
#include<dos.h>
#include<bios.h>
#include"h/struct.h"
#include"h/common.h"
#include"h/node.h"
#include"h/submenus.h"
#include"h/save.h"
#include"h/funckey.h"
#include"h/mouse.h"

void DrawMouse(struct MOUSESTATE state);
void Submenu1Op(struct EDITTYPE *pEdit);
void Submenu2Op(struct EDITTYPE *pEdit);
void Submenu3Op(struct EDITTYPE *pEdit);
void Submenu4Op(struct EDITTYPE *pEdit);
void ChooseSubmenu(struct EDITTYPE *pEdit);

/*****************************************************************************
 * Function: InitMouse
 * Purpose:  Reset the DOS mouse driver via INT 33h and clamp pointer travel
 *           within [2..X_MAX-1] x [2..Y_MAX-1].
 *
 * @return   Non-zero mouse status code on success; 0 if no driver detected.
 *****************************************************************************/
int InitMouse(void)
{
	int retcode;
	union REGS regs;
	int xmin=2, xmax=X_MAX-1;
	int ymin=2, ymax=Y_MAX-1;

	/* AX=0: Reset mouse driver and test installation */
	regs.x.ax=0;
	int86(MOUSEID, &regs, &regs);
	retcode=regs.x.ax;

	if(!retcode)
	{
		return 0;
	}

	/* AX=7: Horizontal travel bounds */
	regs.x.ax=7;
	regs.x.cx=xmin;
	regs.x.dx=xmax;
	int86(MOUSEID, &regs, &regs);

	/* AX=8: Vertical travel bounds */
	regs.x.ax=8;
	regs.x.cx=ymin;
	regs.x.dx=ymax;
	int86(MOUSEID, &regs, &regs);

	return retcode;
}

static int g_menu_btn_down=0;

/*****************************************************************************
 * Function: ReadMouse
 * Purpose:  Poll current pointer coordinates and button mask via INT 33h/AX=3.
 *
 * @param pEdit Editor context whose `state.{x,y,button}` fields are updated.
 *****************************************************************************/
void ReadMouse(struct EDITTYPE *pEdit)
{
	union REGS regs;
	pEdit->state.button=0;

	regs.x.ax=3;
	int86(MOUSEID, &regs, &regs);
	pEdit->state.x=regs.x.cx;
	pEdit->state.y=regs.x.dx;
	pEdit->state.button=regs.x.bx;
	if(pEdit->state.button==0)
	{
		g_menu_btn_down=0;
	}
}

/* DrawMouse - Plot the 7-segment arrow cursor outline at (state.x, state.y) */
void DrawMouse(struct MOUSESTATE state)
{
	int x=state.x, y=state.y;
	setcolor(WHITE);
	line(x,y,x+8,y+8);
	line(x+8,y+8,x+5,y+8);
	line(x+5,y+8,x+8,y+16);
	line(x+8,y+16,x+6,y+16);
	line(x+6,y+16,x+3,y+9);
	line(x+3,y+9,x,y+12);
	line(x,y+12,x,y);
}

/*****************************************************************************
 * Function: ShowMouse
 * Purpose:  Poll mouse movement and update the XOR software arrow cursor.
 *
 * @param pEdit Master editor context.
 * @post        If the pointer moved, erases old arrow (when `mouseFlag==1`),
 *              draws new arrow, sets `mouseFlag=1`, and restores COPY_PUT.
 *****************************************************************************/
void ShowMouse(struct EDITTYPE *pEdit)
{
	struct MOUSESTATE state0;
	state0 = pEdit->state;
	ReadMouse(pEdit);
#ifndef __MSDOS__
	/* On Linux terminal backend, host terminal renders the pointer */
	if(bgi_is_native_mouse())
	{
		return;
	}
#endif
	setwritemode(XOR_PUT);

	if( (state0.x!=pEdit->state.x) || (state0.y!=pEdit->state.y) )
	{
		if(pEdit->state.mouseFlag==1)
		{
			DrawMouse(state0);
		}
		pEdit->state.mouseFlag=1;
		DrawMouse(pEdit->state);
	}
	setwritemode(COPY_PUT);
}

/*****************************************************************************
 * Function: MouseKey
 * Purpose:  Central mouse hit-tester and click dispatcher for all UI regions.
 *
 * Dispatch Priority:
 *   1. Active modal dialog buttons (ONLY when `functionActive != 0 && b == 1`,
 *      preventing invisible dialogs from creating dead click zones over text).
 *   2. Viewport text clicks (when `fa==0 && editActive==0`, excluding the
 *      right-hand 20px scrollbar) -> ConvertNodePosi() + MouseBWChosen().
 *   3. Top-right Exit button (595..635 x 3..22) and bottom-left EN/ZH toggle
 *      (5..28 x 455..475).
 *   4. Scrollbar up/down arrows (616..632 x 52..64 / 436..448).
 *   5. Top menu bar buttons and open pull-down submenus via ChooseSubmenu().
 *
 * @param pEdit Master editor context.
 * @note        100ms delays after click actions serve as simple DOS button
 *              debouncing so a single click does not fire twice across frames.
 *****************************************************************************/
void MouseKey(struct EDITTYPE *pEdit)
{
	int x=pEdit->state.x;
	int y=pEdit->state.y;
	int b=pEdit->state.button;
	int fa=pEdit->state.functionActive;
	setwritemode(COPY_PUT);

	/* 1. Active modal dialog hit-testing (left-click only) */
	if(fa!=0 && b==1)
	{
		/* Left OK(Y) button of Save/Open/New/Exit dialogs (246..306 x 214..232) */
		if(x<=306 && x>=246 && y>=214 && y<=232)
		{
			if(fa==2) /* Save before Exit */
			{
				button3d(246, 214, 306, 232);
				Save(pEdit->file.fname, pEdit);
				pEdit->state.functionActive=0;
				delay(100);
				ExitEditer(pEdit);
				return;
			}
			else if(fa==9) /* Save */
			{
				button3d(246, 214, 306, 232);
				pEdit->state.function=17;
				pEdit->state.functionActive=0;
				delay(100);
				return;
			}
			else if(fa==10) /* Confirm Open filename */
			{
				button3d(246, 214, 306, 232);
				pEdit->state.function=18;
				pEdit->state.functionActive=0;
				pEdit->state.mouseFlag=0;
				delay(100);
				return;
			}
			else if(fa==11) /* Save active file before opening target file */
			{
				button3d(246, 214, 306, 232);
				Save(pEdit->file.fname, pEdit);
				pEdit->state.function=19;
				pEdit->state.functionActive=0;
				delay(100);
				return;
			}
			else if(fa==12) /* Save active file before New */
			{
				button3d(246, 214, 306, 232);
				Save(pEdit->file.fname, pEdit);
				pEdit->state.function=11;
				pEdit->state.functionActive=0;
				delay(100);
				return;
			}
			else if(fa==15) /* Confirm Save As */
			{
				button3d(246, 214, 306, 232);
				Saveas(pEdit);
				pEdit->state.functionActive=0;
				delay(100);
				return;
			}
		}
		/* Right Cancel(N) button of Save/Open/New/Exit dialogs (314..374 x 214..232) */
		else if(x<=374 && x>=314 && y>=214 && y<=232)
		{
			if(fa==2) /* Exit without saving */
			{
				button3d(314, 214, 374, 232);
				pEdit->state.functionActive=0;
				delay(100);
				ExitEditer(pEdit);
				return;
			}
			else if(fa==9 || fa==10 || fa==15) /* Cancel Save / Open / Save As */
			{
				button3d(314, 214, 374, 232);
				pEdit->state.functionActive=0;
				pEdit->editFlag=1;
				delay(100);
				return;
			}
			else if(fa==11) /* Open without saving current file */
			{
				button3d(314, 214, 374, 232);
				pEdit->state.function=19;
				pEdit->state.functionActive=0;
				delay(100);
				return;
			}
			else if(fa==12) /* New without saving current file */
			{
				button3d(314, 214, 374, 232);
				pEdit->state.function=11;
				pEdit->state.functionActive=0;
				pEdit->state.mouseFlag=0;
				delay(100);
				return;
			}
		}

		/* Left action button of Find/Replace dialog (258..314 x 214..232) */
		if(x<=314 && x>=258 && y>=214 && y<=232)
		{
			if(fa==7) /* Replace highlighted match */
			{
				button3d(258, 214, 314, 232);
				pEdit->state.function=22;
				delay(100);
				return;
			}
			else if(fa==5 || fa==6) /* Execute search */
			{
				button3d(258, 214, 314, 232);
				pEdit->state.function=16;
				if(fa==6)
				{
					pEdit->state.mouseFlag=0;
				}
				delay(200);
				return;
			}
		}
		/* Right Cancel button of Find/Replace dialog (323..379 x 214..232) */
		else if(x<=379 && x>=323 && y>=214 && y<=232)
		{
			if(fa==5)
			{
				button3d(323, 214, 379, 232);
				pEdit->state.function=20; /* Cancel Find */
				delay(100);
				return;
			}
			else if(fa==6 || fa==7)
			{
				button3d(323, 214, 379, 232);
				pEdit->state.function=23; /* Cancel Replace */
				delay(100);
				return;
			}
		}

		/* Any click dismisses an info/alert/Help popup (state 1) */
		if(fa==1)
		{
			pEdit->state.functionActive=0;
			pEdit->editFlag=1;
			pEdit->state.mouseFlag=0;
			delay(100);
			return;
		}
	}

	/* 2. Click inside text viewport (excluding 20px scrollbar) to move caret */
	if(fa==0 && pEdit->editActive==0 && b==1
		&& x>=EDITERSCREENX1 && x<=EDITERSCREENX2-20
		&& y>=EDITERSCREENY1 && y<=EDITERSCREENY2)
	{
		int xTemp=pEdit->state.x;
		int yTemp=pEdit->state.y;
		struct MOUSESTATE tempState=pEdit->state;
		if( ConvertNodePosi(pEdit, x, y)==1 )
		{
			/* Distinguish quick click (reposition caret) vs hold (select text) */
			MouseBWChosen(pEdit);
			pEdit->state.x=xTemp;
			pEdit->state.y=yTemp;
			if( pEdit->state.button==0 )
			{
				pEdit->state=tempState;
				pEdit->state.function=2;
				pEdit->state.key=0; /* Trigger caret repaint without arrow step */
			}
		}
	}
	/* 3. Top-right Exit button (595..635 x 3..22) */
	else if(b==1 && x<=635 && x>=595 && y>=3 && y<=22)
	{
		if(pEdit->editActive!=0)
		{
			ExitSubmenu(pEdit);
		}
		button3d(595, 3, 635, 22);
		pEdit->state.function=1;
		delay(100);
	}
	/* 4. Bottom-left EN/ZH language toggle button (5..28 x 455..475) */
	else if(b==1 && x<=28 && x>=5 && y>=455 && y<=475)
	{
		if(pEdit->editActive!=0)
		{
			ExitSubmenu(pEdit);
		}
		button3d(5, 455, 28, 475);
		pEdit->state.function=8;
		pEdit->state.mouseFlag=0;
		delay(100);
	}

	/* 5. Scrollbar arrow buttons: top (Y=52..64) and bottom (Y=436..448) */
	if(fa==0 && pEdit->editActive==0 && b==1
		&& x<=632 && x>=616 && y>=52 && y<=64)
	{
		pEdit->state.function=2;
		pEdit->state.key=UP;
	}
	if(fa==0 && pEdit->editActive==0 && b==1
		&& x<=632 && x>=616 && y>=436 && y<=448)
	{
		pEdit->state.function=2;
		pEdit->state.key=DOWN;
	}

	/* 6. Top menu bar and active dropdown menu hit-testing */
	ChooseSubmenu(pEdit);
}

/*****************************************************************************
 * Function: ChooseSubmenu
 * Purpose:  Handle clicks on top-bar menu buttons (F1..F4) — opening a menu,
 *           switching directly to another menu while one is already open, or
 *           toggling the active menu closed — and forward hover/clicks inside
 *           the viewport to Submenu1Op..4Op when a dropdown is open.
 *
 * @param pEdit Master editor context.
 *****************************************************************************/
void ChooseSubmenu(struct EDITTYPE *pEdit)
{
	int x=pEdit->state.x;
	int y=pEdit->state.y;
	int b=pEdit->state.button;
	int clickEdge=(b==1 && g_menu_btn_down==0);
	if(b==0)
	{
		g_menu_btn_down=0;
	}
	else if(clickEdge)
	{
		g_menu_btn_down=1;
	}

	/* Top menu bar (Y=25..49): allow direct open, switch, or toggle-close */
	if(pEdit->state.functionActive==0 && clickEdge && y>=25 && y<50)
	{
		int clickedMenu=0;
		if(x>=4 && x<=70)         clickedMenu=1; /* File (<F1>) */
		else if(x>=88 && x<=160)  clickedMenu=2; /* Edit (<F2>) */
		else if(x>=184 && x<=250) clickedMenu=3; /* Tools (<F3>) */
		else if(x>=272 && x<=340) clickedMenu=4; /* Help (<F4>) */

		if(clickedMenu!=0)
		{
			if(pEdit->editActive==clickedMenu)
			{
				/* Clicking the already-open menu header toggles it closed */
				ExitSubmenu(pEdit);
				delay(100);
			}
			else
			{
				/* Close any open dropdown so ViewAll() erases it before drawing the next */
				if(pEdit->editActive!=0)
				{
					ExitSubmenu(pEdit);
				}
				switch(clickedMenu)
				{
					case 1: button3d(4, 27, 70, 48);    break;
					case 2: button3d(94, 27, 160, 48);  break;
					case 3: button3d(184, 27, 250, 48); break;
					case 4: button3d(274, 27, 340, 48); break;
					default: break;
				}
				/* Always clear state.key=0 so GetSubmenu() preserves our editActive */
				pEdit->state.key=0;
				pEdit->state.function=6;
				pEdit->editActive=clickedMenu;
				pEdit->submenu.flag=1;
				pEdit->submenu.menuflag=0;
				pEdit->state.mouseFlag=1;
				delay(100);
			}
		}
		else if(pEdit->editActive!=0)
		{
			/* Clicking empty space on the menu bar closes the active dropdown */
			ExitSubmenu(pEdit);
			delay(100);
		}
		return;
	}

	if(pEdit->editActive!=0)
	{
		if(x>=5 && x<=634 && y>=50 && y<=450)
		{
			if(pEdit->editActive==1)      Submenu1Op(pEdit);
			else if(pEdit->editActive==2) Submenu2Op(pEdit);
			else if(pEdit->editActive==3) Submenu3Op(pEdit);
			else if(pEdit->editActive==4) Submenu4Op(pEdit);
		}
		else if(clickEdge)
		{
			/* Clicking outside the menu bar & viewport dismisses the dropdown */
			ExitSubmenu(pEdit);
			delay(100);
		}
	}
}

/* Submenu1Op - Hover tracking & click dispatch for File (F1) dropdown */
void Submenu1Op(struct EDITTYPE *pEdit)
{
	int x=pEdit->state.x;
	int y=pEdit->state.y;
	int b=pEdit->state.button;

	if(x>=5 && x<=85 && y>50 && y<=50+CHARHIGH) /* Item 1: Open */
	{
		if(pEdit->submenu.flag!=1)
		{
			pEdit->submenu.flag=1;
			pEdit->submenu.menuflag=0;
			pEdit->state.mouseFlag=0;
		}
		if(b==1)
		{
			pEdit->state.function=10;
			ExitSubmenu(pEdit);
			delay(100);
		}
	}
	else if(x>=5 && x<=85 && y>50+CHARHIGH && y<=50+2*CHARHIGH) /* Item 2: New */
	{
		if(pEdit->submenu.flag!=2)
		{
			pEdit->submenu.flag=2;
			pEdit->submenu.menuflag=0;
			pEdit->state.mouseFlag=0;
		}
		if(b==1)
		{
			pEdit->state.function=14;
			ExitSubmenu(pEdit);
			delay(100);
		}
	}
	else if(x>=5 && x<=85 && y>50+2*CHARHIGH && y<=50+3*CHARHIGH) /* Item 3: Save */
	{
		if(pEdit->submenu.flag!=3)
		{
			pEdit->submenu.flag=3;
			pEdit->submenu.menuflag=0;
			pEdit->state.mouseFlag=0;
		}
		if(b==1)
		{
			pEdit->state.function=9;
			ExitSubmenu(pEdit);
			delay(100);
		}
	}
	else if(x>=5 && x<=85 && y>50+3*CHARHIGH && y<=50+4*CHARHIGH) /* Item 4: Save As */
	{
		if(pEdit->submenu.flag!=4)
		{
			pEdit->submenu.flag=4;
			pEdit->submenu.menuflag=0;
			pEdit->state.mouseFlag=0;
		}
		if(b==1)
		{
			pEdit->state.function=15;
			ExitSubmenu(pEdit);
			delay(100);
		}
	}
	else if(x>=5 && x<=85 && y>50+4*CHARHIGH && y<=50+5*CHARHIGH) /* Item 5: Close/Exit */
	{
		if(pEdit->submenu.flag!=5)
		{
			pEdit->submenu.flag=5;
			pEdit->submenu.menuflag=0;
			pEdit->state.mouseFlag=0;
		}
		if(b==1)
		{
			pEdit->state.function=1;
			ExitSubmenu(pEdit);
			delay(100);
		}
	}
	else if(b==1) /* Clicking outside dropdown dismisses it */
	{
		ExitSubmenu(pEdit);
		delay(100);
	}
}

/* Submenu2Op - Hover tracking & click dispatch for Edit (F2) dropdown */
void Submenu2Op(struct EDITTYPE *pEdit)
{
	int x=pEdit->state.x;
	int y=pEdit->state.y;
	int b=pEdit->state.button;

	if(x>=95 && x<=175 && y>50 && y<=50+CHARHIGH) /* Item 1: Cut */
	{
		if(pEdit->submenu.flag!=1)
		{
			pEdit->submenu.flag=1;
			pEdit->submenu.menuflag=0;
			pEdit->state.mouseFlag=0;
		}
		if(b==1)
		{
			pEdit->state.function=13;
			ExitSubmenu(pEdit);
			delay(100);
		}
	}
	else if(x>=95 && x<=175 && y>50+CHARHIGH && y<=50+2*CHARHIGH) /* Item 2: Copy */
	{
		if(pEdit->submenu.flag!=2)
		{
			pEdit->submenu.flag=2;
			pEdit->submenu.menuflag=0;
			pEdit->state.mouseFlag=0;
		}
		if(b==1)
		{
			pEdit->state.function=12;
			ExitSubmenu(pEdit);
			delay(100);
		}
	}
	else if(x>=95 && x<=175 && y>50+2*CHARHIGH && y<=50+3*CHARHIGH) /* Item 3: Paste */
	{
		if(pEdit->submenu.flag!=3)
		{
			pEdit->submenu.flag=3;
			pEdit->submenu.menuflag=0;
			pEdit->state.mouseFlag=0;
		}
		if(b==1)
		{
			pEdit->state.function=4;
			ExitSubmenu(pEdit);
			delay(100);
		}
	}
	else if(x>=95 && x<=175 && y>50+3*CHARHIGH && y<=50+4*CHARHIGH) /* Item 4: Select All */
	{
		if(pEdit->submenu.flag!=4)
		{
			pEdit->submenu.flag=4;
			pEdit->submenu.menuflag=0;
			pEdit->state.mouseFlag=0;
		}
		if(b==1)
		{
			pEdit->state.function=25;
			ExitSubmenu(pEdit);
			delay(100);
		}
	}
	else if(b==1)
	{
		ExitSubmenu(pEdit);
		delay(100);
	}
}

/* Submenu3Op - Hover tracking & click dispatch for Tools (F3) dropdown */
void Submenu3Op(struct EDITTYPE *pEdit)
{
	int x=pEdit->state.x;
	int y=pEdit->state.y;
	int b=pEdit->state.button;

	if(x>=185 && x<=265 && y>50 && y<=50+CHARHIGH) /* Item 1: Find */
	{
		if(pEdit->submenu.flag!=1)
		{
			pEdit->submenu.flag=1;
			pEdit->submenu.menuflag=0;
			pEdit->state.mouseFlag=0;
		}
		if(b==1)
		{
			ExitSubmenu(pEdit);
			pEdit->state.function=5;
			delay(100);
		}
	}
	else if(x>=185 && x<=265 && y>50+CHARHIGH && y<=50+2*CHARHIGH) /* Item 2: Find & Replace */
	{
		if(pEdit->submenu.flag!=2)
		{
			pEdit->submenu.flag=2;
			pEdit->submenu.menuflag=0;
			pEdit->state.mouseFlag=0;
		}
		if(b==1)
		{
			ExitSubmenu(pEdit);
			pEdit->state.function=21;
			delay(100);
		}
	}
	else if(b==1)
	{
		ExitSubmenu(pEdit);
		delay(100);
	}
}

/* Submenu4Op - Hover tracking & click dispatch for Help (F4) dropdown */
void Submenu4Op(struct EDITTYPE *pEdit)
{
	int x=pEdit->state.x;
	int y=pEdit->state.y;
	int b=pEdit->state.button;

	if(x>=275 && x<=355 && y>50 && y<=50+CHARHIGH) /* Item 1: Operation Guide */
	{
		if(pEdit->submenu.flag!=1)
		{
			pEdit->submenu.flag=1;
			pEdit->submenu.menuflag=0;
			pEdit->state.mouseFlag=0;
		}
		if(b==1)
		{
			OperationDirection(pEdit);
			pEdit->state.mouseFlag=0;
			delay(100);
		}
	}
	else if(x>=275 && x<=355 && y>50+CHARHIGH && y<=50+2*CHARHIGH) /* Item 2: About/Credits */
	{
		if(pEdit->submenu.flag!=2)
		{
			pEdit->submenu.flag=2;
			pEdit->submenu.menuflag=0;
			pEdit->state.mouseFlag=0;
		}
		if(b==1)
		{
			Explanation(pEdit);
			pEdit->state.mouseFlag=0;
			delay(100);
		}
	}
	else if(b==1)
	{
		ExitSubmenu(pEdit);
		delay(100);
	}
}

/*****************************************************************************
 * Function: WhiteBlack
 * Purpose:  Draw the top and left edges of box (x0,y0)-(x1,y1) in `color`
 *           to produce a raised (WHITE) or sunken (BLACK) 3D bevel effect.
 *****************************************************************************/
void WhiteBlack(int x0, int y0, int x1, int y1, int color)
{
	setcolor(color);
	setwritemode(COPY_PUT);
	line(x0, y0, x1, y0);
	line(x0, y0, x0, y1);
}

/*****************************************************************************
 * Function: button3d
 * Purpose:  Animate a brief 10ms depressed->raised bevel flash when a button
 *           or dialog frame is clicked/opened.
 *****************************************************************************/
void button3d(int x0, int y0, int x1, int y1)
{
	setcolor(WHITE);
	rectangle(x0, y0, x1, y1);
	WhiteBlack(x0, y0, x1, y1, BLACK);
	delay(10);
	setcolor(BLACK);
	rectangle(x0, y0, x1, y1);
	WhiteBlack(x0, y0, x1, y1, WHITE);
}
