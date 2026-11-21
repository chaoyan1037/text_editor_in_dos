/*****************************************************************************
 * File: FUNCKEY.C
 * Purpose: Core interactive editing commands: clean shutdown, 4-way cursor
 *          movement & vertical scrolling, keyboard/mouse text selection,
 *          cut/paste, single-char backspace, unified ASCII/Hanzi Find, and
 *          in-place/resizing Find & Replace.
 * Programmer: YanChao (2011.9.15-2011.11.2)
 *****************************************************************************/
#include<graphics.h>
#include<conio.h>
#include<bios.h>
#include<dos.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"h/common.h"
#include"h/language.h"
#include"h/struct.h"
#include"h/node.h"
#include"h/mouse.h"
#include"h/malloc.h"
#include"h/hanzi.h"
#include"h/spell.h"
#include"h/funckey.h"

void MoveLeft(struct EDITTYPE *pEdit);
void MoveRight(struct EDITTYPE *pEdit);
int Find(struct EDITTYPE *pEdit, char *pGoal);
void Dialog(int x0, int y0, int x1, int y1, struct EDITTYPE *pEdit);
void ChoosenOne(struct EDITTYPE *pEdit, int x, int y);

/*****************************************************************************
 * Function: ExitEditer
 * Purpose:  Shut down the editor cleanly without returning to main()'s loop.
 *
 * @param pEdit Master editor context.
 * @post        Closes BGI VGA mode, frees all document and detached clipboard
 *              nodes, closes persistent HZK16 handle, waits for a key, and
 *              terminates the process via exit(0).
 *****************************************************************************/
void ExitEditer(struct EDITTYPE *pEdit)
{
	closegraph();
	printf("thanks for using.\n");
	FreeAllNode(pEdit);
	exit(0);
}

/*****************************************************************************
 * Function: MoveCursor
 * Purpose:  Erase the current caret underline and move `pEdit->current`
 *           according to `pEdit->state.key` (LEFT, RIGHT, UP, DOWN, or 0 for
 *           click-reposition refresh).
 *
 * @param pEdit Master editor context.
 * @post        Sets `cursorFlag = 1` so DrawCursor() recalculates (x, y).
 *              At viewport top (row 1) on UP, scrolls `pageFirst` backward one
 *              line if off-screen lines exist; at viewport bottom (row 25) on
 *              DOWN, scrolls `pageFirst` forward one line and sets
 *              `pageDownFlag = 1` so the next loop lands the caret on row 25.
 * @complexity  O(1) for LEFT/RIGHT; O(page) for UP/DOWN.
 *****************************************************************************/
void MoveCursor(struct EDITTYPE *pEdit)
{
	int x=pEdit->x, y=pEdit->y;

	/* Overwrite old underline cursor in WHITE before repositioning */
	setcolor(WHITE);
	setwritemode(COPY_PUT);
	line(x, y, x+8, y);
	pEdit->cursorFlag=1;

	if(pEdit->state.key==LEFT)
	{
		if(pEdit->current!=NULL)
		{
			if( pEdit->current!=pEdit->pageFirst )
			{
				pEdit->current=pEdit->current->last;
			}
			else
			{
				/* Stepping left off pageFirst anchors caret at top-left (NULL) */
				pEdit->current=NULL;
			}
		}
	}
	else if(pEdit->state.key==RIGHT)
	{
		if(pEdit->current==NULL)
		{
			pEdit->current=pEdit->pageFirst;
		}
		else if( pEdit->current->next!=NULL )
		{
			pEdit->current=pEdit->current->next;
		}
	}
	else if(pEdit->state.key==UP)
	{
		if(y==EDITERSCREENY1+2*CHARHIGH && x==EDITERSCREENX1)
		{
			/* Row 2 col 1 -> directly before pageFirst (row 1 col 1) */
			pEdit->current=NULL;
		}
		else if(y>=EDITERSCREENY1+2*CHARHIGH)
		{
			y-=CHARHIGH;
			MoveCursorVertical(pEdit, x, y);
		}
		else if(y==EDITERSCREENY1+CHARHIGH)
		{
			/* Row 1: scroll viewport up one line if off-screen text precedes pageFirst */
			if(pEdit->pageFirst!=NULL && pEdit->pageFirst->last!=NULL)
			{
				struct NODE *pNode=pEdit->pageFirst->last;
				while(pNode->last!=NULL && pNode->firstInLine!=1)
				{
					pNode=pNode->last;
				}
				pEdit->pageFirst=pNode;
				pEdit->editFlag=1;
			}
		}
	}
	else if(pEdit->state.key==DOWN)
	{
		if(y<EDITERSCREENY2)
		{
			y+=CHARHIGH;
			MoveCursorVertical(pEdit, x, y);
		}
		else if(y==EDITERSCREENY2 && pEdit->pageFirst!=NULL)
		{
			/* Row 25: scroll viewport down one line if >= CHARINROW+1 lines remain */
			int number=0;
			struct NODE *pNode=pEdit->pageFirst;
			for( ; pNode!=NULL; pNode=pNode->next)
			{
				if(pNode->firstInLine==1)
				{
					number++;
					if(number>=CHARINROW+1)
					{
						break;
					}
				}
			}
			if(number>=CHARINROW+1)
			{
				pNode=pEdit->pageFirst->next;
				while(pNode!=NULL && pNode->firstInLine!=1)
				{
					pNode=pNode->next;
				}
				if(pNode!=NULL)
				{
					pEdit->pageFirst=pNode;
					pEdit->editFlag=1;
					pEdit->pageDownFlag=1;
				}
			}
		}
	}
}

