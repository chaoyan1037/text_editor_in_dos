/*****************************************************************************
 * File: SUBMENUS.C
 * Purpose: Pull-down submenu rendering (F1..F4), keyboard navigation within
 *          open dropdowns, menu item -> command mapping, and Help popups.
 * Programmer: XiaoYao (2011.10.5-2011.10.7)
 *
 * Menu Geometry (each item is 80 px wide x 30 px tall, starting at Y=50):
 *   - F1 File  (X=5..85,   5 items): Open, New, Save, Save As, Close
 *   - F2 Edit  (X=95..175, 4 items): Cut, Copy, Paste, Select All
 *   - F3 Tools (X=185..265,2 items): Find, Find & Replace
 *   - F4 Help  (X=275..355,2 items): Operation Guide, About/Credits
 *****************************************************************************/
#include<graphics.h>
#include<bios.h>
#include"h/common.h"
#include"h/hanzi.h"
#include"h/struct.h"
#include"h/drawmenu.h"
#include"h/spell.h"
#include"h/mouse.h"
#include"h/submenus.h"

void SubmenuF1(struct EDITTYPE *pEdit);
void SubmenuF2(struct EDITTYPE *pEdit);
void SubmenuF3(struct EDITTYPE *pEdit);
void SubmenuF4(struct EDITTYPE *pEdit);
void EnterFunction(struct EDITTYPE *pEdit);
static void DrawSubmenuBox(int x1, int x2, char *items[], int count, struct EDITTYPE *pEdit);

/*****************************************************************************
 * Function: DrawSubmenuBox
 * Purpose:  Shared renderer for all four pull-down menus.
 *
 * @param x1,x2 Horizontal span of the dropdown panel.
 * @param items Array of `count` GB2312 Chinese menu item labels.
 * @param count Number of items in the menu (height = 30 * count px).
 * @param pEdit Editor context (`submenu.menuflag` gates repainting;
 *              `submenu.flag` selects the 1-based highlighted row).
 * @post        Sets `submenu.menuflag = 1` so subsequent idle frames do not
 *              flicker by repainting the dropdown repeatedly.
 *****************************************************************************/
static void DrawSubmenuBox(int x1, int x2, char *items[], int count, struct EDITTYPE *pEdit)
{
	int i;
	int selY1;
	int selY2;
	if(pEdit->submenu.menuflag!=0)
	{
		return;
	}
	pEdit->submenu.menuflag=1;
	selY1=50+30*(pEdit->submenu.flag-1);
	selY2=80+30*(pEdit->submenu.flag-1);

	/* Yellow panel background + green beveled selection bar */
	setfillstyle(SOLID_FILL, YELLOW);
	bar(x1, 50, x2, 50+30*count);

	setfillstyle(SOLID_FILL, LIGHTGREEN);
	bar(x1, selY1, x2, selY2);

	setcolor(DARKGRAY);
	rectangle(x1, selY1, x2, selY2);
	WhiteBlack(x1, selY1, x2, selY2, WHITE);

	for(i=0; i<count; i++)
	{
		Printfhz(x1+2, 55+30*i, items[i], RED, pEdit);
	}
}

/* SubmenuF1 - Render File menu (Open, New, Save, Save As, Close) */
void SubmenuF1(struct EDITTYPE *pEdit)
{
	char *menu[5];
	menu[0]="打开";
	menu[1]="新建";
	menu[2]="保存";
	menu[3]="另存为";
	menu[4]="关闭";
	DrawSubmenuBox(5, 85, menu, 5, pEdit);
}

/* SubmenuF2 - Render Edit menu (Cut, Copy, Paste, Select All) */
void SubmenuF2(struct EDITTYPE *pEdit)
{
	char *menu[4];
	menu[0]="剪切";
	menu[1]="复制";
	menu[2]="粘贴";
	menu[3]="全选";
	DrawSubmenuBox(95, 175, menu, 4, pEdit);
}

