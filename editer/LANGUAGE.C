/*****************************************************************************
 * File: LANGUAGE.C
 * Function: Insert ASCII characters, insert newline (ENTER) nodes, and toggle
 *           between English (ASCII) and Chinese (Pinyin) input modes.
 * Programmer: YanChao (2011.9.16-2011.9.18)
 *****************************************************************************/
#include<stdlib.h>
#include"h/struct.h"
#include"h/node.h"
#include"h/common.h"
#include"h/malloc.h"
#include"h/language.h"

/*
 * GetEnglish - Insert the printable ASCII character in `pEdit->state.ascii`
 *              at the current caret position (flag = 1).
 *
 * Safely handles empty documents, appending at tail, and inserting when
 * `current == NULL` (before `pageFirst`). Triggers viewport and caret redraws.
 */
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

/*
 * GetEnter - Insert a newline node (flag = 0) at the current caret position.
 *
 * If the caret is already on the bottom visible row (EDITERSCREENY2), sets
 * `pageDownFlag = 2` so the main loop scrolls the viewport down by one line.
 */
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

	if(pEdit->y==EDITERSCREENY2)
	{
		pEdit->pageDownFlag=2;
	}
}

/*
 * ExchangeLanguage - Toggle active input mode between English (0) and Chinese (1).
 */
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
