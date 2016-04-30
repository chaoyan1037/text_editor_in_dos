/**********************************************
function: draw submenu, and choose function...

programmer:XiaoYao

time：2011.10.5-2011.10.7
***********************************************/
#include<graphics.h>
#include<bios.h>
#include"h//common.h"
#include"h//hanzi.h"
#include"h//struct.h"
#include"h//drawmenu.h"
#include"h//spell.h"
#include"h//submenus.h"

void SubmenuF1(struct EDITTYPE *pEdit);
void SubmenuF2(struct EDITTYPE *pEdit);
void SubmenuF3(struct EDITTYPE *pEdit);
void SubmenuF4(struct EDITTYPE *pEdit);
void EnterFunction(struct EDITTYPE *pEdit);

void SubmenuF1(struct EDITTYPE *pEdit)
{
    int i;
    long int ascii;
    char *menu[5];
    if(pEdit->submenu.menuflag==0) //0表示子菜单光条移动过或第一次打开子菜单，需画出子菜单
    {
        pEdit->submenu.menuflag=1;     
        setfillstyle(SOLID_FILL, YELLOW);
        bar(5, 50, 85, 200);

        setfillstyle(SOLID_FILL,LIGHTGREEN);
        bar(5, 50+30*(pEdit->submenu.flag-1), 85, 80+30*(pEdit->submenu.flag-1) );

		setcolor(DARKGRAY);
		rectangle(5, 50+30*(pEdit->submenu.flag-1), 85, 80+30*(pEdit->submenu.flag-1) );
		WhiteBlack(5, 50+30*(pEdit->submenu.flag-1), 85, 80+30*(pEdit->submenu.flag-1), WHITE);

        menu[0]="打开";
        menu[1]="新建";
        menu[2]="保存";
        menu[3]="另存为";
        menu[4]="关闭";

        for(i=0; i<5; i++)
        {
            Printfhz( 7, 55+30*i, menu[i], RED, pEdit);
        }
    } 
}  

void SubmenuF2(struct EDITTYPE *pEdit)
{
    int i;
    long int ascii;
    char *menu[4];
    if(pEdit->submenu.menuflag==0)
    {
        pEdit->submenu.menuflag=1;
        setfillstyle(SOLID_FILL, YELLOW);
        bar(95, 50, 175, 170);

        setfillstyle(SOLID_FILL,LIGHTGREEN);
        bar(95, 50+30*(pEdit->submenu.flag-1), 175, 80+30*(pEdit->submenu.flag-1) );

		setcolor(DARKGRAY);
		rectangle(95, 50+30*(pEdit->submenu.flag-1), 175, 80+30*(pEdit->submenu.flag-1) );
		WhiteBlack(95, 50+30*(pEdit->submenu.flag-1), 175, 80+30*(pEdit->submenu.flag-1),WHITE);

        menu[0]="剪切";
        menu[1]="复制";
        menu[2]="粘贴";
		menu[3]="全选";
		
        for(i=0; i<4; i++)
        {
            Printfhz( 97, 55+30*i, menu[i], RED, pEdit);
        }
    }  
}      

void SubmenuF3(struct EDITTYPE *pEdit)
{
    int i;
    long int ascii;
    char *menu[2];
    if(pEdit->submenu.menuflag==0)
    {
		pEdit->submenu.menuflag=1;
		setfillstyle(SOLID_FILL, YELLOW);
		bar(185, 50, 265, 110);
		setfillstyle(SOLID_FILL,LIGHTGREEN);
		bar(185, 50+30*(pEdit->submenu.flag-1), 265, 80+30*(pEdit->submenu.flag-1) );

		setcolor(DARKGRAY);
		rectangle(185, 50+30*(pEdit->submenu.flag-1), 265, 80+30*(pEdit->submenu.flag-1) );
		WhiteBlack(185, 50+30*(pEdit->submenu.flag-1), 265, 80+30*(pEdit->submenu.flag-1), WHITE);

		menu[0]="查找";
		menu[1]="查找替换";

		for(i=0; i<2; i++)
		{
			Printfhz( 187, 55+30*i, menu[i], RED, pEdit);
		}
    }
}

