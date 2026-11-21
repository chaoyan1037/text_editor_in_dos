/*****************************************************************************
 * File: MOUSE.C
 * Function: DOS INT 33h mouse driver wrapper, XOR software cursor drawing,
 *           modal/menu/editor click hit-testing, and 3D bevel button effects.
 * Programmer: XiaoYao (2011.9.10-2011.11.4)
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

/*
 * InitMouse - Initialize DOS mouse driver (INT 33h) and clamp pointer motion
 *             to the 640x480 screen bounds.
 *   AX=0: Reset driver and detect presence (returns 0 if no mouse installed)
 *   AX=7: Set horizontal min/max (CX..DX)
 *   AX=8: Set vertical min/max (CX..DX)
 */
int InitMouse(void)
{
	int retcode;
	union REGS regs;
	int xmin=2, xmax=X_MAX-1;
	int ymin=2, ymax=Y_MAX-1;

	regs.x.ax=0;
	int86(MOUSEID, &regs, &regs);
	retcode=regs.x.ax;

	if(!retcode)
	{
		return 0;
	}

	regs.x.ax=7;
	regs.x.cx=xmin;
	regs.x.dx=xmax;
	int86(MOUSEID, &regs, &regs);

	regs.x.ax=8;
	regs.x.cx=ymin;
	regs.x.dx=ymax;
	int86(MOUSEID, &regs, &regs);

	return retcode;
}

/*
 * ReadMouse - Poll current pointer coordinates (CX, DX) and button state (BX)
 *             via INT 33h function AX=3.
 */
void ReadMouse(struct EDITTYPE *pEdit)
{
	union REGS regs;
	pEdit->state.button=0;

	regs.x.ax=3;
	int86(MOUSEID, &regs, &regs);
	pEdit->state.x=regs.x.cx;
	pEdit->state.y=regs.x.dx;
	pEdit->state.button=regs.x.bx;
}

/* DrawMouse - Outline the arrow pointer at (state.x, state.y) in active mode */
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

/*
 * ShowMouse - Poll mouse movement and update the XOR software pointer.
 *
 * When the pointer moves, XOR-erases the previous arrow (only if `mouseFlag`
 * indicates it is still visible on screen) and XOR-draws it at the new position.
 */
