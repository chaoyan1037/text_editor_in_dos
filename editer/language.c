/*****************************************************************************
 * File: LANGUAGE.C
 * Purpose: Single-byte ASCII insertion, newline (ENTER) insertion, and
 *          English/Chinese input mode switching.
 * Programmer: YanChao (2011.9.16-2011.9.18)
 *****************************************************************************/
#include<stdlib.h>
#include"h/struct.h"
#include"h/node.h"
#include"h/common.h"
#include"h/malloc.h"
#include"h/language.h"

/*****************************************************************************
 * Function: GetEnglish
 * Purpose:  Insert the printable ASCII byte stored in `pEdit->state.ascii`
 *           at the active caret position as a `flag = 1` node.
 *
 * @param pEdit Master editor context (`state.ascii` must be in 0x20..0x7E).
 * @post        - Document list has one new node at `pEdit->current`.
 *              - Sets `editFlag = 1` and `cursorFlag = 1` to schedule redraw.
 * @note        Checks `pEdit->current != NULL` before testing `current->next`
 *              so typing at top-left of a non-empty page routes to InsertNode().
 *****************************************************************************/
void GetEnglish(struct EDITTYPE *pEdit)
{
	char data[2]="h";
	pEdit->editFlag=1;
	pEdit->cursorFlag=1;
	data[0]=(char)pEdit->state.ascii;
	data[1]='\0';
	if( pEdit->start==NULL )
	{
		pEdit->start=MallocNode(pEdit);
		pEdit->start->data[0]=data[0];
		pEdit->start->firstInLine=1;
		pEdit->pageFirst=pEdit->last=pEdit->current=pEdit->start;
	}
	else if( pEdit->current!=NULL && pEdit->current->next==NULL )
	{
		AddNode(pEdit, data);
	}
	else
	{
		InsertNode(pEdit, data);
	}
}

/*****************************************************************************
 * Function: GetEnter
 * Purpose:  Insert a newline (`flag = 0`) node at the active caret position.
 *
 * @param pEdit Master editor context.
 * @post        - New `flag = 0` node inserted at `pEdit->current`.
 *              - Sets `editFlag = 1` and `cursorFlag = 1`.
 *              - If the caret was on the bottom visible row (`y == EDITERSCREENY2`),
 *                sets `pageDownFlag = 2` so main() scrolls down by one line.
 *****************************************************************************/
void GetEnter(struct EDITTYPE *pEdit)
{
	char data[2]="h";
	data[0]=(char)pEdit->state.ascii;
	data[1]='\0';
	pEdit->editFlag=1;
	pEdit->cursorFlag=1;
	if( pEdit->start==NULL )
	{
		pEdit->start=MallocNode(pEdit);
		pEdit->start->data[0]=data[0];
		pEdit->start->firstInLine=1;
		pEdit->pageFirst=pEdit->last=pEdit->current=pEdit->start;
	}
	else if( pEdit->current!=NULL && pEdit->current->next==NULL )
	{
		AddNode(pEdit, data);
	}
	else
	{
		InsertNode(pEdit, data);
	}
	pEdit->current->flag=0;

	/* Pressing Enter on row 25 defers a scroll-down step to the next loop pass */
	if(pEdit->y==EDITERSCREENY2)
	{
		pEdit->pageDownFlag=2;
	}
}

/*****************************************************************************
 * Function: ExchangeLanguage
 * Purpose:  Toggle `pEdit->language` between English (0) and Chinese IME (1).
 *
 * @param pEdit Master editor context.
 *****************************************************************************/
void ExchangeLanguage(struct EDITTYPE *pEdit)
{
	if(pEdit->language==1)
	{
		pEdit->language=0;
	}
	else
	{
		pEdit->language=1;
	}
}