void SubmenuF4(struct EDITTYPE *pEdit)
{
	int i;
	long int ascii;
	char *menu[2];
	if(pEdit->submenu.menuflag==0)
	{
		pEdit->submenu.menuflag=1;
		setfillstyle(SOLID_FILL, YELLOW);
		bar(275, 50, 355, 110);
		setfillstyle(SOLID_FILL, LIGHTGREEN);
		bar(275, 50+30*(pEdit->submenu.flag-1), 355, 80+30*(pEdit->submenu.flag-1));

		setcolor(DARKGRAY);
		rectangle(275, 50+30*(pEdit->submenu.flag-1), 355, 80+30*(pEdit->submenu.flag-1) );
		WhiteBlack(275, 50+30*(pEdit->submenu.flag-1), 355, 80+30*(pEdit->submenu.flag-1), WHITE);

		menu[0]="操作指南";
		menu[1]="说明";
		for(i=0; i<2; i++)
		{
			Printfhz( 277, 55+30*i, menu[i], RED, pEdit);
		}
	}
}
void GetSubmenu(struct EDITTYPE *pEdit)
{
   pEdit->submenu.menuflag=0;/*菜单标志位，为0则需画出菜单*/
   switch(pEdit->state.key)
   {
	   case    F1:    pEdit->editActive=1;
			  break;
	   case    F2:    pEdit->editActive=2;
			  break;
	   case    F3:    pEdit->editActive=3;
			  break;
	   case    F4:    pEdit->editActive=4;
			  break;
   }
}

void Submenu(struct EDITTYPE *pEdit)
{
	switch(pEdit->editActive)
	{
		case 1:	 SubmenuF1( pEdit );
		 	 break;
		case 2:	 SubmenuF2( pEdit );
			 break;
		case 3:	 SubmenuF3( pEdit );
			 break;
		case 4:	 SubmenuF4( pEdit );
			 break;
	}
	if(bioskey(1)!=0)
	{
		pEdit->state.key=bioskey(0);

		switch(pEdit->state.key)
		{
			case UP:    if(pEdit->submenu.flag>1)//子菜单中光条位置
				    {
					pEdit->submenu.flag--;
					pEdit->submenu.menuflag=0;
				    }
					break;
			case DOWN:  if(pEdit->submenu.flag<pEdit->submenu.SubmenuNum[pEdit->editActive-1])
				    {
					pEdit->submenu.flag++;
					pEdit->submenu.menuflag=0;
				    }
					break;

			case ENTER: EnterFunction(pEdit);
				    break;

			case ESC:   ExitSubmenu(pEdit);
				    break;
		}
	}
}
/****************choose shumenu then press ENTER...**********/
void EnterFunction(struct EDITTYPE *pEdit)
{
	switch(pEdit->editActive)
	{
		case 1: switch(pEdit->submenu.flag)
				{
					case 1: pEdit->state.function=10;
							break;
					case 2: pEdit->state.function=14;
							break;
					case 3: pEdit->state.function=9;
							break;
					case 4: pEdit->state.function=15;
							break;
					case 5:	pEdit->state.function=1;
							break;
					default:break;
				}
				ExitSubmenu(pEdit);
				break;  
		case 2: switch(pEdit->submenu.flag)
				{
					case 1: pEdit->state.function=13;
							break;
					case 2: pEdit->state.function=12;
							break;
					case 3: pEdit->state.function=4;
							break;
					case 4: pEdit->state.function=25;
							break;
					default:break;
				}
				ExitSubmenu(pEdit);
				break;
		case 3: switch(pEdit->submenu.flag)
				{
					case 1: pEdit->state.function=5;
							break;
					case 2: pEdit->state.function=21;
							break;
					default:break;
				}
				ExitSubmenu(pEdit);
				break;
		case 4: switch(pEdit->submenu.flag)
				{
					case 1: OperationDirection(pEdit);
							pEdit->state.mouseFlag=0;
							break;
					case 2: Explanation(pEdit);
							pEdit->state.mouseFlag=0;
						break;
					default:break;
				}
				break;
	}
}

void ExitSubmenu(struct EDITTYPE *pEdit)
{
	int temp=pEdit->editActive;
	pEdit->editActive=0;
	pEdit->submenu.flag=1;
	pEdit->editFlag=1;
	settextjustify(LEFT_TEXT,TOP_TEXT);
	switch(temp)
	{
		case 1: DrawMenu1(pEdit);
			break;
		case 2: DrawMenu2(pEdit);
			break;
		case 3: DrawMenu3(pEdit);
			break;
		case 4: DrawMenu4(pEdit);
			break;
	}
	settextjustify(LEFT_TEXT,BOTTOM_TEXT);	
}
/********** Operation Direction***************/
void  OperationDirection(struct EDITTYPE *pEdit)
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
}
