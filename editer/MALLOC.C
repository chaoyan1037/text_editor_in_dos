/*****************************************************************************
 * File: MALLOC.C
 * Function: Checked dynamic allocation and zero-initialization of NODE items.
 * Programmer: YanChao (2011.9.12)
 *****************************************************************************/
#include<graphics.h>
#include<stdio.h>
#include<stdlib.h>
#include<conio.h>
#include"h/struct.h"
#include"h/common.h"
#include"h/node.h"
#include"h/hanzi.h"
#include"h/malloc.h"

/*
 * MallocNode - Allocate and initialize a single document NODE.
 *
 * Defaults the new node to a single-byte ASCII placeholder (flag=1,
 * firstInLine=0, empty NUL-terminated payload, detached links).
 * If heap allocation fails on DOS's limited memory model, cleanly tears down
 * graphics mode, releases existing nodes and the HZK16 file handle, and exits
 * with a non-zero error code.
 */
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
