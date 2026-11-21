/*****************************************************************************
 * File: NODE.C
 * Purpose: Doubly-linked document list primitives, clipboard deep-copy/free
 *          routines, and O(page) viewport coordinate <-> node mapping.
 * Programmer: YanChao (2011.9.11-2011.9.15)
 *
 * Core Data-Structure Invariants:
 *   1. Endpoints: If `start == NULL`, then `last == current == pageFirst == NULL`.
 *      Otherwise `start->last == NULL` and `last->next == NULL`.
 *   2. Caret Semantics: `current == NULL` means "immediately before `pageFirst`".
 *      When scrolled down (`pageFirst != start`), inserting while `current == NULL`
 *      MUST splice the new node between `pageFirst->last` and `pageFirst` so
 *      the off-screen prefix of the document is never severed.
 *   3. Layout Consistency: GetNodePosi(), ConvertNodePosi(),
 *      MoveCursorVertical(), and ViewAll() all wrap onto the next visual line
 *      under the exact same rule:
 *        - `flag == 0` (ENTER): `x = EDITERSCREENX1; y += CHARHIGH;`
 *        - `flag == 1 or 2`:    `x += flag * CHARSIZE;`
 *                               if `x >= CHARINLINE*CHARSIZE + EDITERSCREENX1` (605)
 *                               then `x = EDITERSCREENX1; y += CHARHIGH;`
 *****************************************************************************/

#include<graphics.h>
#include<stdlib.h>
#include"h/malloc.h"
#include"h/common.h"
#include"h/struct.h"
#include"h/node.h"

/*****************************************************************************
 * Function: AddNode
 * Purpose:  Allocate a node with payload `data[0..1]` and append it right
 *           after `pEdit->current` (typically at the document tail).
 *
 * @param pEdit Editor context whose document list is mutated.
 * @param data  At least 2 bytes of payload (`data[1]` is '\0' for ASCII).
 * @post        `pEdit->current` points to the newly appended node; `pEdit->last`
 *              is updated if the new node has no successor.
 * @complexity  O(1).
 *****************************************************************************/
void AddNode( struct EDITTYPE *pEdit, char data[] )
{
	struct NODE *pNode=MallocNode(pEdit);
	pNode->data[0]=data[0];
	pNode->data[1]=data[1];
	pNode->data[2]='\0';

	if(pEdit->start==NULL)
	{
		/* Empty document: new node becomes the sole head/tail/pageFirst/current */
		pNode->firstInLine=1;
		pEdit->start=pEdit->pageFirst=pEdit->last=pEdit->current=pNode;
		return;
	}
	if(pEdit->current==NULL)
	{
		/* Safety fallback if caller invoked AddNode with current==NULL */
		pNode->last=pEdit->last;
		pEdit->last->next=pNode;
		pEdit->last=pEdit->current=pNode;
		return;
	}
	pNode->last=pEdit->current;
	pNode->next=pEdit->current->next;
	if(pEdit->current->next!=NULL)
	{
		pEdit->current->next->last=pNode;
	}
	else
	{
		pEdit->last=pNode;
	}
	pEdit->current->next=pNode;
	pEdit->current=pNode;
}

/*****************************************************************************
 * Function: InsertNode
 * Purpose:  Insert a new node at the active caret position (empty document,
 *           mid-document, tail, or before `pageFirst` when `current == NULL`).
 *
 * @param pEdit Editor context whose document list is mutated.
 * @param data  2-byte payload copied into the new node.
 * @post        `pEdit->current` advances to the inserted node. When inserted
 *              before a scrolled `pageFirst`, `pageFirst->last->next` is
 *              updated so the doubly-linked list remains unbroken.
 * @complexity  O(1).
 *****************************************************************************/