/*****************************************************************************
 * Function: BlackWhiteChoosen
 * Purpose:  Begin or extend a highlighted selection range via Ctrl+Left or
 *           Ctrl+Right (or mouse hold).
 *
 * @param pEdit Master editor context.
 * @post        If a previous detached clipboard existed (`copyCutFlag == 1`),
 *              frees it first so `clipStart..clipLast` can safely track the
 *              new live selection inside the document list.
 *****************************************************************************/
void BlackWhiteChoosen(struct EDITTYPE *pEdit)
{
	if(pEdit->copyCutFlag==1)
	{
		ClearClip(pEdit);
		pEdit->copyActive=0;
	}
	if(pEdit->clipFlag==1)
	{
		pEdit->clipStart=NULL;
		pEdit->clipLast=NULL;
		pEdit->clipFlag=0;
	}
	if(pEdit->state.key==CTRL_LEFT)
	{
		MoveLeft(pEdit);
	}
	else
	{
		MoveRight(pEdit);
	}
}

/*****************************************************************************
 * Function: MoveLeft
 * Purpose:  Highlight `pEdit->current` in blue/white, expand `clipStart`
 *           leftward, and retreat `pEdit->current` by one node.
 *
 * @param pEdit Master editor context.
 *****************************************************************************/
void MoveLeft(struct EDITTYPE *pEdit)
{
	int x=0, y=0;
	if( pEdit->current!=NULL )
	{
		if(pEdit->current!=pEdit->pageFirst)
		{
			if( GetNodePosi(pEdit, pEdit->current, &x, &y) )
			{
				if(pEdit->current->flag==0)
				{
					/* Highlight newline slot at the end of the preceding visual row */
					if(pEdit->current->last!=NULL
						&& GetNodePosi(pEdit, pEdit->current->last, &x, &y))
					{
						setfillstyle(SOLID_FILL, BLUE);
						bar(x, y-CHARHIGH, x+CHARSIZE, y);
					}
				}
				else
				{
					int w=pEdit->current->flag*CHARSIZE;
					/* If GetNodePosi() soft-wrapped to col 0 of next row, step back */
					if(x==EDITERSCREENX1)
					{
						x+=CHARSIZE*CHARINLINE;
						y-=CHARHIGH;
					}
					ChoosenOne(pEdit, x-w, y);
				}
			}
			if(pEdit->clipLast==NULL)
			{
				pEdit->clipLast=pEdit->current;
			}
			pEdit->clipStart=pEdit->current;
			pEdit->current=pEdit->current->last;
		}
		else
		{
			/* Selecting pageFirst places caret at top-left (current = NULL) */
			x=EDITERSCREENX1;
			y=EDITERSCREENY1+CHARHIGH;
			ChoosenOne(pEdit, x, y);
			if(pEdit->clipLast==NULL)
			{
				pEdit->clipLast=pEdit->current;
			}
			pEdit->clipStart=pEdit->pageFirst;
			pEdit->current=NULL;
		}
	}
}

/*****************************************************************************
 * Function: MoveRight
 * Purpose:  Advance `pEdit->current` by one node, highlight it in blue/white,
 *           and expand `clipLast` rightward.
 *
 * @param pEdit Master editor context.
 *****************************************************************************/
