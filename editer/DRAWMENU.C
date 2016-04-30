/*********************************************
function: draw mainmenu,submenu,and dialog box

programmer: YanChao

time：2011.9.10-2011.10.29
*********************************************/
#include<graphics.h>
#include<conio.h>
#include"h/struct.h"
#include"h/mouse.h"
#include"h/spell.h"
#include"h/common.h"
#include"h/hanzi.h"
#include"h/drawmenu.h"

void LocateRow(struct EDITTYPE *pEdit, int *pRow);
void LocateLine(struct EDITTYPE *pEdit, int *pLine);

/****************画主界面*****************/
void DrawMainMenu(struct EDITTYPE *pEdit)
{
	int gd=VGA, gm=VGAHI;
	//modify the dir according to your case
	initgraph(&gd,&gm,"C://TC30//BGI");
	cleardevice();
	
	setwritemode(COPY_PUT);
	setfillstyle(SOLID_FILL, LIGHTGRAY);
	bar(0, 0, 639, 479);

	setfillstyle(SOLID_FILL, WHITE);
	bar(EDITERSCREENX1, EDITERSCREENY1, EDITERSCREENX2, EDITERSCREENY2);

	setfillstyle(SOLID_FILL, LIGHTBLUE);
	bar(0, 0, X_MAX, 25);    /*blue rectangle*/
	setcolor(BLACK);
	rectangle(595, 3, 635, 22);
	WhiteBlack(595, 3, 635, 22, WHITE);

	DrawMenu1(pEdit);
	DrawMenu2(pEdit);
	DrawMenu3(pEdit);
	DrawMenu4(pEdit);
	
	Printfhz( 400, 5, "汉化的全屏幕编辑器", YELLOW, pEdit);
	Printfhz( 600, 5, "退出", YELLOW, pEdit);

	ShowLanguage(pEdit);
}
/****************画文件菜单*****************/
void DrawMenu1(struct EDITTYPE *pEdit)
{
	char *menu="文件";
	setwritemode(COPY_PUT);
	setcolor(RED);
	setfillstyle(SOLID_FILL, LIGHTGRAY);
	bar(4, 26, 70, 49);
	Printfhz( 6, 30, menu, RED, pEdit);
	outtextxy(37, 35, "<F1>");
	setcolor(BLACK);
	rectangle(4, 27, 70, 48);
	WhiteBlack(4, 27, 70, 48, WHITE);
}
/****************画编辑菜单*****************/
void DrawMenu2(struct EDITTYPE *pEdit)
{
	char *menu="编辑";
	setwritemode(COPY_PUT);
	setcolor(RED);
	setfillstyle(SOLID_FILL, LIGHTGRAY);
	bar(94, 26, 160, 49);
	Printfhz( 95, 30, menu, RED, pEdit);
	outtextxy(37+90*1, 35, "<F2>");
	setcolor(BLACK);
	rectangle(94, 27, 160, 48);
	WhiteBlack(94, 27, 160, 48, WHITE);
}
/****************画工具菜单*****************/
void DrawMenu3(struct EDITTYPE *pEdit)
{
	char *menu="工具";
	setwritemode(COPY_PUT);
	setcolor(RED);
	setfillstyle(SOLID_FILL, LIGHTGRAY);
	bar(184, 26, 250, 49);
	Printfhz( 185, 30, menu, RED, pEdit);
	outtextxy(37+90*2, 35, "<F3>");
	setcolor(BLACK);
	rectangle(184, 27, 250, 48);
	WhiteBlack(184, 27, 250, 48, WHITE);
}
/****************画帮助菜单*****************/
void DrawMenu4(struct EDITTYPE *pEdit)
{
	char *menu="帮助";
	setwritemode(COPY_PUT);
	setcolor(RED);
	setfillstyle(SOLID_FILL, LIGHTGRAY);
	bar(274, 26, 340, 49);
	Printfhz( 275, 30, menu, RED, pEdit);
	outtextxy(37+90*3, 35, "<F4>");
	setcolor(BLACK);
	rectangle(274, 27, 340, 48);
	WhiteBlack(274, 27, 340, 48, WHITE);
}	
/******************画光标******************/
void DrawCursor(struct EDITTYPE *pEdit)
{
	int x=pEdit->x, y=pEdit->y;
	setcolor(BROWN);
	setwritemode(XOR_PUT);
	if(pEdit->cursorFlag==1)
	{
		pEdit->cursorFlag=0;
		/*查找功能在if语句里就完成了*/
		if( GetNodePosi(pEdit, pEdit->current, &pEdit->x, &pEdit->y)==0 )
		{
			;  /****空语句****/
		}
	}
	x=pEdit->x, y=pEdit->y;
	if(y>=EDITERSCREENY2)
	{
	    y=EDITERSCREENY1+CHARHIGH*CHARINROW;
	}
	line(x,y,x+8,y);
}
/****************画滚动条*****************/
void RollBar(struct EDITTYPE *pEdit)
{
	int currentCount=CHARINROW+1;
	int y=EDITERSCREENY1+14, height=60;
	struct NODE *pNode=pEdit->start;
	
	while(pNode&&pNode!=pEdit->pageFirst)
	{
		if(pNode->firstInLine) /*是否是行首节点*/
		{
			currentCount++;
		}
		pNode=pNode->next;
	}
	if(pEdit->tempCurrentCount!=currentCount||pEdit->tempLineCount!=pEdit->lineCount)
	{
		pEdit->tempCurrentCount=currentCount;
		pEdit->tempLineCount=pEdit->lineCount;		
		if(currentCount>CHARINROW)
		{
			setfillstyle(SOLID_FILL, LIGHTBLUE);
			bar(EDITERSCREENX2-20, EDITERSCREENY1, EDITERSCREENX2, EDITERSCREENY2);

			y+=currentCount-CHARINROW;
			if(y>=EDITERSCREENY2-76)
			{
				static i=0;
				y=EDITERSCREENY2-76+(i/pEdit->tempLineCount);
				height=height-(i/pEdit->tempLineCount);
				if(height>=10)
				{
					i++;
				}
			}
			setfillstyle(SOLID_FILL, LIGHTGREEN);
			bar(EDITERSCREENX2-18, y+2, EDITERSCREENX2-2, y+height);

			setwritemode(COPY_PUT);	
			setcolor(BLACK);
			rectangle(EDITERSCREENX2-18, y+2, EDITERSCREENX2-2, y+height);	

			moveto(EDITERSCREENX2-18, EDITERSCREENY1+14);
			lineto(EDITERSCREENX2-2,  EDITERSCREENY1+14);
			lineto(EDITERSCREENX2-10, EDITERSCREENY1+2);
			lineto(EDITERSCREENX2-18, EDITERSCREENY1+14);
			floodfill(EDITERSCREENX2-10, EDITERSCREENY1+5, BLACK);

			moveto(EDITERSCREENX2-18, EDITERSCREENY2-14);
			lineto(EDITERSCREENX2-2,  EDITERSCREENY2-14);
			lineto(EDITERSCREENX2-10, EDITERSCREENY2-2);
			lineto(EDITERSCREENX2-18, EDITERSCREENY2-14);
			floodfill(EDITERSCREENX2-10, EDITERSCREENY2-5, BLACK);

		}
	}
}
/*********************显示行列********************/
void ShowPosition(struct EDITTYPE *pEdit)
{
	int x0=pEdit->row;
	int y0=pEdit->line;
	int x=0, y=0;
	char line[6]=":hhh", row[4]=":hh";

	LocateRow(pEdit, &x); 
	LocateLine(pEdit, &y);
	if(x0!=x||y0!=y)
	{
		pEdit->row=x;
		pEdit->line=y;
		
		row[1]=x/10+'0';
		row[2]=x%10+'0';
		line[1]=y/1000+'0'; /*行最多可显示9999*/
		line[2]=(y%1000)/100+'0';
		line[3]=((y%1000)%100)/10+'0';
		line[4]=((y%1000)%100)%10+'0';
		if(line[1]-'0'==0)
		{
			line[1]=line[2];
			line[2]=line[3];
			line[3]=line[4];
			line[4]='\0';
			if(line[1]-'0'==0)
			{
				line[1]=line[2];
				line[2]=line[3];
				line[3]='\0';
			}
		}
		else if(line[1]-'0'==0)
		{
			line[1]=line[2];
			line[2]=line[3];
			line[3]='\0';
		}
			
		setfillstyle(SOLID_FILL, LIGHTGRAY);
		bar(401, 451, 639, 479);

		setcolor(RED);
		Printfhz(520, 460, "行", RED, pEdit);
		Printfhz(585, 460, "列", RED, pEdit);
		
		setcolor(RED);
		outtextxy(540, 475, line);	
		outtextxy(605, 475, row);
	}
}
/******************find row*************************/
void LocateRow(struct EDITTYPE *pEdit, int *pRow)
{
	int x=pEdit->x;
	struct NODE *pNode=pEdit->current;
	*pRow=1;
	if(x==EDITERSCREENX1)
	{
		return;
	}
	else
	{
		for( ; pNode->firstInLine==0; pNode=pNode->last)
		{
			if(pNode==NULL)
			{
				*pRow=1;
				return;
			}
			(*pRow)++;
		}
		(*pRow)++;
	}	
}
/******************locata line********************/
void LocateLine(struct EDITTYPE *pEdit, int *pLine)
{
	struct NODE *pNode=pEdit->start;
	int y=pEdit->y;
	y=(y-EDITERSCREENY1)/CHARHIGH;
	while(pNode&&pNode!=pEdit->pageFirst)
	{
		if(pNode->firstInLine) /*是否是行首节点*/
		{
			y++;
		}
		pNode=pNode->next;
	}
	*pLine=y;
}