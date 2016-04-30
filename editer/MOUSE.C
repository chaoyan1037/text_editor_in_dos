/*****************************************************************************************
function：1.read state of mouse when moving mouse then show mouse where it is.
		  2.mouse click and do function
		  
programmer:XiaoYao

time：2011.9.10-2011.11.4
******************************************************************************************/
#include<graphics.h>
#include<stdlib.h>
#include<dos.h>
#include<bios.h>
#include"h/struct.h"
#include"h/common.h"
#include"h/node.h"
#include"h/submenus.h"
#include"h/common.h"
#include"h/save.h"
#include"h/mouse.h"

int InitMouse(void);
void ReadMouse(struct EDITTYPE *pEdit);
void DrawMouse(struct MOUSESTATE state);

void Submenu1Op(struct EDITTYPE *pEdit);
void Submenu2Op(struct EDITTYPE *pEdit);
void Submenu3Op(struct EDITTYPE *pEdit);
void Submenu4Op(struct EDITTYPE *pEdit);
void ChooseSubmenu(struct EDITTYPE *pEdit);

int InitMouse(void)
{
	int retcode;
	union REGS regs;
	int xmin=2, xmax=X_MAX-1;
	int ymin=2, ymax=Y_MAX-1;
	regs.x.ax=0;  /* o mouse function*/
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

void ReadMouse(struct EDITTYPE *pEdit)
{
	union REGS regs;
	pEdit->state.button=0;  /*avoid function work twice*/

	regs.x.ax=3;
	int86(MOUSEID, &regs, &regs);
	pEdit->state.x=regs.x.cx;
	pEdit->state.y=regs.x.dx;
	pEdit->state.button=regs.x.bx;
}

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

void ShowMouse(struct EDITTYPE *pEdit)
{
	struct MOUSESTATE state0;
	state0 = pEdit->state; /*初始鼠标状态*/
	ReadMouse(pEdit);      /*读现在的鼠标状态*/
	setwritemode(XOR_PUT);
	
	if( (state0.x!=pEdit->state.x) || (state0.y!=pEdit->state.y) )
	{
		if(pEdit->state.mouseFlag==1)/*mouseFlag为1时异或画两次鼠标*/
		{
			DrawMouse(state0);
		}
		pEdit->state.mouseFlag=1;
		DrawMouse( pEdit->state );
	}
	setwritemode(COPY_PUT);
}

/*************************************************************/
void MouseKey(struct EDITTYPE *pEdit)
{
	int x=pEdit->state.x;
	int y=pEdit->state.y;
	int b=pEdit->state.button;
	int fa=pEdit->state.functionActive;
	setwritemode(COPY_PUT);
	
	/*************************************/
	if(x<=306&&x>=246&&y>=214&&y<=232)
	{
		/************save before exit*************/
		if(fa==2)
		{
			button3d(246, 214, 306, 232);
			if(b==1)
			{
				Save(pEdit->file.fname, pEdit);
				pEdit->state.functionActive=0;
				delay(100);
				ExitEditer(pEdit);
			}
		}
		/************save*************/
		else if(fa==9)
		{
			button3d(246, 214, 306, 232);
			if(b==1)
			{
				pEdit->state.function=17;
				pEdit->state.functionActive=0;
				delay(100);
			}
		}
		/************open input*************/
		else if(fa==10)
		{
			button3d(246, 214, 306, 232);
			if(b==1)
			{
				pEdit->state.function=18;
				pEdit->state.functionActive=0;
				pEdit->state.mouseFlag=0;
				delay(100);
			}
		}
		/************open save*************/
		else if(fa==11)
		{
			button3d(246, 214, 306, 232);
			if(b==1)
			{
				Save("edit.file.fname", pEdit);
				pEdit->state.function=19;							
				pEdit->state.functionActive=0;
				delay(100);
			}
		}
		/************new save*************/
		else if(fa==12)
		{
			button3d(246, 214, 306, 232);
			if(b==1)
			{
				Save(pEdit->file.fname, pEdit);
				pEdit->state.function=11;							
				pEdit->state.functionActive=0;
				delay(100);
			}
		}
		/************save as*************/
		else if(fa==15)
		{
			button3d(246, 214, 306, 232);
			if(b==1)
			{
				Saveas(pEdit);
				pEdit->state.functionActive=0;
				delay(100);
			}
		}
	}
	/************以下和上面一一对应*************/
	else if(x<=374&&x>=314&&y>=214&&y<=232)
	{
		if(fa==2)
		{
			button3d(314, 214, 374, 232);
			if(b==1)
			{
				pEdit->state.functionActive=0;
				delay(100);
				ExitEditer(pEdit);
			}
		}
		else if(fa==9)
		{
			button3d(314, 214, 374, 232);
			if(b==1)
			{
				pEdit->state.functionActive=0;
				pEdit->editFlag=1;
				delay(100);
			}
		}
		else if(fa==10)
		{
			button3d(314, 214, 374, 232);
			if(b==1)
			{
				pEdit->state.functionActive=0;
				pEdit->editFlag=1;
				delay(100);
			}
		}
		else if(fa==11)
		{
			button3d(314, 214, 374, 232);
			if(b==1)
			{
				pEdit->state.function=19;
				pEdit->state.functionActive=0;
				delay(100);
			}
		}
		else if(fa==12)
		{
			button3d(314, 214, 374, 232);
			if(b==1)
			{
				pEdit->state.function=11;
				pEdit->state.functionActive=0;
				pEdit->state.mouseFlag=0;
				delay(100);
			}
		}
		else if(fa==15)
		{
			button3d(314, 214, 374, 232);
			if(b==1)
			{
				pEdit->state.functionActive=0;
				pEdit->editFlag=1;
				delay(100);
			}
		}
	}
	
	/************sure in find or replace************/
	if(x<=314&&x>=258&&y>=214&&y<=232)
	{
		/************click to replace*************/
		if(fa==7)
		{
			button3d(258, 214, 314, 232);
			if(b==1)
			{
				pEdit->state.function=22;
				delay(100);
			}
		}
		/***********click to find**********/
		else if( fa==5||fa==6 )
		{
			button3d(258, 214, 314, 232);
			if(b==1)
			{
				pEdit->state.function=16;
				if(fa==6)
				{
					pEdit->state.mouseFlag=0;
				}
				delay(200);
			}
		}
		else if(fa==1)
		{
			if(b==1)
			{
				pEdit->state.functionActive=0;
				pEdit->editFlag=1;
				pEdit->state.mouseFlag=0;
				delay(100);
			}
		}
	
	}
	/*******************cancle in find or replace***********************/
	else if(x<=379&&x>=323&&y>=214&&y<=232)
	{
		if(fa==5||fa==6)
		{
			button3d(323, 214, 379, 232);
			if(b==1&&fa==5)
			{
				pEdit->state.function=20; /*cancle find*/
				delay(100); 
			}
			else if(b==1&&fa==6)	
			{
				pEdit->state.function=23;/*cancle find and replace*/
				delay(100); 
			}
		}
		else if(fa==7)
		{
			button3d(323, 214, 379, 232);
			if(b==1)
			{
				pEdit->state.function=23;
				delay(100);
			}
		}
	}
	/***move current edit position****/
	else if(x>=5&&x<=634&&y>=50&&y<=450&&b==1&&pEdit->editActive==0)
	{
		int xTemp=pEdit->state.x;
		int yTemp=pEdit->state.y;
		struct MOUSESTATE tempState=pEdit->state;
		if( ConvertNodePosi(pEdit, x, y)==1 )/***here edit by YanChao***/
		{
			MouseBWChosen(pEdit);
			pEdit->state.x=xTemp;
			pEdit->state.y=yTemp;
			if( pEdit->state.button==0 )
			{
				pEdit->state=tempState;
				pEdit->state.function=2;
				pEdit->state.key=0;       /*clear key*/
			}
		}
		
	}
	/***function 1 is to exit***/
	else if(x<=635&&x>=595&&y>=3&&y<=22)
	{			
		button3d(595, 3, 635, 22);
		if(b==1)
		{
			pEdit->state.function=1;
		}
	}
	/***function 8 is to exchange language***/
	else if(x<=25&&x>=4&&y>=455&&y<=475)
	{
		button3d(5, 455, 28, 475);
		if(b==1)
		{
			pEdit->state.function=8;
			pEdit->state.mouseFlag=0;
			delay(100);
		}
	}
	/***********roll bar up*************/
	if(x<=632&&x>=616&&y>=52&&y<=64&&b==1)
	{
	    pEdit->state.function=2;
		pEdit->state.key=UP;
	}
	/************roll bar down*******/
	if(x<=632&&x>=616&&y>=436&&y<=448&&b==1)
	{
		pEdit->state.function=2;
		pEdit->state.key=DOWN;
	}
	/*******Submen operations*******/
	ChooseSubmenu(pEdit);

}
/***********mouse click or press key to choose submenu***************/
void ChooseSubmenu(struct EDITTYPE *pEdit) 
{
	int x=pEdit->state.x;
	int y=pEdit->state.y;
	int b=pEdit->state.button;
	
	if(x>=5&&x<=634&&y>=50&&y<=450&&pEdit->editActive!=0)
	{
		if(pEdit->editActive==1)
		{		
			Submenu1Op(pEdit);		
		}
		else if(pEdit->editActive==2)
		{
			Submenu2Op(pEdit);
		}
		else if(pEdit->editActive==3)
		{
			Submenu3Op(pEdit);
		}
		else if(pEdit->editActive==4)
		{
			Submenu4Op(pEdit);
		}
	}
	if(x>=5&&x<=70&&y>=25&&y<50&&pEdit->editActive==0)
	{
		button3d(4, 27, 70, 48);
		if(b==1)
		{
			pEdit->state.function=6;
			pEdit->editActive=1;
			pEdit->submenu.flag=1;
			pEdit->state.mouseFlag=1;	
		}
	}
	else if(x>=95&&x<=160&&y>=25&&y<=50&&pEdit->editActive==0)
	{
		button3d(94, 27, 160, 48);	
		if(b==1)
		{
			pEdit->state.function=6;
			pEdit->editActive=2;
			pEdit->submenu.flag=1;
			pEdit->state.mouseFlag=1;
		}
	}
	else if(x>=185&&x<=250&&y>=25&&y<=50&&pEdit->editActive==0)
	{
		button3d(184, 27, 250, 48);		
		if(b==1)
		{
			pEdit->state.function=6;
			pEdit->editActive=3;
			pEdit->submenu.flag=1;
			pEdit->state.mouseFlag=1;
		}
	}
	else if(x>=275&&x<=340&&y>=25&&y<=50&&pEdit->editActive==0)
	{
		button3d(274, 27, 340, 48);		
		if(b==1)
		{
			pEdit->state.function=6;
			pEdit->editActive=4;
			pEdit->submenu.flag=1;
			pEdit->state.mouseFlag=1;
		}
	}
}
void Submenu1Op(struct EDITTYPE *pEdit)
{
	int x=pEdit->state.x;
	int y=pEdit->state.y;
	int b=pEdit->state.button;
	/*open file*/
	if(x>=5&&x<=85&&y>50&&y<=80)
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
		}
	}
	/*new file*/
	else if(x>=5&&x<=85&&y>80&&y<=110)
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
		}
	}
	/*save file*/
	else if(x>=5&&x<=85&&y>110&&y<=140)
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
		}
	}
	/*save file as*/
	else if(x>=5&&x<=85&&y>140&&y<=170)
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
		}
	}
	/*****exit editer*****/
	else if(x>=5&&x<=85&&y>170&&y<=200)
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
		}
	}
	/*****exit zicaidan***/
	else if(b==1)
	{
		pEdit->state.mouseFlag=0;
		ExitSubmenu(pEdit);
	}
}
void Submenu2Op(struct EDITTYPE *pEdit)
{
	int x=pEdit->state.x;
	int y=pEdit->state.y;
	int b=pEdit->state.button;
	/*cut*/
	if(x>=95&&x<=175&&y>50&&y<=80)
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
		}
	}
	/*copy*/
	else if(x>=95&&x<=175&&y>80&&y<=110)
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
		}
	}
	/*paste*/
	else if(x>=95&&x<=175&&y>110&&y<=140)
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
		}
	}
	/*select all*/
	else if(x>=95&&x<=175&&y>140&&y<=170)
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
		}
	}
	/*****exit zicaidan***/
	else if(b==1)
	{
		ExitSubmenu(pEdit);
	}
}
void Submenu3Op(struct EDITTYPE *pEdit)
{
	int x=pEdit->state.x;
	int y=pEdit->state.y;
	int b=pEdit->state.button;
	/*find*/
	if(x>=185&&x<=265&&y>50&&y<=80)
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
		}
	}
	/*find and replace*/
	else if(x>=185&&x<=265&&y>80&&y<=110)
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
		}
	}
	/*****exit zicaidan***/
	else if(b==1)
	{
		ExitSubmenu(pEdit);
	}
}
/***********************************/
void Submenu4Op(struct EDITTYPE *pEdit)
{
	int x=pEdit->state.x;
	int y=pEdit->state.y;
	int b=pEdit->state.button;
	/*makers*/
	if(x>=275&&x<=355&&y>50&&y<=80)
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
		}
	}
	/*shuoming*/
	else if(x>=275&&x<=355&&y>80&&y<=110)
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
		}
	}
	/*****exit zicaidan***/
	else if(b==1)
	{
		ExitSubmenu(pEdit);
	}
}

/****************show dialog3d******************/
void WhiteBlack(int x0, int y0, int x1, int y1, int color)
{
	setcolor(color);
	setwritemode(COPY_PUT);
	line(x0, y0, x1, y0);
	line(x0, y0, x0, y1);
}
/******************make a 3d button************************/
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