void MoveRight(struct EDITTYPE *pEdit)
{
	int x=0, y=0;
	if( pEdit->current==NULL && pEdit->pageFirst!=NULL )
	{
		x=EDITERSCREENX1;
		y=EDITERSCREENY1+CHARHIGH;
		pEdit->current=pEdit->pageFirst;
		ChoosenOne(pEdit, x, y);

		pEdit->clipStart=pEdit->pageFirst;
		pEdit->clipLast=pEdit->pageFirst;
	}
	else if( pEdit->current!=NULL && pEdit->current!=pEdit->last )
	{
		pEdit->current=pEdit->current->next;
		if( pEdit->current!=NULL && GetNodePosi(pEdit, pEdit->current->last, &x, &y) )
		{
			ChoosenOne(pEdit, x, y);
		}
		if(pEdit->clipStart==NULL)
		{
			pEdit->clipStart=pEdit->current;
		}
		pEdit->clipLast=pEdit->current;
	}
}

/*****************************************************************************
 * Function: Paste
 * Purpose:  Insert newly allocated copies of every node in the detached
 *           clipboard (`clipStart..clipLast`) at the current caret (Ctrl+V).
 *
 * @param pEdit Master editor context.
 * @pre         Only runs when `copyActive == 1 && copyCutFlag == 1` (detached
 *              clipboard owned by CopySelection() or CopyCut()).
 * @post        Clipboard contents remain intact so Paste() can be invoked
 *              repeatedly; safe even when pasting into an empty document.
 * @complexity  O(C) where C = clipboard length.
 *****************************************************************************/
void Paste(struct EDITTYPE *pEdit)
{
	if(pEdit->copyActive==1 && pEdit->copyCutFlag==1 && pEdit->clipStart!=NULL)
	{
		struct NODE *pNode=pEdit->clipStart;
		pEdit->editFlag=1;
		pEdit->cursorFlag=1;

		while(pNode!=NULL)
		{
			if(pEdit->start==NULL)
			{
				/* Pasting into an empty document creates the initial node cleanly */
				struct NODE *p=MallocNode(pEdit);
				p->data[0]=pNode->data[0];
				p->data[1]=pNode->data[1];
				p->data[2]='\0';
				p->flag=pNode->flag;
				p->firstInLine=1;
				pEdit->start=pEdit->pageFirst=pEdit->last=pEdit->current=p;
			}
			else if(pEdit->current!=NULL && pEdit->current->next==NULL)
			{
				AddNode(pEdit, pNode->data);
				pEdit->current->flag=pNode->flag;
			}
			else
			{
				InsertNode(pEdit, pNode->data);
				pEdit->current->flag=pNode->flag;
			}

			if(pNode==pEdit->clipLast)
			{
				break;
			}
			pNode=pNode->next;
		}
	}
}

/*****************************************************************************
 * Function: CopyCut
 * Purpose:  Unlink the live selection span (`clipStart..clipLast`) from the
 *           document into a detached clipboard (Ctrl+X), or immediately free
 *           it when invoked by BACKSPACE after Select All.
 *
 * @param pEdit Master editor context.
 * @pre         Only executes when `copyCutFlag == 0` (live document span) so
 *              repeated Ctrl+X presses cannot corrupt an already-detached clip.
 * @post        - Main list bridges `clipStart->last <-> clipLast->next`.
 *              - `start`, `last`, `pageFirst`, and `current` are all repaired.
 *              - Detached clip has `clipStart->last == NULL` and
 *                `clipLast->next == NULL` with `copyCutFlag = 1` (or freed if
 *                invoked via BACKSPACE).
 *****************************************************************************/
