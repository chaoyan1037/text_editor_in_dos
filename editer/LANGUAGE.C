/**********************************************
function£ºget in a char or ENTER

programmer:YanChao

time£º2011.9.16-2011.9.18
***********************************************/
#include<stdlib.h>
#include"h/language.h"
#include"h/struct.h"
#include"h/node.h"
#include"h/common.h"
#include"h/malloc.h"
#include"h/language.h"

/********************Get char, number,etc***********************/
void GetEnglish(struct EDITTYPE *pEdit)
{
    char data[2]="h";
    pEdit->editFlag=1;
    pEdit->cursorFlag=1;
    data[0]=pEdit->state.ascii;
    if( pEdit->start==NULL )
    {
        pEdit->start=MallocNode(pEdit);
        pEdit->start->data[0]=data[0];
	pEdit->start->firstInLine=1;
        pEdit->pageFirst=pEdit->last=pEdit->current=pEdit->start;
    }
    else if( (pEdit->pageFirst!=NULL)&&(pEdit->current->next==NULL) )
    {
        AddNode( pEdit, data);
    }
    else
    {
	    InsertNode( pEdit, data);
    }
}
/******************get ENTER key***************************/
void GetEnter(struct EDITTYPE *pEdit)
{
	char data[2]="h";
	data[0]=pEdit->state.ascii;
	pEdit->editFlag=1;
	pEdit->cursorFlag=1;
	if( pEdit->start==NULL )
	{
		pEdit->start=MallocNode(pEdit);
		pEdit->start->data[0]=data[0];
		pEdit->start->firstInLine=1;
		pEdit->pageFirst=pEdit->last=pEdit->current=pEdit->start;
	}
	else if( (pEdit->start!=NULL)&&(pEdit->current->next==NULL) )
	{
		AddNode(pEdit, data);
	}
	else
	{
		InsertNode( pEdit, data);
	}
	pEdit->current->flag=0;

	if(pEdit->y==EDITERSCREENY2)           
	{                                   
		pEdit->pageDownFlag=2;                
	}					     
}
/*************Exchange Language*************/
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