void InsertNode( struct EDITTYPE *pEdit, char data[] )
{
	struct NODE *pNode=MallocNode(pEdit);
	pNode->data[0]=data[0];
	pNode->data[1]=data[1];
	pNode->data[2]='\0';

	if(pEdit->start==NULL)
	{
		pNode->firstInLine=1;
		pEdit->start=pEdit->pageFirst=pEdit->last=pEdit->current=pNode;
		return;
	}

	if(pEdit->current!=NULL)
	{
		/* Splice immediately after `current` (handles both interior and tail) */
		pNode->next=pEdit->current->next;
		pNode->last=pEdit->current;
		if(pEdit->current->next!=NULL)
		{
			pEdit->current->next->last=pNode;
		}
		else
		{
			pEdit->last=pNode;
		}
		pEdit->current->next=pNode;
		pEdit->current=pNode;
	}
	else
	{
		/* Caret sits at top-left of viewport: insert immediately BEFORE pageFirst */
		struct NODE *pPrev=NULL;
		if(pEdit->pageFirst!=NULL)
		{
			pPrev=pEdit->pageFirst->last;
			pNode->next=pEdit->pageFirst;
			pNode->last=pPrev;
			pEdit->pageFirst->last=pNode;
			pEdit->pageFirst->firstInLine=0;
			if(pPrev!=NULL)
			{
				/* Preserve link to off-screen lines above pageFirst */
				pPrev->next=pNode;
			}
			else
			{
				/* Inserted at the very beginning of the document */
				pEdit->start=pNode;
			}
			pNode->firstInLine=1;
			pEdit->pageFirst=pNode;
			pEdit->current=pNode;
		}
		else
		{
			pNode->firstInLine=1;
			pEdit->start=pEdit->pageFirst=pEdit->last=pEdit->current=pNode;
		}
	}
}

/*****************************************************************************
 * Function: DeleteNode
 * Purpose:  Unlink and free a single node from the document while maintaining
 *           every list, viewport, caret, selection, and find-match invariant.
 *
 * @param pNode Node to remove and free (no-op if NULL).
 * @param pEdit Master editor context.
 * @post        - `pNode` is freed; neither `start`, `last`, `pageFirst`,
 *                `current`, live `clipStart`/`clipLast`, nor `findStart`/
 *                `findLast` can dangle to `pNode`.
 *              - If the sole node was deleted, all document pointers become NULL.
 *              - If a scrolled `pageFirst` was the final node on screen,
 *                `pageFirst` walks back to the start of the preceding visual
 *                line so the viewport does not turn blank.
 * @complexity  O(1) normally; O(line length) only when deleting the sole node
 *              on a scrolled page.
 *****************************************************************************/
void DeleteNode( struct NODE *pNode, struct EDITTYPE *pEdit )
{
	struct NODE *pPrev;
	struct NODE *pNext;
	if(pNode==NULL)
	{
		return;
	}

	/* Snapshot neighbors BEFORE free(pNode) to eliminate use-after-free */
	pPrev=pNode->last;
	pNext=pNode->next;

	/* Relink predecessor -> successor (or advance document head) */
	if(pPrev==NULL)
	{
		pEdit->start=pNext;
	}
	else
	{
		pPrev->next=pNext;
	}

	/* Relink successor -> predecessor (or retreat document tail) */
	if(pNext==NULL)
	{
		pEdit->last=pPrev;
	}
	else
	{
		pNext->last=pPrev;
	}

	if(pEdit->start!=NULL)
	{
		pEdit->start->firstInLine=1;
	}

	/* Repair pageFirst if the top-left visible node was deleted */
	if(pEdit->pageFirst==pNode)
	{
		if(pNext!=NULL)
		{
			pEdit->pageFirst=pNext;
			pEdit->pageFirst->firstInLine=1;
		}
		else if(pPrev!=NULL)
		{
			/* Scrolled past start and deleted the very last node in document:
			 * step pageFirst back to the beginning of the previous line. */
			struct NODE *pLine=pPrev;
			while(pLine->last!=NULL && pLine->firstInLine==0)
			{
				pLine=pLine->last;
			}
			pEdit->pageFirst=pLine;
		}
		else
		{
			pEdit->pageFirst=NULL;
		}
	}

	/* Update caret ONLY if the deleted node was the current caret anchor */
	if(pEdit->current==pNode)
	{
		if(pPrev==NULL || (pEdit->pageFirst!=NULL && pEdit->pageFirst->last==pPrev))
		{
			/* Caret has retreated to top-left of current page */
			pEdit->current=NULL;
		}
		else
		{
			pEdit->current=pPrev;
		}
	}

	/* Invalidate non-owning selection or find-match pointers referencing pNode */
	if(pEdit->copyCutFlag==0 && (pEdit->clipStart==pNode || pEdit->clipLast==pNode))
	{
		pEdit->clipStart=NULL;
		pEdit->clipLast=NULL;
	}
	if(pEdit->findStart==pNode || pEdit->findLast==pNode)
	{
		pEdit->findStart=NULL;
		pEdit->findLast=NULL;
	}

	free(pNode);
}