void CopyCut(struct EDITTYPE *pEdit)
{
	if( pEdit->copyCutFlag==0 && pEdit->clipStart!=NULL && pEdit->clipLast!=NULL )
	{
		struct NODE *pPrev=pEdit->clipStart->last;
		struct NODE *pNext=pEdit->clipLast->next;
		int pageFirstCut=0;
		struct NODE *pScan=pEdit->clipStart;

		/* Check if the cut span encompasses the current viewport start */
		while(pScan!=NULL)
		{
			if(pScan==pEdit->pageFirst)
			{
				pageFirstCut=1;
				break;
			}
			if(pScan==pEdit->clipLast)
			{
				break;
			}
			pScan=pScan->next;
		}

		/* Stitch main document list around the removed span */
		if(pPrev!=NULL)
		{
			pPrev->next=pNext;
		}
		else
		{
			pEdit->start=pNext;
		}

		if(pNext!=NULL)
		{
			pNext->last=pPrev;
		}
		else
		{
			pEdit->last=pPrev;
		}

		if(pEdit->start!=NULL)
		{
			pEdit->start->firstInLine=1;
		}

		/* Keep viewport populated if pageFirst was inside the cut span */
		if(pageFirstCut)
		{
			if(pNext!=NULL)
			{
				pEdit->pageFirst=pNext;
				pEdit->pageFirst->firstInLine=1;
			}
			else if(pPrev!=NULL)
			{
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

		/* Position caret immediately before the removed span */
		if(pPrev==NULL || (pEdit->pageFirst!=NULL && pEdit->pageFirst->last==pPrev))
		{
			pEdit->current=NULL;
		}
		else
		{
			pEdit->current=pPrev;
		}

		/* Sever outer links on the detached clipboard segment */
		pEdit->clipStart->last=NULL;
		pEdit->clipLast->next=NULL;

		pEdit->copyActive=1;
		pEdit->copyCutFlag=1;
		pEdit->cursorFlag=1;
		pEdit->editFlag=1;

		/* BACKSPACE deletes the selection outright without keeping it in clipboard */
		if(pEdit->state.key==BACKSPACE)
		{
			ClearClip(pEdit);
			pEdit->copyActive=0;
		}
	}
}

/*****************************************************************************
 * Function: BackSpace
 * Purpose:  Delete the single node immediately preceding the caret.
 *
 * @param pEdit Master editor context (no-op when `current == NULL`).
 *****************************************************************************/
void BackSpace(struct EDITTYPE *pEdit)
{
	if(pEdit->current!=NULL)
	{
		DeleteNode(pEdit->current, pEdit);
		pEdit->editFlag=1;
		pEdit->cursorFlag=1;
	}
}

/*****************************************************************************
 * Function: Find
 * Purpose:  Search forward from the node after `current` (or from `pageFirst`
 *           when `current == NULL`) for `pGoal`, supporting ASCII, GB2312
 *           Hanzi, and mixed ASCII+Hanzi queries.
 *
 * @param pEdit Master editor context.
 * @param pGoal NUL-terminated search string.
 * @return      1 if matched (sets `findStart`, `findLast`, `current=findLast`,
 *              and highlights all visible matched nodes in blue/white);
 *              0 if not found or `pGoal` is empty/document is empty.
 * @note        Uses `findStart`/`findLast` instead of `clipStart`/`clipLast`
 *              so searching never overwrites or leaks the clipboard.
 * @complexity  O(N * M) worst-case where N = remaining nodes, M = strlen(pGoal).
 *****************************************************************************/
int Find(struct EDITTYPE *pEdit, char *pGoal)
{
	struct NODE *pStart;
	struct NODE *pNode;
	struct NODE *pEnd=NULL;
	int found=0;

	pEdit->findStart=NULL;
	pEdit->findLast=NULL;

	if(pGoal==NULL || pGoal[0]=='\0' || pEdit->pageFirst==NULL)
	{
		return 0;
	}

	/* Search forward from current->next; if not found, wrap around from pageFirst */
	{
		struct NODE *searchLimit=NULL;
		int pass;
		for(pass=0; pass<2 && !found; pass++)
		{
			if(pass==0)
			{
				pStart=(pEdit->current==NULL) ? pEdit->pageFirst : pEdit->current->next;
				searchLimit=NULL;
			}
			else
			{
				if(pEdit->current==NULL)
				{
					break; /* Pass 0 already searched from pageFirst to end */
				}
				pStart=pEdit->pageFirst;
				searchLimit=pEdit->current->next;
			}

			for( ; pStart!=NULL && pStart!=searchLimit; pStart=pStart->next)
			{
				int idx=0;
				int match=1;
				pNode=pStart;
				pEnd=NULL;

				while(pGoal[idx]!='\0')
				{
					unsigned char ch=(unsigned char)pGoal[idx];
					if(pNode==NULL)
					{
						match=0;
						break;
					}
					if(ch>=0x20 && ch<=0x7e)
					{
						/* Printable ASCII byte must match a flag=1 node */
						if(pNode->flag!=1 || pNode->data[0]!=pGoal[idx])
						{
							match=0;
							break;
						}
						pEnd=pNode;
						pNode=pNode->next;
						idx+=1;
					}
					else
					{
						/* Non-ASCII lead byte must match a 2-byte GB2312 flag=2 node */
						if(pGoal[idx+1]=='\0' || pNode->flag!=2
							|| pNode->data[0]!=pGoal[idx]
							|| pNode->data[1]!=pGoal[idx+1])
						{
							match=0;
							break;
						}
						pEnd=pNode;
						pNode=pNode->next;
						idx+=2;
					}
				}

				if(match && pEnd!=NULL)
				{
					found=1;
					break;
				}
			}
		}
	}

	if(found)
	{
		int x=EDITERSCREENX1, y=EDITERSCREENY1+CHARHIGH;
		const int maxX=CHARINLINE*CHARSIZE+EDITERSCREENX1;
		struct NODE *pCur=pEdit->pageFirst;

		pEdit->findStart=pStart;
		pEdit->findLast=pEnd;
		pEdit->current=pEnd;

		/* Walk once from pageFirst to find exact top-left (x, y) of pStart */
		while(pCur!=NULL && pCur!=pStart)
		{
			if(pCur->flag==0)
			{
				x=EDITERSCREENX1;
				y+=CHARHIGH;
			}
			else
			{
				x+=pCur->flag*CHARSIZE;
				if(x>=maxX)
				{
					x=EDITERSCREENX1;
					y+=CHARHIGH;
				}
			}
			pCur=pCur->next;
		}

		/* Highlight every matched node from pStart..pEnd, honoring line wraps */
		if(pCur==pStart)
		{
			struct NODE *savedCurrent=pEdit->current;
			pNode=pStart;
			while(pNode!=NULL)
			{
				if(y<=EDITERSCREENY2)
				{
					pEdit->current=pNode;
					ChoosenOne(pEdit, x, y);
				}
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
				if(pNode==pEnd)
				{
					break;
				}
				pNode=pNode->next;
			}
			pEdit->current=savedCurrent;
		}
		return 1;
	}

	return 0;
}

/*****************************************************************************
 * Function: FindFunction
 * Purpose:  Display the Find dialog (Ctrl+F) and read up to 13 printable
 *           ASCII characters into `pEdit->findGoal`.
 *
 * @param pEdit Master editor context.
 * @post        On ENTER: sets `state.functionActive = 5` (awaiting F/C).
 *              On ESC:   calls CancleFind() and returns to normal editing.
 *****************************************************************************/
void FindFunction(struct EDITTYPE *pEdit)
{
	int i=0;
	pEdit->findGoal[0]='\0';
	Dialog(240, 160, 400, 240, pEdit);
	Printfhz(250, 165, "\xb2\xe9\xd5\xd2", RED, pEdit);
	while(1)
	{
		if(bioskey(1)!=0)
		{
			long int key=bioskey(0);
			int ascii=(int)(key&0x00ff);
			if(key==ESC)
			{
				CancleFind(pEdit);
				break;
			}
			else if(key==BACKSPACE)
			{
				if(i>0)
				{
					i--;
					pEdit->findGoal[i]='\0';
					setfillstyle(SOLID_FILL, LIGHTGREEN);
					bar(250, 185, 389, 205);
					setcolor(WHITE);
					rectangle(250, 185, 389, 205);
					WhiteBlack(250, 185, 389, 205, BLACK);
				}
			}
			else if(key==ENTER)
			{
				pEdit->state.functionActive=5;
				break;
			}
			else if(ascii>=0x20 && ascii<=0x7e && i<13)
			{
				pEdit->findGoal[i]=(char)ascii;
				pEdit->findGoal[i+1]='\0';
				i++;
			}
			setcolor(BLACK);
			outtextxy(253, 205, &pEdit->findGoal[0]);
		}
	}
}

/*****************************************************************************
 * Function: FingGoal
 * Purpose:  Invoke Find() for the active Find (state 5) or Replace (state 6)
 *           workflow and transition dialog visuals/state accordingly.
 *
 * @param pEdit Master editor context.
 * @post        - Not found: prints "提示：未找到！" and sets `functionActive=1`.
 *              - Found in Replace mode (state 6): changes left button to
 *                "替换(R)" and advances to `functionActive = 7`.
 *****************************************************************************/
void FingGoal(struct EDITTYPE *pEdit)
{
	setwritemode(COPY_PUT);
	if( Find(pEdit, &pEdit->findGoal[0])==0 )
	{
		setfillstyle(SOLID_FILL, LIGHTGRAY);
		if(pEdit->state.functionActive==5)
		{
			bar(250, 164, 283, 182);
			setcolor(BLACK);
			Printfhz(250, 165, "\xcc\xe1\xca\xbe\xa3\xba\xce\xb4\xd5\xd2\xb5\xbd\xa3\xa1", RED, pEdit);
		}
		else if(pEdit->state.functionActive==6)
		{
			bar(241, 134, 283, 152);
			Printfhz(250, 135, "\xcc\xe1\xca\xbe\xa3\xba\xce\xb4\xd5\xd2\xb5\xbd\xa3\xa1", RED, pEdit);
			pEdit->replaceGoal[0]='\0';

			setfillstyle(SOLID_FILL, LIGHTGRAY);
			bar(241, 210, 320, 239);
			setcolor(BLACK);
			rectangle(258, 214, 314, 232);
			WhiteBlack(258, 214, 314, 232, WHITE);
			Printfhz(260, 215, "\xc8\xb7\xb6\xa8", RED, pEdit);
			setcolor(RED);
			outtextxy(293, 231, "(S)");
		}
		pEdit->findGoal[0]='\0';
		pEdit->state.functionActive=1;
	}
	else if(pEdit->state.functionActive==6)
	{
		/* Replace workflow: match found -> morph Find(F) button into Replace(R) */
		pEdit->state.functionActive=7;
		setwritemode(COPY_PUT);
		setfillstyle(SOLID_FILL, LIGHTGRAY);
		bar(241, 210, 320, 239);
		setcolor(BLACK);
		rectangle(258, 214, 314, 232);
		WhiteBlack(258, 214, 314, 232, WHITE);
		Printfhz(260, 215, "\xcc\xe6\xbb\xbb", RED, pEdit);
		setcolor(RED);
		outtextxy(293, 231, "(R)");
	}
}

/* Dialog - Draw shared Find/Replace modal frame, input box, and (F)/(C) buttons */
void Dialog(int x0, int y0, int x1, int y1, struct EDITTYPE *pEdit)
{
	setfillstyle(SOLID_FILL, LIGHTGRAY);
	bar(x0, y0, x1, y1);
	button3d(x0, y0, x1, y1);

	/* Green input box (250..389 x 185..205) */
	setfillstyle(SOLID_FILL, LIGHTGREEN);
	bar(250, 185, 389, 205);

	setcolor(WHITE);
	rectangle(250, 185, 389, 205);
	WhiteBlack(250, 185, 389, 205, BLACK);

	/* Find(F) button (258..314 x 214..232) and Cancel(C) (323..379 x 214..232) */
	setcolor(BLACK);
	rectangle(258, 214, 314, 232);
	WhiteBlack(258, 214, 314, 232, WHITE);

	setcolor(BLACK);
	rectangle(323, 214, 379, 232);
	WhiteBlack(323, 214, 379, 232, WHITE);

	Printfhz(260, 215, "\xb2\xe9\xd5\xd2", RED, pEdit);
	Printfhz(325, 215, "\xc8\xa1\xcf\xfb", RED, pEdit);

	setcolor(RED);
	outtextxy(293, 231, "(F)");
	outtextxy(357, 231, "(C)");
}

/* CancleFind - Reset Find state and trigger viewport repaint to clear highlights */
void CancleFind(struct EDITTYPE *pEdit)
{
	pEdit->findStart=NULL;
	pEdit->findLast=NULL;
	pEdit->findGoal[0]='\0';
	pEdit->state.functionActive=0;
	pEdit->editFlag=1;
}

/*****************************************************************************
 * Function: FindAndReplace
 * Purpose:  Display the Find & Replace modal dialog (Ctrl+R) and read both
 *           `findGoal` (inputFlag=0) and `replaceGoal` (inputFlag=1).
 *
 * @param pEdit Master editor context.
 * @post        First ENTER advances focus from Find box to Replace box;
 *              second ENTER arms search (`functionActive = 6`). ESC cancels.
 *****************************************************************************/
void FindAndReplace(struct EDITTYPE *pEdit)
{
	int i=0, j=0, inputFlag=0;
	pEdit->findGoal[0]='\0';
	pEdit->replaceGoal[0]='\0';
	Dialog(240, 130, 400, 240, pEdit);

	/* Shorten lower input box left edge to make room for "替换:" label */
	setfillstyle(SOLID_FILL, LIGHTGRAY);
	bar(250, 185, 288, 205);
	setcolor(BLACK);
	line(288, 185, 288, 205);

	/* Draw upper "查找:" input box (288..389 x 155..175) */
	setfillstyle(SOLID_FILL, LIGHTGREEN);
	bar(288, 155, 389, 175);
	setcolor(WHITE);
	rectangle(288, 155, 389, 175);
	WhiteBlack(288, 155, 389, 175, BLACK);

	setcolor(RED);
	Printfhz(250, 135, "\xcc\xe6\xbb\xbb", RED, pEdit);
	Printfhz(250, 159, "\xb2\xe9\xd5\xd2", RED, pEdit);
	Printfhz(250, 189, "\xcc\xe6\xbb\xbb", RED, pEdit);
	outtextxy(281, 175, ":");
	outtextxy(281, 205, ":");

	while(1)
	{
		if(bioskey(1)!=0)
		{
			long int key=bioskey(0);
			int ascii=(int)(key&0x00ff);
			if(key==ESC)
			{
				CancleReplace(pEdit);
				break;
			}
			else if(key==BACKSPACE)
			{
				if(i>0 && inputFlag==0)
				{
					i--;
					pEdit->findGoal[i]='\0';
					setfillstyle(SOLID_FILL, LIGHTGREEN);
					bar(288, 155, 389, 175);
					setcolor(WHITE);
					rectangle(288, 155, 389, 175);
					WhiteBlack(288, 155, 389, 175, BLACK);
				}
				else if(j>0 && inputFlag==1)
				{
					j--;
					pEdit->replaceGoal[j]='\0';
					setfillstyle(SOLID_FILL, LIGHTGREEN);
					bar(288, 185, 389, 205);
					setcolor(WHITE);
					rectangle(288, 185, 389, 205);
					WhiteBlack(288, 185, 389, 205, BLACK);
				}
			}
			else if(key==ENTER)
			{
				if(inputFlag==0)
				{
					inputFlag=1;
				}
				else
				{
					pEdit->state.functionActive=6;
					pEdit->state.mouseFlag=0;
					break;
				}
			}
			else if(ascii>=0x20 && ascii<=0x7e)
			{
				if(i<12 && inputFlag==0)
				{
					pEdit->findGoal[i]=(char)ascii;
					pEdit->findGoal[i+1]='\0';
					i++;
				}
				else if(j<12 && inputFlag==1)
				{
					pEdit->replaceGoal[j]=(char)ascii;
					pEdit->replaceGoal[j+1]='\0';
					j++;
				}
			}

			if(inputFlag==0)
			{
				setcolor(BLACK);
				outtextxy(290, 175, &pEdit->findGoal[0]);
			}
			else
			{
				setcolor(BLACK);
				outtextxy(290, 205, &pEdit->replaceGoal[0]);
			}
		}
	}
}

/*****************************************************************************
 * Function: ReplaceFunction
 * Purpose:  Replace the document span `[findStart .. findLast]` with the
 *           ASCII string in `pEdit->replaceGoal`.
 *
 * @param pEdit Master editor context.
 * @post        Handles equal-length, longer, shorter, and empty ("")
 *              replacements safely without use-after-free, clears `findStart`/
 *              `findLast`/`replaceGoal`, returns to `functionActive = 0`, and
 *              requests a full viewport & caret redraw.
 *****************************************************************************/
void ReplaceFunction(struct EDITTYPE *pEdit)
{
	if(pEdit->findStart!=NULL && pEdit->findLast!=NULL)
	{
		int i=0;
		struct NODE *pNode=pEdit->findStart;
		struct NODE *pEnd=pEdit->findLast;

		/* Phase 1: overwrite existing matched nodes in-place for shared prefix */
		while(pEdit->replaceGoal[i]!='\0' && pNode!=NULL)
		{
			pNode->flag=1;
			pNode->data[0]=pEdit->replaceGoal[i];
			pNode->data[1]='\0';
			pNode->data[2]='\0';
			pEdit->current=pNode;
			i++;
			if(pNode==pEnd)
			{
				pNode=NULL;
				break;
			}
			pNode=pNode->next;
		}

		/* Phase 2a: replacement longer than match -> append extra chars */
		if(pEdit->replaceGoal[i]!='\0')
		{
			for( ; pEdit->replaceGoal[i]!='\0'; i++)
			{
				pEdit->state.ascii=(unsigned char)pEdit->replaceGoal[i];
				GetEnglish(pEdit);
			}
		}
		/* Phase 2b: replacement shorter/empty -> delete leftover matched nodes */
		else if(pNode!=NULL)
		{
			struct NODE *pNext;
			while(pNode!=NULL)
			{
				int isLast=(pNode==pEnd);
				pNext=pNode->next; /* Save next BEFORE DeleteNode frees pNode */
				DeleteNode(pNode, pEdit);
				if(isLast)
				{
					break;
				}
				pNode=pNext;
			}
		}

		pEdit->editFlag=1;
		pEdit->cursorFlag=1;
		pEdit->replaceGoal[0]='\0';
		pEdit->state.functionActive=0;
	}
	pEdit->findLast=NULL;
	pEdit->findStart=NULL;
}

/* CancleReplace - Clear Find/Replace state and redraw viewport */
void CancleReplace(struct EDITTYPE *pEdit)
{
	pEdit->findLast=NULL;
	pEdit->findStart=NULL;
	pEdit->findGoal[0]='\0';
	pEdit->replaceGoal[0]='\0';
	pEdit->state.functionActive=0;
	pEdit->editFlag=1;
}

/*****************************************************************************
 * Function: AllChosen
 * Purpose:  Highlight all visible nodes on the current viewport page (Ctrl+A)
 *           and enter `functionActive = 13` so Ctrl+C, Ctrl+X, or BACKSPACE
 *           acts on the entire page selection.
 *
 * @param pEdit Master editor context (safe no-op on empty documents).
 * @post        Frees any prior detached clipboard, highlights `pageFirst`
 *              through `pageLast` (or `last`), restores `current`, and sets
 *              `clipStart..clipLast` + `functionActive = 13`.
 *****************************************************************************/
void AllChosen(struct EDITTYPE *pEdit)
{
	struct NODE *pNode=pEdit->pageFirst;
	struct NODE *pageLast=NULL;
	struct NODE *temp=pEdit->current;
	int number=0, count=0;
	int x=EDITERSCREENX1, y=EDITERSCREENY1+CHARHIGH;

	if(pEdit->pageFirst==NULL)
	{
		return;
	}

	if(pEdit->copyCutFlag==1)
	{
		ClearClip(pEdit);
		pEdit->copyActive=0;
	}

	/* Advance to the start of the 25th visible line (if present) */
	for( ; pNode!=NULL; pNode=pNode->next)
	{
		if(pNode->firstInLine==1)
		{
			number++;
			if(number>=CHARINROW)
			{
				break;
			}
		}
	}
	/* Find the terminating node of the 25th line (accounting for 2-col Hanzi) */
	for( ; pNode!=NULL; pNode=pNode->next)
	{
		count+=(pNode->flag==0) ? 1 : pNode->flag;
		if(pNode->flag==0 || count>=CHARINLINE)
		{
			pageLast=pNode;
			break;
		}
	}

	/* Highlight from pageFirst to pageLast/last (works for 1-node docs too) */
	pEdit->current=pEdit->pageFirst;
	pEdit->clipStart=pEdit->pageFirst;
	pEdit->clipLast=pEdit->pageFirst;
	ChoosenOne(pEdit, x, y);
	while(pEdit->current!=pEdit->last && pEdit->current!=pageLast)
	{
		MoveRight(pEdit);
		if(pEdit->current==NULL)
		{
			break;
		}
	}
	pEdit->clipStart=pEdit->pageFirst;
	pEdit->current=temp;
	pEdit->state.functionActive=13;
}

/*****************************************************************************
 * Function: ChoosenOne
 * Purpose:  Render the node at `pEdit->current` with inverted selection
 *           colors (WHITE foreground on BLUE cell) at baseline (x, y).
 *
 * @param pEdit Editor context whose `current` node is drawn.
 * @param x     Left pixel coordinate of the character cell.
 * @param y     Baseline pixel coordinate (bottom edge of the 16px cell).
 *****************************************************************************/
void ChoosenOne(struct EDITTYPE *pEdit, int x, int y)
{
	if(pEdit->current==NULL)
	{
		return;
	}
	setfillstyle(SOLID_FILL, BLUE);
	setcolor(WHITE);
	if(pEdit->current->flag==1)
	{
		bar(x, y-CHARHIGH, x+CHARSIZE, y);
		outtextxy(x, y, pEdit->current->data);
	}
	else if(pEdit->current->flag==0)
	{
		bar(x, y-CHARHIGH, x+CHARSIZE, y);
	}
	else if(pEdit->current->flag==2)
	{
		bar(x, y-CHARHIGH, x+2*CHARSIZE, y);
		if(pEdit->current->data[0]==' ' && pEdit->current->data[1]==' ')
		{
			OutputYao(x, y-CHARHIGH, WHITE);
		}
		else
		{
			Displayhz(x, y-CHARHIGH, pEdit->current->data, WHITE, pEdit);
		}
	}
}

/*****************************************************************************
 * Function: MouseBWChosen
 * Purpose:  Poll the mouse button for up to ~100ms (50 x 2ms) after a viewport
 *           click to distinguish a quick click (move caret) from a press-and-
 *           hold (queue function 24 / CTRL_RIGHT to extend text selection).
 *
 * @param pEdit Master editor context.
 *****************************************************************************/
void MouseBWChosen(struct EDITTYPE *pEdit)
{
	int i=0;
	if(pEdit->state.button)
	{
		for(i=0; i<50; i++)
		{
			ReadMouse(pEdit);
			if(!pEdit->state.button)
			{
				break;
			}
			delay(2);
		}
		if(pEdit->state.button)
		{
			pEdit->state.key=CTRL_RIGHT;
			pEdit->state.function=24;
		}
	}
}
