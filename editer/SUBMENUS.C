/**********************************************
function: draw submenu, and choose function...

programmer:XiaoYao

time：2011.10.5-2011.10.7
***********************************************/
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

void SubmenuF2(struct EDITTYPE *pEdit)
{
	char *menu[4];
	menu[0]="剪切";
	menu[1]="复制";
	menu[2]="粘贴";
	menu[3]="全选";
	DrawSubmenuBox(95, 175, menu, 4, pEdit);
}

void SubmenuF3(struct EDITTYPE *pEdit)
{
	char *menu[2];
	menu[0]="查找";
	menu[1]="查找替换";
	DrawSubmenuBox(185, 265, menu, 2, pEdit);
}

void SubmenuF4(struct EDITTYPE *pEdit)
{
	char *menu[2];
	menu[0]="操作指南";
	menu[1]="说明";
	DrawSubmenuBox(275, 355, menu, 2, pEdit);
}

void GetSubmenu(struct EDITTYPE *pEdit)
{
	pEdit->submenu.menuflag=0; /* 菜单标志位，为0则需画出菜单 */
	switch(pEdit->state.key)
	{
		case F1: pEdit->editActive=1;
				 break;
		case F2: pEdit->editActive=2;
				 break;
		case F3: pEdit->editActive=3;
				 break;
		case F4: pEdit->editActive=4;
				 break;
		default: break;
	}
}

void Submenu(struct EDITTYPE *pEdit)
{
	switch(pEdit->editActive)
	{
		case 1: SubmenuF1(pEdit);
				break;
		case 2: SubmenuF2(pEdit);
				break;
		case 3: SubmenuF3(pEdit);
				break;
		case 4: SubmenuF4(pEdit);
				break;
		default: break;
	}
	if(bioskey(1)!=0)
	{
		pEdit->state.key=bioskey(0);

		switch(pEdit->state.key)
		{
			case UP:
				if(pEdit->submenu.flag>1) /* 子菜单中光条位置 */
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

/****************choose submenu then press ENTER...**********/
void EnterFunction(struct EDITTYPE *pEdit)
{
	switch(pEdit->editActive)
	{
		case 1:
			switch(pEdit->submenu.flag)
			{
				case 1: pEdit->state.function=10; break;
				case 2: pEdit->state.function=14; break;
				case 3: pEdit->state.function=9;  break;
				case 4: pEdit->state.function=15; break;
				case 5: pEdit->state.function=1;  break;
				default: break;
			}
			ExitSubmenu(pEdit);
			break;
		case 2:
			switch(pEdit->submenu.flag)
			{
				case 1: pEdit->state.function=13; break;
				case 2: pEdit->state.function=12; break;
				case 3: pEdit->state.function=4;  break;
				case 4: pEdit->state.function=25; break;
				default: break;
			}
			ExitSubmenu(pEdit);
			break;
		case 3:
			switch(pEdit->submenu.flag)
			{
				case 1: pEdit->state.function=5;  break;
				case 2: pEdit->state.function=21; break;
				default: break;
			}
			ExitSubmenu(pEdit);
			break;
		case 4:
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

/********** Operation Direction***************/
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

/************just for a test...*****************/
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