/*****************************************************************************
 * Function: FreeAllNode
 * Purpose:  Free every node in the document list plus any owned detached
 *           clipboard (`copyCutFlag == 1`), and reset all list/caret fields.
 *
 * @param pEdit Master editor context.
 * @post        All document, clipboard, and find pointers are NULL; flags and
 *              caret coordinates are reset to empty-document defaults.
 * @complexity  O(N + C) where N = document nodes, C = detached clipboard nodes.
 *****************************************************************************/
void FreeAllNode( struct EDITTYPE *pEdit )
{
	struct NODE *pNode=pEdit->start;
	struct NODE *pNext;
	while(pNode!=NULL)
	{
		pNext=pNode->next;
		free(pNode);
		pNode=pNext;
	}

	/* Only free clipboard nodes if they are detached from the main list */
	if(pEdit->copyCutFlag==1)
	{
		ClearClip(pEdit);
	}
	else
	{
		pEdit->clipStart=NULL;
		pEdit->clipLast=NULL;
	}
	pEdit->start=NULL;
	pEdit->last=NULL;
	pEdit->current=NULL;
	pEdit->pageFirst=NULL;
	pEdit->findStart=NULL;
	pEdit->findLast=NULL;
	pEdit->copyActive=0;
	pEdit->copyCutFlag=0;
	pEdit->clipFlag=0;
	pEdit->lineCount=0;
	pEdit->x=EDITERSCREENX1;
	pEdit->y=EDITERSCREENY1+CHARHIGH;
}

/*****************************************************************************
 * Function: ClearClip
 * Purpose:  Free every heap node belonging to a detached clipboard list.
 *
 * @param pEdit Editor context with detached `clipStart..clipLast`.
 * @pre         Must ONLY be called when `copyCutFlag == 1` (detached nodes).
 * @post        `clipStart == clipLast == NULL` and `copyCutFlag == 0`.
 * @complexity  O(C) where C = number of nodes in the clipboard.
 *****************************************************************************/
void ClearClip(struct EDITTYPE *pEdit)
{
	struct NODE *pNode=pEdit->clipStart;
	struct NODE *pNext;
	while(pNode!=NULL)
	{
		/* Save successor before freeing, stopping cleanly after clipLast */
		pNext=(pNode==pEdit->clipLast) ? NULL : pNode->next;
		free(pNode);
		pNode=pNext;
	}
	pEdit->clipStart=NULL;
	pEdit->clipLast=NULL;
	pEdit->copyCutFlag=0;
}

/*****************************************************************************
 * Function: CopySelection
 * Purpose:  Implement Ctrl+C by deep-copying the live selection range
 *           (`clipStart..clipLast`) into a detached heap-allocated list.
 *
 * @param pEdit Editor context with an active selection (`copyCutFlag == 0`).
 * @post        - `clipStart..clipLast` points to the new independent copy.
 *              - `copyCutFlag = 1`, `copyActive = 1`, `clipFlag = 0`.
 *              - Repeated Ctrl+C presses while `copyCutFlag == 1` are idempotent.
 * @note        Deep-copying isolates the clipboard from subsequent edits or
 *              deletions and prevents self-paste infinite loops in Paste().
 * @complexity  O(S) time and space where S = number of selected nodes.
 *****************************************************************************/
void CopySelection(struct EDITTYPE *pEdit)
{
	struct NODE *pSrc;
	struct NODE *newStart=NULL;
	struct NODE *newLast=NULL;

	if(pEdit->copyCutFlag==1)
	{
		/* Clipboard already owns a detached list; keep it active */
		if(pEdit->clipStart!=NULL)
		{
			pEdit->copyActive=1;
			pEdit->cursorFlag=1;
		}
		return;
	}
	if(pEdit->clipStart==NULL || pEdit->clipLast==NULL)
	{
		return;
	}

	/* Clone each node in [clipStart .. clipLast] into a standalone list */
	pSrc=pEdit->clipStart;
	while(pSrc!=NULL)
	{
		struct NODE *pCopy=MallocNode(pEdit);
		pCopy->flag=pSrc->flag;
		pCopy->data[0]=pSrc->data[0];
		pCopy->data[1]=pSrc->data[1];
		pCopy->data[2]='\0';
		pCopy->last=newLast;
		pCopy->next=NULL;
		if(newLast!=NULL)
		{
			newLast->next=pCopy;
		}
		else
		{
			newStart=pCopy;
		}
		newLast=pCopy;
		if(pSrc==pEdit->clipLast)
		{
			break;
		}
		pSrc=pSrc->next;
	}

	pEdit->clipStart=newStart;
	pEdit->clipLast=newLast;
	pEdit->copyActive=1;
	pEdit->copyCutFlag=1;
	pEdit->clipFlag=0;
	pEdit->cursorFlag=1;
}

