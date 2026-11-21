/*****************************************************************************
 * File: DRAWMENU.C
 * Purpose: BGI VGA initialization, window shell rendering, top menu buttons,
 *          XOR underline caret, proportional scrollbar, and status bar.
 * Programmer: YanChao (2011.9.10-2011.10.29)
 *
 * Performance Notes:
 *   - Both RollBar() and ShowPosition() are called on every iteration of the
 *     main event loop. Each maintains change-detection caches inside EDITTYPE
 *     so idle iterations return in O(1) without walking the linked list.
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

/*****************************************************************************
 * Function: DrawMainMenu
 * Purpose:  Initialize VGA 640x480 graphics mode and paint the static editor
 *           frame (title bar, Exit button, F1..F4 buttons, viewport, EN/ZH).
 *
 * @param pEdit Editor context (used to render Hanzi labels and language box).
 * @post        BGI graphics mode is active and COPY_PUT mode is selected.
 *****************************************************************************/
void DrawMainMenu(struct EDITTYPE *pEdit)
{
	int gd=VGA, gm=VGAHI;
	/* Adjust BGI driver directory below if Turbo C 3.0 lives elsewhere */
	initgraph(&gd, &gm, "C://TC30//BGI");
	cleardevice();

	/* Full-screen light-gray chrome background */
	setwritemode(COPY_PUT);
	setfillstyle(SOLID_FILL, LIGHTGRAY);
	bar(0, 0, 639, 479);

	/* White document canvas */
	setfillstyle(SOLID_FILL, WHITE);
	bar(EDITERSCREENX1, EDITERSCREENY1, EDITERSCREENX2, EDITERSCREENY2);

	/* Blue title bar (Y=0..25) and top-right Exit button (X=595..635) */
	setfillstyle(SOLID_FILL, LIGHTBLUE);
	bar(0, 0, X_MAX, 25);
	setcolor(BLACK);
	rectangle(595, 3, 635, 22);
	WhiteBlack(595, 3, 635, 22, WHITE);

	/* Four top-bar menu buttons */
	DrawMenu1(pEdit);
	DrawMenu2(pEdit);
	DrawMenu3(pEdit);
	DrawMenu4(pEdit);

	Printfhz( 400, 5, "汉化的全屏幕编辑器", YELLOW, pEdit);
	Printfhz( 600, 5, "退出", YELLOW, pEdit);

	/* Bottom-left EN/ZH mode button */
	ShowLanguage(pEdit);
}

/* DrawMenu1 - Paint File (<F1>) button at X=4..70, Y=26..49 */
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

/* DrawMenu2 - Paint Edit (<F2>) button at X=94..160, Y=26..49 */
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

/* DrawMenu3 - Paint Tools (<F3>) button at X=184..250, Y=26..49 */
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

/* DrawMenu4 - Paint Help (<F4>) button at X=274..340, Y=26..49 */
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

/*****************************************************************************
 * Function: DrawCursor
 * Purpose:  Recompute caret coordinates when `cursorFlag == 1`, then toggle
 *           an 8-pixel horizontal underline cursor in XOR_PUT mode.
 *
 * @param pEdit Master editor context.
 * @post        Clears `cursorFlag = 0`; leaves BGI write mode in XOR_PUT
 *              (reset to COPY_PUT by subsequent drawing functions).
 *****************************************************************************/
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

/*****************************************************************************
 * Function: RollBar
 * Purpose:  Render the proportional right-hand scrollbar (X=614..634,
 *           Y=50..450) with up/down arrow triangles and scaled thumb.
 *
 * @param pEdit Master editor context.
 * @note        - Fast-paths in O(1) when `(pageFirst, lineCount)` has not
 *                changed since the last render.
 *              - Scales thumb height as `trackHeight * CHARINROW / totalLines`
 *                (clamped to [14, trackHeight]) and thumb position linearly
 *                over `linesBefore / (totalLines - CHARINROW)` using 32-bit
 *                `long` arithmetic to prevent 16-bit overflow or divide-by-0.
 * @complexity  O(1) when idle; O(scrolled prefix) when scrolling/lineCount changes.
 *****************************************************************************/
