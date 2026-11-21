/*****************************************************************************
 * File: NODE.C
 * Function: Doubly-linked document list operations, clipboard deep-copy/free,
 *           and viewport coordinate <-> node mapping routines.
 * Programmer: YanChao (2011.9.11-2011.9.15)
 *
 * Key Invariants Maintained by This Module:
 *   1. `start->last == NULL` and `last->next == NULL` whenever non-empty.
 *   2. `current == NULL` represents the caret positioned immediately BEFORE
 *      `pageFirst` (which may or may not equal `start` when scrolled).
 *   3. `GetNodePosi`, `ConvertNodePosi`, `MoveCursorVertical`, and `ViewAll`
 *      share the exact same line-wrapping threshold:
 *        maxX = CHARINLINE * CHARSIZE + EDITERSCREENX1 (605 px).
 *****************************************************************************/

#include<graphics.h>
#include<stdlib.h>
#include"h/malloc.h"
#include"h/common.h"
#include"h/struct.h"
#include"h/node.h"

/*
 * AddNode - Append a new node immediately after `current` (or at document tail).
 * Initializes empty documents cleanly and keeps `last` and `current` synced.
 */
void AddNode( struct EDITTYPE *pEdit, char data[] )
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
	if(pEdit->current==NULL)
	{
		/* Fallback: attach to document tail if current is unset */
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

/*
 * InsertNode - Insert a new node at the active caret position.
 *
 * Cases handled:
 *   - Empty document (`start == NULL`): creates the single head/tail node.
 *   - Mid/tail document (`current != NULL`): splices right after `current`.
 *   - Top-left of page (`current == NULL`): splices right BEFORE `pageFirst`,
 *     preserving any off-screen predecessor (`pageFirst->last`) when scrolled
 *     so the doubly-linked list remains intact.
 */
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
		/* Splice between current and current->next */
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
		/* Insert immediately before pageFirst (caret at top-left of page) */
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
				pPrev->next=pNode;
			}
			else
			{
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

/*
 * DeleteNode - Safely unlink and free a single node from the document list.
 *
 * Updates `start`, `last`, `pageFirst`, and `current` for all boundary cases
 * (head, tail, sole node, or scrolled `pageFirst`), and invalidates any
 * non-owned selection or find-match references pointing to the freed node.
 */
void DeleteNode( struct NODE *pNode, struct EDITTYPE *pEdit )
{
	struct NODE *pPrev;
	struct NODE *pNext;
	if(pNode==NULL)
	{
		return;
	}
	/* Capture neighbors before freeing pNode to avoid use-after-free */
	pPrev=pNode->last;
	pNext=pNode->next;

	/* Stitch surrounding links (or update document start/last) */
	if(pPrev==NULL)
	{
		pEdit->start=pNext;
	}
	else
	{
		pPrev->next=pNext;
	}

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

	/* Keep pageFirst valid if the top-left visible node was deleted */
	if(pEdit->pageFirst==pNode)
	{
		if(pNext!=NULL)
		{
			pEdit->pageFirst=pNext;
			pEdit->pageFirst->firstInLine=1;
		}
		else if(pPrev!=NULL)
		{
			/* Walk back to the start of the previous line so viewport stays populated */
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

	/* Retreat caret only if the deleted node was the current caret anchor */
	if(pEdit->current==pNode)
	{
		if(pPrev==NULL || (pEdit->pageFirst!=NULL && pEdit->pageFirst->last==pPrev))
		{
			pEdit->current=NULL;
		}
		else
		{
			pEdit->current=pPrev;
		}
	}

	/* Prevent dangling selection / find pointers into deleted nodes */
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

/*
 * FreeAllNode - Release every node in the document and any owned clipboard.
 *
 * Only calls ClearClip() when `copyCutFlag == 1` (detached heap clipboard);
 * if `copyCutFlag == 0`, `clipStart..clipLast` were already freed as part of
 * the main document list, avoiding double-free corruption.
 */
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

/*
 * ClearClip - Free all heap nodes belonging to a detached clipboard list.
 * Saves `pNext` before freeing each node and terminates cleanly at `clipLast`.
 */
void ClearClip(struct EDITTYPE *pEdit)
{
	struct NODE *pNode=pEdit->clipStart;
	struct NODE *pNext;
	while(pNode!=NULL)
	{
		pNext=(pNode==pEdit->clipLast) ? NULL : pNode->next;
		free(pNode);
		pNode=pNext;
	}
	pEdit->clipStart=NULL;
	pEdit->clipLast=NULL;
	pEdit->copyCutFlag=0;
}

/*
 * CopySelection - Deep-copy the live selection (`clipStart..clipLast`) into
 *                 an independent detached clipboard list (Ctrl+C).
 *
 * Detaching the copied data ensures that subsequent edits, deletions, or
 * pasting inside the original selection range cannot corrupt the clipboard
 * or trigger self-referential infinite loops.
 */
void CopySelection(struct EDITTYPE *pEdit)
{
	struct NODE *pSrc;
	struct NODE *newStart=NULL;
	struct NODE *newLast=NULL;

	if(pEdit->copyCutFlag==1)
	{
		/* Clipboard already owns a detached copy (e.g., repeated Ctrl+C) */
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

/*
 * GetNodePosi - Compute the screen caret coordinate immediately AFTER pGoalNode.
 *
 * Walks forward from `pageFirst` applying the same line-wrap and newline rules
 * as ViewAll(). Clamps Y to EDITERSCREENY2 so the caret stays inside the frame.
 *
 * Returns:
 *   1 if pGoalNode was found from pageFirst (writes *xx and *yy),
 *   0 if pGoalNode is NULL or not reachable from pageFirst (sets top-left on NULL).
 */
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

/*
 * ConvertNodePosi - Map a pixel click (x, y) to the corresponding caret node.
 *
 * Performs a single O(page) pass from `pageFirst`, terminating as soon as
 * `curY > targetY`. Snaps (x, y) to the 8x16 character grid and places
 * `pEdit->current` immediately before the clicked node (`NULL` if clicking
 * `pageFirst`), or at `pEdit->last` if clicking directly after the last char.
 *
 * Returns: 1 if a valid character cell was hit, 0 otherwise.
 */
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

	/* Quantize pixel coordinates to visual row baseline and 8px column start */
	targetY=EDITERSCREENY1+(1+(y-1-EDITERSCREENY1)/CHARHIGH)*CHARHIGH;
	targetColX=EDITERSCREENX1+((x-EDITERSCREENX1)/CHARSIZE)*CHARSIZE;

	while(pNode!=NULL && curY<=targetY)
	{
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

	/* Allow clicking directly after the final character of the document */
	if(pNode==NULL && curY==targetY && targetColX==curX && pEdit->last!=NULL)
	{
		pEdit->current=pEdit->last;
		return 1;
	}

	return 0;
}

/*
 * MoveCursorVertical - Move caret to target row `y` at column closest to `x`.
 *
 * Used by UP/DOWN arrow handling. Runs in a single O(page) pass from
 * `pageFirst`:
 *   - If target row has a character at column `x`, places caret there.
 *   - If target row is shorter than `x`, clamps caret to the end of that row
 *     (before its trailing ENTER node, or after the last char on the final line).
 *   - Handles empty trailing lines created by a final ENTER node.
 *
 * Returns: 1 if the target row exists and caret was updated, 0 otherwise.
 */
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

	/* Target row was shorter than targetColX: clamp to end of target row */
	if(foundLine && lastOnLine!=NULL)
	{
		if(lastOnLine->flag==0)
		{
			/* Place caret before the newline node so cursor stays on targetY */
			pEdit->current=(lastOnLine==pEdit->pageFirst) ? NULL : lastOnLine->last;
		}
		else
		{
			pEdit->current=lastOnLine;
		}
		return 1;
	}

	/* Target row is the empty line following a final ENTER node */
	if(pNode==NULL && curY==targetY && pEdit->last!=NULL)
	{
		pEdit->current=pEdit->last;
		return 1;
	}

	return 0;
}