/*****************************************************************************
 * Function: GetNodePosi
 * Purpose:  Compute the screen caret coordinate immediately AFTER `pGoalNode`.
 *
 * @param pEdit     Editor context providing `pageFirst`.
 * @param pGoalNode Target node whose trailing caret position is requested.
 * @param xx        Output pointer for pixel X coordinate.
 * @param yy        Output pointer for baseline pixel Y coordinate.
 * @return          1 if `pGoalNode` was reached from `pageFirst`;
 *                  0 if `pageFirst` or `pGoalNode` is NULL (sets top-left)
 *                  or `pGoalNode` is not reachable from `pageFirst`.
 * @complexity      O(D) where D = node distance from `pageFirst` to `pGoalNode`.
 *****************************************************************************/
int GetNodePosi( struct EDITTYPE *pEdit, struct NODE *pGoalNode, int *xx, int *yy)
{
	struct NODE *pNode=pEdit->pageFirst;
	int x=EDITERSCREENX1;
	int y=EDITERSCREENY1+CHARHIGH;
	const int maxX=CHARINLINE*CHARSIZE+EDITERSCREENX1;

	if(pNode==NULL || pGoalNode==NULL)
	{
		*xx=EDITERSCREENX1;
		*yy=EDITERSCREENY1+CHARHIGH;
		return 0;
	}

	while(pNode!=NULL)
	{
		if(pNode->flag==0)
		{
			x=EDITERSCREENX1;
			y+=CHARHIGH;
		}
		else
		{
			x+=pNode->flag*CHARSIZE;
			if(x>=maxX)
			{
				x=EDITERSCREENX1;
				y+=CHARHIGH;
			}
		}
		if(y>EDITERSCREENY2)
		{
			y=EDITERSCREENY2;
		}
		if(pNode==pGoalNode)
		{
			*xx=x;
			*yy=y;
			return 1;
		}
		pNode=pNode->next;
	}
	return 0;
}

/*****************************************************************************
 * Function: ConvertNodePosi
 * Purpose:  Map an exact mouse click at pixel (x, y) to the corresponding
 *           document caret position (`pEdit->current`).
 *
 * @param pEdit Editor context whose `current` pointer is updated on hit.
 * @param x     Click pixel X coordinate.
 * @param y     Click pixel Y coordinate.
 * @return      1 if (x, y) landed on a character cell (or immediately after
 *              the final character of the document); 0 otherwise.
 * @note        Uses a single O(page) walk starting at `pageFirst` and stops
 *              as soon as `curY > targetY` (at most 25 visual rows).
 *              Clicking `pageFirst` sets `current = NULL` (not `pageFirst->last`)
 *              so scrolled pages keep the caret visible on row 1 col 1.
 * @complexity  O(page) (~<= 1875 nodes max), independent of total file size.
 *****************************************************************************/
