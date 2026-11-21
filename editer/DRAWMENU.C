/*****************************************************************************
 * File: DRAWMENU.C
 * Function: Initialize VGA graphics and draw main window chrome, top menu
 *           bar buttons, XOR text caret, proportional vertical scrollbar,
 *           and 1-based line/column status bar readout.
 * Programmer: YanChao (2011.9.10-2011.10.29)
 *****************************************************************************/
#include<graphics.h>
#include<conio.h>
#include<stdio.h>
#include"h/struct.h"
#include"h/mouse.h"
#include"h/spell.h"
#include"h/common.h"
#include"h/hanzi.h"
#include"h/node.h"
#include"h/drawmenu.h"

void LocateRow(struct EDITTYPE *pEdit, int *pRow);
void LocateLine(struct EDITTYPE *pEdit, int *pLine);

/*
 * DrawMainMenu - Enter 640x480 VGA BGI graphics mode and draw the initial
 *                editor shell (title bar, menu buttons, white viewport,
 *                Exit button, and EN/ZH input mode toggle).
 */
void DrawMainMenu(struct EDITTYPE *pEdit)
{
	int gd=VGA, gm=VGAHI;
	/* Modify the BGI driver path below if TC 3.0 is installed elsewhere */
	initgraph(&gd, &gm, "C://TC30//BGI");
	cleardevice();

	/* Window background frame */
	setwritemode(COPY_PUT);
	setfillstyle(SOLID_FILL, LIGHTGRAY);
	bar(0, 0, 639, 479);

	/* Text editing canvas */
	setfillstyle(SOLID_FILL, WHITE);
	bar(EDITERSCREENX1, EDITERSCREENY1, EDITERSCREENX2, EDITERSCREENY2);

	/* Top title bar and Exit button */
	setfillstyle(SOLID_FILL, LIGHTBLUE);
	bar(0, 0, X_MAX, 25);
	setcolor(BLACK);
	rectangle(595, 3, 635, 22);
	WhiteBlack(595, 3, 635, 22, WHITE);

	/* Four top-level menu buttons: File, Edit, Tools, Help */
	DrawMenu1(pEdit);
	DrawMenu2(pEdit);
	DrawMenu3(pEdit);
	DrawMenu4(pEdit);

	Printfhz( 400, 5, "汉化的全屏幕编辑器", YELLOW, pEdit);
	Printfhz( 600, 5, "退出", YELLOW, pEdit);

	ShowLanguage(pEdit);
}

/* DrawMenu1 - Render the File (<F1>) top-bar menu button */
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

/* DrawMenu2 - Render the Edit (<F2>) top-bar menu button */
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

/* DrawMenu3 - Render the Tools (<F3>) top-bar menu button */
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

/* DrawMenu4 - Render the Help (<F4>) top-bar menu button */
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

/*
 * DrawCursor - Recompute caret coordinates when `cursorFlag == 1` and toggle
 *              the 8px underline cursor in XOR mode so it blinks without
 *              destroying underlying pixels.
 */
void DrawCursor(struct EDITTYPE *pEdit)
{
	int x=pEdit->x, y=pEdit->y;
	setcolor(BROWN);
	setwritemode(XOR_PUT);
	if(pEdit->cursorFlag==1)
	{
		pEdit->cursorFlag=0;
		GetNodePosi(pEdit, pEdit->current, &pEdit->x, &pEdit->y);
	}
	x=pEdit->x;
	y=pEdit->y;
	if(y>=EDITERSCREENY2)
	{
		y=EDITERSCREENY1+CHARHIGH*CHARINROW;
	}
	line(x, y, x+8, y);
}

/*
 * RollBar - Draw the proportional right-hand scrollbar track, thumb, and
 *           up/down arrow triangles.
 *
 * Caches `(pageFirst, lineCount)` so idle iterations of the main loop return
 * immediately without traversing the linked list. Uses 32-bit `long` math to
 * compute thumb height and offset safely without 16-bit overflow or divide-by-0.
 */
