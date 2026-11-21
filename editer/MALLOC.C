/*****************************************************************************
 * File: MALLOC.C
 * Purpose: Checked heap allocation and default initialization for `NODE`.
 * Programmer: YanChao (2011.9.12)
 *
 * Notes:
 *   - Under 16-bit DOS (Turbo C 3.0 small/medium/large models), heap space
 *     is constrained. Centralizing malloc() in MallocNode() guarantees that
 *     out-of-memory failures tear down BGI graphics mode gracefully instead
 *     of leaving the terminal stuck in VGA graphics mode.
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

/*****************************************************************************
 * Function: MallocNode
 * Purpose:  Allocate one `struct NODE` on the heap and initialize all fields
 *           to safe defaults (detached links, flag=1 ASCII, empty payload).
 *
 * @param pEdit Master editor context (used only to clean up on OOM failure).
 * @return      Non-NULL pointer to a freshly initialized `struct NODE`.
 * @post        On allocation failure, closes BGI graphics, frees all nodes
 *              and closes HZK16, waits for a keypress, and terminates via
 *              exit(1) so callers never receive NULL.
 *****************************************************************************/
struct NODE *MallocNode(struct EDITTYPE *pEdit)
{
	struct NODE *pNode;
	pNode=(struct NODE *)malloc( sizeof(struct NODE) );
	if(pNode==NULL)
	{
		/* Restore DOS text mode before printing fatal diagnostic message */
		closegraph();
		printf("no enough memory.\n");
		FreeAllNode(pEdit);
		close_hz(pEdit);
		getch();
		exit(1);
	}

	/* Initialize links and character payload to a clean NUL-terminated state */
	pNode->last=NULL;
	pNode->next=NULL;
	pNode->flag=1;
	pNode->firstInLine=0;
	pNode->data[0]='\0';
	pNode->data[1]='\0';
	pNode->data[2]='\0';
	return pNode;
}