/* SubmenuF3 - Render Tools menu (Find, Find & Replace) */
void SubmenuF3(struct EDITTYPE *pEdit)
{
	char *menu[2];
	menu[0]="查找";
	menu[1]="查找替换";
	DrawSubmenuBox(185, 265, menu, 2, pEdit);
}

/* SubmenuF4 - Render Help menu (Operation Guide, About) */
void SubmenuF4(struct EDITTYPE *pEdit)
{
	char *menu[2];
	menu[0]="操作指南";
	menu[1]="说明";
	DrawSubmenuBox(275, 355, menu, 2, pEdit);
}

/*****************************************************************************
 * Function: GetSubmenu
 * Purpose:  Open the pull-down menu requested by F1..F4 (or mouse click).
 *
 * @param pEdit Editor context. When opened by mouse click, ChooseSubmenu()
 *              sets `state.key = 0` and pre-populates `editActive`, so the
 *              switch leaves `editActive` intact while arming `menuflag = 0`.
 *****************************************************************************/
void GetSubmenu(struct EDITTYPE *pEdit)
{
	pEdit->submenu.menuflag=0;
	switch(pEdit->state.key)
	{
		case F1: pEdit->editActive=1; break;
		case F2: pEdit->editActive=2; break;
		case F3: pEdit->editActive=3; break;
		case F4: pEdit->editActive=4; break;
		default: break;
	}
}

/*****************************************************************************
 * Function: Submenu
 * Purpose:  Main-loop handler while a pull-down menu is open (`editActive!=0`).
 *           Draws the active dropdown and handles UP/DOWN/ENTER/ESC keys.
 *
 * @param pEdit Master editor context.
 *****************************************************************************/
void Submenu(struct EDITTYPE *pEdit)
{
	switch(pEdit->editActive)
	{
		case 1: SubmenuF1(pEdit); break;
		case 2: SubmenuF2(pEdit); break;
		case 3: SubmenuF3(pEdit); break;
		case 4: SubmenuF4(pEdit); break;
		default: break;
	}
	if(bioskey(1)!=0)
	{
		pEdit->state.key=bioskey(0);

		switch(pEdit->state.key)
		{
			case UP:
				if(pEdit->submenu.flag>1)
				{
					pEdit->submenu.flag--;
					pEdit->submenu.menuflag=0;
				}
				break;
			case DOWN:
				if(pEdit->editActive>=1 && pEdit->editActive<=4
					&& pEdit->submenu.flag<pEdit->submenu.SubmenuNum[pEdit->editActive-1])
				{
					pEdit->submenu.flag++;
					pEdit->submenu.menuflag=0;
				}
				break;
			case ENTER:
				EnterFunction(pEdit);
				break;
			case ESC:
				ExitSubmenu(pEdit);
				break;
			default:
				break;
		}
	}
}

/*****************************************************************************
 * Function: EnterFunction
 * Purpose:  Execute the currently highlighted menu item by queuing its
 *           corresponding `state.function` command ID (or displaying a Help
 *           popup directly) and closing the dropdown menu.
 *
 * @param pEdit Master editor context.
 *****************************************************************************/
void EnterFunction(struct EDITTYPE *pEdit)
{
	switch(pEdit->editActive)
	{
		case 1: /* File menu */
			switch(pEdit->submenu.flag)
			{
				case 1: pEdit->state.function=10; break; /* Open */
				case 2: pEdit->state.function=14; break; /* New */
				case 3: pEdit->state.function=9;  break; /* Save */
				case 4: pEdit->state.function=15; break; /* Save As */
				case 5: pEdit->state.function=1;  break; /* Exit */
				default: break;
			}
			ExitSubmenu(pEdit);
			break;
		case 2: /* Edit menu */
			switch(pEdit->submenu.flag)
			{
				case 1: pEdit->state.function=13; break; /* Cut */
				case 2: pEdit->state.function=12; break; /* Copy */
				case 3: pEdit->state.function=4;  break; /* Paste */
				case 4: pEdit->state.function=25; break; /* Select All */
				default: break;
			}
			ExitSubmenu(pEdit);
			break;
		case 3: /* Tools menu */
			switch(pEdit->submenu.flag)
			{
				case 1: pEdit->state.function=5;  break; /* Find */
				case 2: pEdit->state.function=21; break; /* Find & Replace */
				default: break;
			}
			ExitSubmenu(pEdit);
			break;
		case 4: /* Help menu */
			switch(pEdit->submenu.flag)
			{
				case 1:
					OperationDirection(pEdit);
					pEdit->state.mouseFlag=0;
					break;
				case 2:
					Explanation(pEdit);
					pEdit->state.mouseFlag=0;
					break;
				default: break;
			}
			break;
		default:
			break;
	}
}

