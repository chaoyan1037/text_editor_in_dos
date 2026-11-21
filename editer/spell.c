/*****************************************************************************
 * File: SPELL.C
 * Purpose: Chinese character (GB18030/GB2312) node insertion, bottom-left
 *          input-mode status indicator, and custom '垚' glyph helper.
 *
 * Notes:
 *   - On Linux/POSIX, Chinese characters are entered directly via the host
 *     OS/terminal UTF-8 IME (converted to GB18030 in platform/bgi_tui.c) and
 *     inserted via Gethanzi(), so the legacy DOS `spell/` pinyin dictionary
 *     directory is no longer needed.
 *****************************************************************************/
#include<graphics.h>
#include<stdlib.h>
#include"h/hanzi.h"
#include"h/common.h"
#include"h/struct.h"
#include"h/malloc.h"
#include"h/node.h"
#include"h/mouse.h"
#include"h/spell.h"

/*****************************************************************************
 * Function: Gethanzi
 * Purpose:  Insert the 2-byte GB18030/GB2312 glyph in `pEdit->spell.pcode`
 *           at the active caret position as a `flag = 2` document node.
 *
 * @param pEdit Master editor context with `spell.pcode[0..1]` populated.
 * @post        Inserts node at `pEdit->current` and sets `editFlag=1`,
 *              `cursorFlag=1`. Safe when document is empty or `current==NULL`.
 *****************************************************************************/
void Gethanzi(struct EDITTYPE *pEdit)
{
	char data[3]="hh";
	pEdit->editFlag=1;
	pEdit->cursorFlag=1;
	data[0]=pEdit->spell.pcode[0];
	data[1]=pEdit->spell.pcode[1];
	data[2]='\0';

	if( pEdit->start==NULL )
	{
		pEdit->start=MallocNode(pEdit);
		pEdit->start->data[0]=data[0];
		pEdit->start->data[1]=data[1];
		pEdit->start->flag=2;
		pEdit->start->firstInLine=1;
		pEdit->pageFirst=pEdit->start;
		pEdit->last=pEdit->current=pEdit->start;
	}
	else if( pEdit->current!=NULL && pEdit->current->next==NULL )
	{
		AddNode(pEdit, data);
		pEdit->current->flag=2;
	}
	else
	{
		InsertNode(pEdit, data);
		pEdit->current->flag=2;
	}
}

/*****************************************************************************
 * Function: ShowLanguage
 * Purpose:  Draw the bottom-left "中英" indicator showing that both English
 *           and native OS/terminal UTF-8 Chinese input are active.
 *
 * @param pEdit Master editor context.
 *****************************************************************************/
void ShowLanguage(struct EDITTYPE *pEdit)
{
	setfillstyle(SOLID_FILL, LIGHTGRAY);
	setwritemode(COPY_PUT);
	bar(4, 451, 60, 479);
	setcolor(BLACK);
	rectangle(5, 455, 44, 475);
	WhiteBlack(5, 455, 44, 475, WHITE);
	Printfhz(8, 458, "\xd6\xd0\xd3\xa2", RED, pEdit);
}

/*****************************************************************************
 * Function: OutputYao
 * Purpose:  Render the custom glyph '垚' (U+579A) at (x0, y0) in `color`.
 *
 * @param x0    Left pixel coordinate of the 16x16 glyph cell.
 * @param y0    Top pixel coordinate of the 16x16 glyph cell.
 * @param color BGI palette color index.
 *****************************************************************************/
void OutputYao(int x0, int y0, int color)
{
	bgi_draw_yao_native(x0, y0, color);
}
