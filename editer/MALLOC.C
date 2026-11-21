/**********************************************
function£ºmalloc memory,delete memory and free all.

promagromer:YanChao

time£º2011.9.12-2011.9.12
**********************************************/
#include<graphics.h>
#include<stdio.h>
#include<stdlib.h>
#include<conio.h>
#include"h/struct.h"
#include"h/common.h"
#include"h/node.h"
#include"h/hanzi.h"
#include"h/malloc.h"


/***********Malloc Node and init *******************/
struct NODE *MallocNode(struct EDITTYPE *pEdit)
{
	struct NODE *pNode;
	pNode=(struct NODE *)malloc( sizeof(struct NODE) );
	if(pNode==NULL)
	{
		closegraph();
		printf("no enough memory.\n");
		FreeAllNode(pEdit);
		close_hz(pEdit);
		getch();
		exit(1);
	}

	pNode->last=NULL;
	pNode->next=NULL;
	pNode->flag=1;
	pNode->firstInLine=0;
	pNode->data[0]='\0';
	pNode->data[1]='\0';
	pNode->data[2]='\0';
	return pNode;
}