/*****************************************************************************
 * Function: ExitSubmenu
 * Purpose:  Dismiss the active pull-down menu, restore its top-bar button,
 *           and schedule a viewport repaint to erase the dropdown overlay.
 *
 * @param pEdit Master editor context.
 * @post        `editActive = 0`, `submenu.flag = 1`, `editFlag = 1`,
 *              `state.mouseFlag = 0`, text justification restored to BOTTOM.
 *****************************************************************************/
void ExitSubmenu(struct EDITTYPE *pEdit)
{
	int temp=pEdit->editActive;
	pEdit->editActive=0;
	pEdit->submenu.flag=1;
	pEdit->editFlag=1;
	pEdit->state.mouseFlag=0;
	settextjustify(LEFT_TEXT, TOP_TEXT);
	switch(temp)
	{
		case 1: DrawMenu1(pEdit); break;
		case 2: DrawMenu2(pEdit); break;
		case 3: DrawMenu3(pEdit); break;
		case 4: DrawMenu4(pEdit); break;
		default: break;
	}
	settextjustify(LEFT_TEXT, BOTTOM_TEXT);
}

/*****************************************************************************
 * Function: OperationDirection
 * Purpose:  Display the yellow "操作指南" usage tips popup (275..470 x 50..145).
 *
 * @param pEdit Master editor context.
 * @post        Closes menu (`editActive = 0`) and sets `functionActive = 1`
 *              so any keypress or mouse click cleanly dismisses the popup.
 *****************************************************************************/
void OperationDirection(struct EDITTYPE *pEdit)
{
	int i;
	char *string[5];
	setfillstyle(SOLID_FILL, YELLOW);
	bar(275, 50, 470, 145);
	setcolor(BLACK);
	rectangle(275, 50, 470, 145);
	WhiteBlack(275, 50, 470, 145, WHITE);

	string[0]="⒈支持鼠标键盘同时操作";
	string[1]="⒉快捷键设置和记事本一致";
	string[2]="";
	string[3]="";
	string[4]="⒌一直被模仿，从未被超越";
	for(i=0; i<5; i++)
	{
		Printfhz(275, 52+18*i, string[i], RED, pEdit);
	}
	pEdit->editActive=0;
	pEdit->submenu.flag=1;
	pEdit->state.functionActive=1;
}

/*****************************************************************************
 * Function: Explanation
 * Purpose:  Display the yellow "制作人员" credits popup (275..470 x 50..145),
 *           combining HZK16 text with custom bitmap OutputYao() for "肖垚".
 *
 * @param pEdit Master editor context.
 * @post        Closes menu (`editActive = 0`) and sets `functionActive = 1`
 *              so any keypress or mouse click cleanly dismisses the popup.
 *****************************************************************************/
void Explanation(struct EDITTYPE *pEdit)
{
	int i;
	char *string[5];
	setfillstyle(SOLID_FILL, YELLOW);
	bar(275, 50, 470, 145);
	setcolor(BLACK);
	rectangle(275, 50, 470, 145);
	WhiteBlack(275, 50, 470, 145, WHITE);

	string[0]="制作人员";
	string[1]="晏超超";
	string[2]="肖";
	string[3]="";
	string[4]="一直被模仿，从未被超越";
	OutputYao(306, 88, RED);
	for(i=0; i<5; i++)
	{
		Printfhz(290, 52+18*i, string[i], RED, pEdit);
	}
	pEdit->editActive=0;
	pEdit->submenu.flag=1;
	pEdit->state.functionActive=1;
}