void ShowMouse(struct EDITTYPE *pEdit)
{
	struct MOUSESTATE state0;
	state0 = pEdit->state;
	ReadMouse(pEdit);
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

/*
 * MouseKey - Dispatch mouse clicks across modal dialogs, editor text area,
 *            title/status buttons, scrollbar arrows, and pull-down menus.
 *
 * Modal dialog hit-testing is strictly gated on `fa != 0` so invisible dialog
 * coordinates cannot create dead click zones inside the text editor when no
 * dialog is active.
 */
void MouseKey(struct EDITTYPE *pEdit)
{
	int x=pEdit->state.x;
	int y=pEdit->state.y;
	int b=pEdit->state.button;
	int fa=pEdit->state.functionActive;
	setwritemode(COPY_PUT);

	/* 1. Active modal dialog buttons */
	if(fa!=0 && b==1)
	{
		/* Left button ("确定" / Y) on Save/Open/New/Exit dialogs */
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
			else if(fa==11) /* Save current file before opening new one */
			{
				button3d(246, 214, 306, 232);
				Save(pEdit->file.fname, pEdit);
				pEdit->state.function=19;
				pEdit->state.functionActive=0;
				delay(100);
				return;
			}
			else if(fa==12) /* Save current file before New */
			{
				button3d(246, 214, 306, 232);
				Save(pEdit->file.fname, pEdit);
				pEdit->state.function=11;
				pEdit->state.functionActive=0;
				delay(100);
				return;
			}
			else if(fa==15) /* Confirm Save As filename */
			{
				button3d(246, 214, 306, 232);
				Saveas(pEdit);
				pEdit->state.functionActive=0;
				delay(100);
				return;
			}
		}
		/* Right button ("取消" / N) on Save/Open/New/Exit dialogs */
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

		/* Left button ("查找" / "替换") on Find / Replace dialogs */
		if(x<=314 && x>=258 && y>=214 && y<=232)
		{
			if(fa==7) /* Execute replacement */
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
		/* Right button ("取消") on Find / Replace dialogs */
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

		/* Info/alert popup dismissal (invalid filename, not found, Help) */
		if(fa==1)
		{
			pEdit->state.functionActive=0;
			pEdit->editFlag=1;
			pEdit->state.mouseFlag=0;
			delay(100);
			return;
		}
	}

	/* 2. Click inside text editing area (excluding scrollbar) to move caret */
	if(fa==0 && pEdit->editActive==0 && b==1
		&& x>=EDITERSCREENX1 && x<=EDITERSCREENX2-20
		&& y>=EDITERSCREENY1 && y<=EDITERSCREENY2)
	{
		int xTemp=pEdit->state.x;
		int yTemp=pEdit->state.y;
		struct MOUSESTATE tempState=pEdit->state;
		if( ConvertNodePosi(pEdit, x, y)==1 )
		{
			/* Long press enters drag-selection; quick click moves caret */
			MouseBWChosen(pEdit);
			pEdit->state.x=xTemp;
			pEdit->state.y=yTemp;
			if( pEdit->state.button==0 )
			{
				pEdit->state=tempState;
				pEdit->state.function=2;
				pEdit->state.key=0; /* Refresh caret without directional step */
			}
		}
	}
	/* 3. Top-right Exit button */
	else if(b==1 && x<=635 && x>=595 && y>=3 && y<=22)
	{
		button3d(595, 3, 635, 22);
		pEdit->state.function=1;
		delay(100);
	}
	/* 4. Bottom-left EN/ZH language toggle button */
	else if(b==1 && x<=28 && x>=5 && y>=455 && y<=475)
	{
		button3d(5, 455, 28, 475);
		pEdit->state.function=8;
		pEdit->state.mouseFlag=0;
		delay(100);
	}

	/* 5. Scrollbar up/down arrow clicks */
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

	/* 6. Top menu bar and open pull-down submenu interaction */
	ChooseSubmenu(pEdit);
}

/*
 * ChooseSubmenu - Handle clicks on the F1..F4 top menu bar buttons and
 *                 delegate hover/click events when a pull-down menu is open.
 */
void ChooseSubmenu(struct EDITTYPE *pEdit)
{
	int x=pEdit->state.x;
	int y=pEdit->state.y;
	int b=pEdit->state.button;

	if(x>=5 && x<=634 && y>=50 && y<=450 && pEdit->editActive!=0)
	{
		if(pEdit->editActive==1)      Submenu1Op(pEdit);
		else if(pEdit->editActive==2) Submenu2Op(pEdit);
		else if(pEdit->editActive==3) Submenu3Op(pEdit);
		else if(pEdit->editActive==4) Submenu4Op(pEdit);
	}
	if(pEdit->editActive==0 && pEdit->state.functionActive==0 && b==1)
	{
		/* Clear state.key=0 so GetSubmenu() won't override editActive with a stale F1..F4 key */
		if(x>=5 && x<=70 && y>=25 && y<50)
		{
			button3d(4, 27, 70, 48);
			pEdit->state.key=0;
			pEdit->state.function=6;
			pEdit->editActive=1;
			pEdit->submenu.flag=1;
			pEdit->state.mouseFlag=1;
			delay(100);
		}
		else if(x>=95 && x<=160 && y>=25 && y<=50)
		{
			button3d(94, 27, 160, 48);
			pEdit->state.key=0;
			pEdit->state.function=6;
			pEdit->editActive=2;
			pEdit->submenu.flag=1;
			pEdit->state.mouseFlag=1;
			delay(100);
		}
		else if(x>=185 && x<=250 && y>=25 && y<=50)
		{
			button3d(184, 27, 250, 48);
			pEdit->state.key=0;
			pEdit->state.function=6;
			pEdit->editActive=3;
			pEdit->submenu.flag=1;
			pEdit->state.mouseFlag=1;
			delay(100);
		}
		else if(x>=275 && x<=340 && y>=25 && y<=50)
		{
			button3d(274, 27, 340, 48);
			pEdit->state.key=0;
			pEdit->state.function=6;
			pEdit->editActive=4;
			pEdit->submenu.flag=1;
			pEdit->state.mouseFlag=1;
			delay(100);
		}
	}
}

/* Submenu1Op - Mouse hover highlight and click dispatch for File (F1) menu */
void Submenu1Op(struct EDITTYPE *pEdit)
{
	int x=pEdit->state.x;
	int y=pEdit->state.y;
	int b=pEdit->state.button;

	if(x>=5 && x<=85 && y>50 && y<=80) /* Open */
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
	else if(x>=5 && x<=85 && y>80 && y<=110) /* New */
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
	else if(x>=5 && x<=85 && y>110 && y<=140) /* Save */
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
	else if(x>=5 && x<=85 && y>140 && y<=170) /* Save As */
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
	else if(x>=5 && x<=85 && y>170 && y<=200) /* Exit */
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
	else if(b==1) /* Click outside dropdown closes it */
	{
		ExitSubmenu(pEdit);
		delay(100);
	}
}

/* Submenu2Op - Mouse hover highlight and click dispatch for Edit (F2) menu */
void Submenu2Op(struct EDITTYPE *pEdit)
{
	int x=pEdit->state.x;
	int y=pEdit->state.y;
	int b=pEdit->state.button;

	if(x>=95 && x<=175 && y>50 && y<=80) /* Cut */
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
	else if(x>=95 && x<=175 && y>80 && y<=110) /* Copy */
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
	else if(x>=95 && x<=175 && y>110 && y<=140) /* Paste */
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
	else if(x>=95 && x<=175 && y>140 && y<=170) /* Select All */
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

/* Submenu3Op - Mouse hover highlight and click dispatch for Tools (F3) menu */
void Submenu3Op(struct EDITTYPE *pEdit)
{
	int x=pEdit->state.x;
	int y=pEdit->state.y;
	int b=pEdit->state.button;

	if(x>=185 && x<=265 && y>50 && y<=80) /* Find */
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
	else if(x>=185 && x<=265 && y>80 && y<=110) /* Find & Replace */
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

/* Submenu4Op - Mouse hover highlight and click dispatch for Help (F4) menu */
void Submenu4Op(struct EDITTYPE *pEdit)
{
	int x=pEdit->state.x;
	int y=pEdit->state.y;
	int b=pEdit->state.button;

	if(x>=275 && x<=355 && y>50 && y<=80) /* Operation Guide */
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
	else if(x>=275 && x<=355 && y>80 && y<=110) /* About / Credits */
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

/* WhiteBlack - Draw top and left border lines to create a 3D bevel highlight */
void WhiteBlack(int x0, int y0, int x1, int y1, int color)
{
	setcolor(color);
	setwritemode(COPY_PUT);
	line(x0, y0, x1, y0);
	line(x0, y0, x0, y1);
}

/* button3d - Briefly invert and restore a button's 3D bevel on click */
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