int ConvertNodePosi(struct EDITTYPE *pEdit, int x, int y)
{
	int targetY, targetColX;
	int curX=EDITERSCREENX1;
	int curY=EDITERSCREENY1+CHARHIGH;
	const int maxX=CHARINLINE*CHARSIZE+EDITERSCREENX1;
	struct NODE *pNode=pEdit->pageFirst;

	if(pNode==NULL)
	{
		return 0;
	}
	if(y<=EDITERSCREENY1 || y>EDITERSCREENY2 || x<EDITERSCREENX1 || x>EDITERSCREENX2)
	{
		return 0;
	}

	/* Quantize pixel click to visual row baseline and 8px column left edge */
	targetY=EDITERSCREENY1+(1+(y-1-EDITERSCREENY1)/CHARHIGH)*CHARHIGH;
	targetColX=EDITERSCREENX1+((x-EDITERSCREENX1)/CHARSIZE)*CHARSIZE;

	while(pNode!=NULL && curY<=targetY)
	{
		/* ENTER occupies an 8px clickable slot at line end; Hanzi occupies 16px */
		int width=(pNode->flag==2) ? (2*CHARSIZE) : CHARSIZE;
		if(curY==targetY)
		{
			if(targetColX>=curX && targetColX<curX+width)
			{
				pEdit->current=(pNode==pEdit->pageFirst) ? NULL : pNode->last;
				return 1;
			}
		}

		if(pNode->flag==0)
		{
			curX=EDITERSCREENX1;
			curY+=CHARHIGH;
		}
		else
		{
			curX+=pNode->flag*CHARSIZE;
			if(curX>=maxX)
			{
				curX=EDITERSCREENX1;
				curY+=CHARHIGH;
			}
		}
		pNode=pNode->next;
	}

	/* Also accept clicks directly after the final character on the last line */
	if(pNode==NULL && curY==targetY && targetColX==curX && pEdit->last!=NULL)
	{
		pEdit->current=pEdit->last;
		return 1;
	}

	return 0;
}

/*****************************************************************************
 * Function: MoveCursorVertical
 * Purpose:  Move caret vertically onto visual row `y` at the column closest
 *           to `x` in a single O(page) pass (used by UP and DOWN keys).
 *
 * @param pEdit Editor context whose `current` pointer is updated on success.
 * @param x     Desired column pixel X coordinate.
 * @param y     Target row baseline pixel Y coordinate.
 * @return      1 if target row exists and `pEdit->current` was updated,
 *              0 if target row is beyond the end of the document.
 * @note        Replaces the original 75x ConvertNodePosi() fallback loop
 *              (which was O(75 * N^2)) with a single O(page) scan that tracks
 *              `lastOnLine` to clamp cleanly onto shorter lines and handles
 *              trailing empty lines after a final ENTER node.
 * @complexity  O(page).
 *****************************************************************************/
int MoveCursorVertical(struct EDITTYPE *pEdit, int x, int y)
{
	int targetY, targetColX;
	int curX=EDITERSCREENX1;
	int curY=EDITERSCREENY1+CHARHIGH;
	const int maxX=CHARINLINE*CHARSIZE+EDITERSCREENX1;
	struct NODE *pNode=pEdit->pageFirst;
	struct NODE *lastOnLine=NULL;
	int foundLine=0;

	if(pNode==NULL)
	{
		return 0;
	}
	if(y<=EDITERSCREENY1 || y>EDITERSCREENY2 || x<EDITERSCREENX1)
	{
		return 0;
	}

	targetY=EDITERSCREENY1+(1+(y-1-EDITERSCREENY1)/CHARHIGH)*CHARHIGH;
	targetColX=EDITERSCREENX1+((x-EDITERSCREENX1)/CHARSIZE)*CHARSIZE;

	while(pNode!=NULL && curY<=targetY)
	{
		int width=(pNode->flag==2) ? (2*CHARSIZE) : CHARSIZE;
		if(curY==targetY)
		{
			foundLine=1;
			lastOnLine=pNode;
			/* Exact column hit on target row */
			if(targetColX>=curX && targetColX<curX+width)
			{
				pEdit->current=(pNode==pEdit->pageFirst) ? NULL : pNode->last;
				return 1;
			}
		}

		if(pNode->flag==0)
		{
			curX=EDITERSCREENX1;
			curY+=CHARHIGH;
		}
		else
		{
			curX+=pNode->flag*CHARSIZE;
			if(curX>=maxX)
			{
				curX=EDITERSCREENX1;
				curY+=CHARHIGH;
			}
		}
		pNode=pNode->next;
	}

	/* Target row exists but ended before targetColX: clamp caret to its end */
	if(foundLine && lastOnLine!=NULL)
	{
		if(lastOnLine->flag==0)
		{
			/* Place caret immediately BEFORE the row's newline node */
			pEdit->current=(lastOnLine==pEdit->pageFirst) ? NULL : lastOnLine->last;
		}
		else
		{
			/* Place caret immediately AFTER the final character on the row */
			pEdit->current=lastOnLine;
		}
		return 1;
	}

	/* Target row is an empty trailing line following a final ENTER node */
	if(pNode==NULL && curY==targetY && pEdit->last!=NULL)
	{
		pEdit->current=pEdit->last;
		return 1;
	}

	return 0;
}