void RollBar(struct EDITTYPE *pEdit)
{
	int linesBefore=0;
	int currentCount;
	int totalLines;
	int trackTop=EDITERSCREENY1+16;    /* Below 14px top arrow button */
	int trackBottom=EDITERSCREENY2-16; /* Above 14px bottom arrow button */
	int trackHeight=trackBottom-trackTop;
	int thumbHeight, thumbY;
	struct NODE *pNode;

	/* Skip full list walk and BGI redraw if scroll state is unchanged */
	if(pEdit->lastRollPageFirst==pEdit->pageFirst
		&& pEdit->tempLineCount==pEdit->lineCount
		&& pEdit->tempCurrentCount!=0)
	{
		return;
	}

	/* Count visual lines currently scrolled above pageFirst */
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

	/* Update cache before painting */
	pEdit->lastRollPageFirst=pEdit->pageFirst;
	pEdit->tempCurrentCount=currentCount;
	pEdit->tempLineCount=pEdit->lineCount;

	/* Compute thumb size and offset proportionally to document length */
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

	/* Paint track, thumb box, and filled top/bottom arrow triangles */
	setwritemode(COPY_PUT);
	setfillstyle(SOLID_FILL, LIGHTBLUE);
	bar(EDITERSCREENX2-20, EDITERSCREENY1, EDITERSCREENX2, EDITERSCREENY2);

	setfillstyle(SOLID_FILL, LIGHTGREEN);
	bar(EDITERSCREENX2-18, thumbY, EDITERSCREENX2-2, thumbY+thumbHeight);

	setcolor(BLACK);
	rectangle(EDITERSCREENX2-18, thumbY, EDITERSCREENX2-2, thumbY+thumbHeight);

	/* Up-arrow triangle at top of scrollbar (Y=52..64) */
	moveto(EDITERSCREENX2-18, EDITERSCREENY1+14);
	lineto(EDITERSCREENX2-2,  EDITERSCREENY1+14);
	lineto(EDITERSCREENX2-10, EDITERSCREENY1+2);
	lineto(EDITERSCREENX2-18, EDITERSCREENY1+14);
	floodfill(EDITERSCREENX2-10, EDITERSCREENY1+5, BLACK);

	/* Down-arrow triangle at bottom of scrollbar (Y=436..448) */
	moveto(EDITERSCREENX2-18, EDITERSCREENY2-14);
	lineto(EDITERSCREENX2-2,  EDITERSCREENY2-14);
	lineto(EDITERSCREENX2-10, EDITERSCREENY2-2);
	lineto(EDITERSCREENX2-18, EDITERSCREENY2-14);
	floodfill(EDITERSCREENX2-10, EDITERSCREENY2-5, BLACK);
}

/*****************************************************************************
 * Function: ShowPosition
 * Purpose:  Display the 1-based line and column number ("行:N 列:M") in the
 *           bottom-right status bar whenever the caret or viewport moves.
 *
 * @param pEdit Master editor context.
 * @complexity  O(1) when idle (cached on current, pageFirst, x, y).
 *****************************************************************************/
void ShowPosition(struct EDITTYPE *pEdit)
{
	int x0=pEdit->row;
	int y0=pEdit->line;
	int x=0, y=0;
	char line[12], row[8];

	/* Fast path: caret anchor, viewport origin, and pixel coords unchanged */
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

/*****************************************************************************
 * Function: LocateRow
 * Purpose:  Compute the 1-based character column at the caret by counting
 *           nodes backward from `pEdit->current` to the line's firstInLine.
 *
 * @param pEdit Master editor context.
 * @param pRow  Output pointer receiving the 1-based column index (>= 1).
 * @complexity  O(line length) <= CHARINLINE.
 *****************************************************************************/
void LocateRow(struct EDITTYPE *pEdit, int *pRow)
{
	int x=pEdit->x;
	struct NODE *pNode=pEdit->current;
	*pRow=1;
	if(x==EDITERSCREENX1 || pNode==NULL)
	{
		return;
	}
	/* Always test pNode != NULL before reading pNode->firstInLine */
	for( ; pNode!=NULL && pNode->firstInLine==0; pNode=pNode->last)
	{
		(*pRow)++;
	}
	if(pNode==NULL)
	{
		*pRow=1;
		return;
	}
	(*pRow)++; /* Include the firstInLine node itself */
}

/*****************************************************************************
 * Function: LocateLine
 * Purpose:  Compute the 1-based absolute line number of the caret by adding
 *           the viewport row index to the count of lines before `pageFirst`.
 *
 * @param pEdit Master editor context.
 * @param pLine Output pointer receiving the 1-based line number (>= 1).
 * @complexity  O(scrolled prefix).
 *****************************************************************************/
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