void RollBar(struct EDITTYPE *pEdit)
{
	int linesBefore=0;
	int currentCount;
	int totalLines;
	int trackTop=EDITERSCREENY1+16;
	int trackBottom=EDITERSCREENY2-16;
	int trackHeight=trackBottom-trackTop;
	int thumbHeight, thumbY;
	struct NODE *pNode;

	/* Fast path: nothing scrolled and line count unchanged */
	if(pEdit->lastRollPageFirst==pEdit->pageFirst
		&& pEdit->tempLineCount==pEdit->lineCount
		&& pEdit->tempCurrentCount!=0)
	{
		return;
	}

	/* Count visual lines scrolled off the top before pageFirst */
	pNode=pEdit->start;
	while(pNode!=NULL && pNode!=pEdit->pageFirst)
	{
		if(pNode->firstInLine)
		{
			linesBefore++;
		}
		pNode=pNode->next;
	}
	currentCount=linesBefore+CHARINROW+1;

	pEdit->lastRollPageFirst=pEdit->pageFirst;
	pEdit->tempCurrentCount=currentCount;
	pEdit->tempLineCount=pEdit->lineCount;

	/* Compute proportional thumb size and vertical offset within track */
	totalLines=linesBefore+pEdit->lineCount+1;
	if(totalLines<CHARINROW)
	{
		thumbHeight=60;
		thumbY=trackTop;
	}
	else
	{
		int maxScroll=totalLines-CHARINROW;
		thumbHeight=(int)((long)trackHeight*(long)CHARINROW/(long)totalLines);
		if(thumbHeight<14)
		{
			thumbHeight=14;
		}
		if(thumbHeight>trackHeight)
		{
			thumbHeight=trackHeight;
		}
		if(maxScroll>0)
		{
			thumbY=trackTop+(int)((long)linesBefore*(long)(trackHeight-thumbHeight)/(long)maxScroll);
		}
		else
		{
			thumbY=trackTop;
		}
		if(thumbY<trackTop)
		{
			thumbY=trackTop;
		}
		if(thumbY+thumbHeight>trackBottom)
		{
			thumbY=trackBottom-thumbHeight;
		}
	}

	/* Draw track background, thumb rectangle, and top/bottom scroll arrows */
	setwritemode(COPY_PUT);
	setfillstyle(SOLID_FILL, LIGHTBLUE);
	bar(EDITERSCREENX2-20, EDITERSCREENY1, EDITERSCREENX2, EDITERSCREENY2);

	setfillstyle(SOLID_FILL, LIGHTGREEN);
	bar(EDITERSCREENX2-18, thumbY, EDITERSCREENX2-2, thumbY+thumbHeight);

	setcolor(BLACK);
	rectangle(EDITERSCREENX2-18, thumbY, EDITERSCREENX2-2, thumbY+thumbHeight);

	/* Up-arrow button triangle */
	moveto(EDITERSCREENX2-18, EDITERSCREENY1+14);
	lineto(EDITERSCREENX2-2,  EDITERSCREENY1+14);
	lineto(EDITERSCREENX2-10, EDITERSCREENY1+2);
	lineto(EDITERSCREENX2-18, EDITERSCREENY1+14);
	floodfill(EDITERSCREENX2-10, EDITERSCREENY1+5, BLACK);

	/* Down-arrow button triangle */
	moveto(EDITERSCREENX2-18, EDITERSCREENY2-14);
	lineto(EDITERSCREENX2-2,  EDITERSCREENY2-14);
	lineto(EDITERSCREENX2-10, EDITERSCREENY2-2);
	lineto(EDITERSCREENX2-18, EDITERSCREENY2-14);
	floodfill(EDITERSCREENX2-10, EDITERSCREENY2-5, BLACK);
}

/*
 * ShowPosition - Update the bottom-right "行:N 列:M" status bar indicator.
 *
 * Uses change-detection caching on `(current, pageFirst, x, y)` so idle loop
 * iterations skip linked-list scans completely.
 */
void ShowPosition(struct EDITTYPE *pEdit)
{
	int x0=pEdit->row;
	int y0=pEdit->line;
	int x=0, y=0;
	char line[12], row[8];

	if(pEdit->lastPosCurrent==pEdit->current
		&& pEdit->lastPosPageFirst==pEdit->pageFirst
		&& pEdit->lastPosX==pEdit->x
		&& pEdit->lastPosY==pEdit->y)
	{
		return;
	}
	pEdit->lastPosCurrent=pEdit->current;
	pEdit->lastPosPageFirst=pEdit->pageFirst;
	pEdit->lastPosX=pEdit->x;
	pEdit->lastPosY=pEdit->y;

	LocateRow(pEdit, &x);
	LocateLine(pEdit, &y);
	if(x0!=x || y0!=y)
	{
		pEdit->row=x;
		pEdit->line=y;

		sprintf(line, ":%d", y);
		sprintf(row, ":%d", x);

		setwritemode(COPY_PUT);
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

/*
 * LocateRow - Compute the 1-based character column number at the caret
 *             by walking backward from `current` to the line's firstInLine node.
 */
void LocateRow(struct EDITTYPE *pEdit, int *pRow)
{
	int x=pEdit->x;
	struct NODE *pNode=pEdit->current;
	*pRow=1;
	if(x==EDITERSCREENX1 || pNode==NULL)
	{
		return;
	}
	for( ; pNode!=NULL && pNode->firstInLine==0; pNode=pNode->last)
	{
		(*pRow)++;
	}
	if(pNode==NULL)
	{
		*pRow=1;
		return;
	}
	(*pRow)++;
}

/*
 * LocateLine - Compute the 1-based absolute document line number of the caret
 *              by combining the viewport row index with scrolled-off lines.
 */
void LocateLine(struct EDITTYPE *pEdit, int *pLine)
{
	struct NODE *pNode=pEdit->start;
	int y=(pEdit->y-EDITERSCREENY1)/CHARHIGH;
	if(y<1)
	{
		y=1;
	}
	while(pNode!=NULL && pNode!=pEdit->pageFirst)
	{
		if(pNode->firstInLine)
		{
			y++;
		}
		pNode=pNode->next;
	}
	*pLine=y;
}
