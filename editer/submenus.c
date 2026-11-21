/*****************************************************************************
 * File: SUBMENUS.C
 * Purpose: Pull-down submenu rendering (F1..F4), keyboard navigation within
 *          open dropdowns, menu item -> command mapping, and Help popups.
 * Programmer: XiaoYao (2011.10.5-2011.10.7)
 *
 * Menu Geometry (each item is 80 px wide x 16 px tall [1 row], starting at Y=50):
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
 * @param count Number of items in the menu (height = CHARHIGH * count px).
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
	selY1=50+CHARHIGH*(pEdit->submenu.flag-1);
	selY2=50+CHARHIGH*(pEdit->submenu.flag);

	/* Yellow panel background + green beveled selection bar */
	setfillstyle(SOLID_FILL, YELLOW);
	bar(x1, 50, x2, 50+CHARHIGH*count);

	setfillstyle(SOLID_FILL, LIGHTGREEN);
	bar(x1, selY1, x2, selY2);

	setcolor(DARKGRAY);
	rectangle(x1, selY1, x2, selY2);
	WhiteBlack(x1, selY1, x2, selY2, WHITE);

	for(i=0; i<count; i++)
	{
		Printfhz(x1+2, 50+CHARHIGH*i, items[i], RED, pEdit);
	}
}

/* SubmenuF1 - Render File menu (Open, New, Save, Save As, Close) */
void SubmenuF1(struct EDITTYPE *pEdit)
{
	char *menu[5];
	menu[0]="\xb4\xf2\xbf\xaa";
	menu[1]="\xd0\xc2\xbd\xa8";
	menu[2]="\xb1\xa3\xb4\xe6";
	menu[3]="\xc1\xed\xb4\xe6\xce\xaa";
	menu[4]="\xb9\xd8\xb1\xd5";
	DrawSubmenuBox(5, 85, menu, 5, pEdit);
}

/* SubmenuF2 - Render Edit menu (Cut, Copy, Paste, Select All) */
void SubmenuF2(struct EDITTYPE *pEdit)
{
	char *menu[4];
	menu[0]="\xbc\xf4\xc7\xd0";
	menu[1]="\xb8\xb4\xd6\xc6";
	menu[2]="\xd5\xb3\xcc\xf9";
	menu[3]="\xc8\xab\xd1\xa1";
	DrawSubmenuBox(95, 175, menu, 4, pEdit);
}

/* SubmenuF3 - Render Tools menu (Find, Find & Replace) */
void SubmenuF3(struct EDITTYPE *pEdit)
{
	char *menu[2];
	menu[0]="\xb2\xe9\xd5\xd2";
	menu[1]="\xb2\xe9\xd5\xd2\xcc\xe6\xbb\xbb";
	DrawSubmenuBox(185, 265, menu, 2, pEdit);
}

/* SubmenuF4 - Render Help menu (Operation Guide, About) */
void SubmenuF4(struct EDITTYPE *pEdit)
{
	char *menu[2];
	menu[0]="\xb2\xd9\xd7\xf7\xd6\xb8\xc4\xcf";
	menu[1]="\xcb\xb5\xc3\xf7";
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
 *           Draws the active dropdown and handles UP/DOWN/LEFT/RIGHT, F1..F4
 *           menu switching, ENTER, and ESC keys.
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
		int targetMenu=0;
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
			case LEFT:
				targetMenu=(pEdit->editActive>1) ? (pEdit->editActive-1) : 4;
				break;
			case RIGHT:
				targetMenu=(pEdit->editActive<4) ? (pEdit->editActive+1) : 1;
				break;
			case F1: targetMenu=1; break;
			case F2: targetMenu=2; break;
			case F3: targetMenu=3; break;
			case F4: targetMenu=4; break;
			case ENTER:
				EnterFunction(pEdit);
				break;
			case ESC:
				ExitSubmenu(pEdit);
				break;
			default:
				break;
		}

		if(targetMenu!=0)
		{
			if(targetMenu==pEdit->editActive)
			{
				ExitSubmenu(pEdit);
			}
			else
			{
				ExitSubmenu(pEdit);
				pEdit->state.key=0;
				pEdit->state.function=6;
				pEdit->editActive=targetMenu;
				pEdit->submenu.flag=1;
				pEdit->submenu.menuflag=0;
			}
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
	bar(275, 50, 475, 134);
	setcolor(BLACK);
	rectangle(275, 50, 475, 134);
	WhiteBlack(275, 50, 475, 134, WHITE);

	/* 1. 支持鼠标键盘同时操作 */
	string[0]="\xa2\xb1\xd6\xa7\xb3\xd6\xca\xf3\xb1\xea\xbc\xfc\xc5\xcc\xcd\xac\xca\xb1\xb2\xd9\xd7\xf7";
	/* 2. 快捷键设置和记事本一致 */
	string[1]="\xa2\xb2\xbf\xec\xbd\xdd\xbc\xfc\xc9\xe8\xd6\xc3\xba\xcd\xbc\xc7\xca\xc2\xb1\xbe\xd2\xbb\xd6\xc2";
	/* 3. 支持系统输入法输中文 */
	string[2]="\xa2\xb3\xd6\xa7\xb3\xd6\xcf\xb5\xcd\xb3\xca\xe4\xc8\xeb\xb7\xa8\xca\xe4\xd6\xd0\xce\xc4";
	/* 4. 菜单支持点击和方向切换 */
	string[3]="\xa2\xb4\xb2\xcb\xb5\xa5\xd6\xa7\xb3\xd6\xb5\xe3\xbb\xf7\xba\xcd\xb7\xbd\xcf\xf2\xc7\xd0\xbb\xbb";
	/* 5. 一直被模仿，从未被超越 */
	string[4]="\xa2\xb5\xd2\xbb\xd6\xb1\xb1\xbb\xc4\xa3\xb7\xc2\xa3\xac\xb4\xd3\xce\xb4\xb1\xbb\xb3\xac\xd4\xbd";
	for(i=0; i<5; i++)
	{
		Printfhz(275, 52+16*i, string[i], RED, pEdit);
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

	string[0]="\xd6\xc6\xd7\xf7\xc8\xcb\xd4\xb1";
	string[1]="\xea\xcc\xb3\xac\xb3\xac";
	string[2]="\xd0\xa4";
	string[3]="";
	string[4]="\xd2\xbb\xd6\xb1\xb1\xbb\xc4\xa3\xb7\xc2\xa3\xac\xb4\xd3\xce\xb4\xb1\xbb\xb3\xac\xd4\xbd";
	OutputYao(306, 88, RED);
	for(i=0; i<5; i++)
	{
		Printfhz(290, 52+18*i, string[i], RED, pEdit);
	}
	pEdit->editActive=0;
	pEdit->submenu.flag=1;
	pEdit->state.functionActive=1;
}
