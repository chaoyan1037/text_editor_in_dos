/*****************************************************************************
 * File: VIEW.C
 * Function: Redraw the visible text viewport from `pageFirst` and update
 *           title-bar filename changes.
 * Programmer: YanChao (2011.9.11-2011.9.12)
 *
 * Rendering & Layout Details:
 *   - Runs only when `editFlag == 1` (text area) or `file.fileflag == 1`
 *     (title bar).
 *   - Consecutive ASCII characters (`flag == 1`) on the same visual line are
 *     batched into `asciiBuf` and flushed with a single `outtextxy()` call to
 *     minimize BGI overhead.
 *   - Recomputes `firstInLine` on every node from `pageFirst` to the end of
 *     the document (clearing stale flags from prior edits) and updates
 *     `pEdit->lineCount` so scrolling, status bar, and scrollbar stay exact.
 *****************************************************************************/
#include<graphics.h>
#include<stdlib.h>
#include"h/hanzi.h"
#include"h/struct.h"
#include"h/malloc.h"
#include"h/common.h"
#include"h/spell.h"
#include"h/view.h"

/*
 * ViewAll - Repaint the text viewport when `editFlag == 1` and the title bar
 *           filename when `file.fileflag == 1`.
 */
void ViewAll(struct EDITTYPE *pEdit)
{
	if( pEdit->editFlag==1 )
	{
		int x=EDITERSCREENX1, y=EDITERSCREENY1+CHARHIGH;
		const int maxX=CHARINLINE*CHARSIZE+EDITERSCREENX1;
		struct NODE *pNode=pEdit->pageFirst;
		char asciiBuf[CHARINLINE+2];
		int asciiLen=0;
		int asciiStartX=EDITERSCREENX1;

		/* Clear client area (excluding the 20px right-hand scrollbar) */
		setcolor(BLACK);
		settextjustify(LEFT_TEXT, BOTTOM_TEXT);
		setfillstyle(SOLID_FILL, WHITE);
		bar(EDITERSCREENX1, EDITERSCREENY1, EDITERSCREENX2-20, EDITERSCREENY2);
		pEdit->state.mouseFlag=0; /* Background redrawn; mouse must redraw fresh */
		pEdit->editFlag=0;
		pEdit->cursorFlag=1;      /* Re-sync caret after viewport redraw */

		pEdit->lineCount=0;
		if(pNode!=NULL)
		{
			pNode->firstInLine=1;
		}
		while(pNode!=NULL)
		{
			/* Clear stale firstInLine before deciding whether this node wraps */
			if(pNode->next!=NULL)
			{
				pNode->next->firstInLine=0;
			}

			if(pNode->flag==0)
			{
				/* Explicit newline: flush pending ASCII run and advance row */
				if(asciiLen>0)
				{
					asciiBuf[asciiLen]='\0';
					outtextxy(asciiStartX, y, asciiBuf);
					asciiLen=0;
				}
				x=EDITERSCREENX1;
				y+=CHARHIGH;
				if(pNode->next!=NULL)
				{
					pNode->next->firstInLine=1;
					pEdit->lineCount++;
				}
			}
			else if(pNode->flag==1)
			{
				/* Accumulate visible ASCII characters into a single BGI run */
				if(y<=EDITERSCREENY2)
				{
					if(asciiLen==0)
					{
						asciiStartX=x;
					}
					if(asciiLen<CHARINLINE)
					{
						asciiBuf[asciiLen++]=pNode->data[0];
					}
					else
					{
						asciiBuf[asciiLen]='\0';
						outtextxy(asciiStartX, y, asciiBuf);
						asciiStartX=x;
						asciiBuf[0]=pNode->data[0];
						asciiLen=1;
					}
				}
				x+=CHARSIZE;
			}
			else if(pNode->flag==2)
			{
				/* Chinese glyph (16x16): flush ASCII run first, then draw glyph */
				if(asciiLen>0)
				{
					asciiBuf[asciiLen]='\0';
					outtextxy(asciiStartX, y, asciiBuf);
					asciiLen=0;
				}
				if(y<=EDITERSCREENY2)
				{
					/* Special pair (' ', ' ') with flag=2 represents custom glyph 'ˆ' */
					if( (pNode->data[0]==' ')&&(pNode->data[1]==' ') )
					{
						OutputYao(x, y-CHARHIGH, BLACK);
					}
					else
					{
						Displayhz(x, y-CHARHIGH, pNode->data, BLACK, pEdit);
					}
				}
				x+=2*CHARSIZE;
			}

			/* Soft word-wrap when visual column reaches or exceeds CHARINLINE */
			if( x>=maxX )
			{
				if(asciiLen>0)
				{
					asciiBuf[asciiLen]='\0';
					outtextxy(asciiStartX, y, asciiBuf);
					asciiLen=0;
				}
				x=EDITERSCREENX1;
				y+=CHARHIGH;
				if(pNode->next!=NULL)
				{
					pNode->next->firstInLine=1;
					pEdit->lineCount++;
				}
			}
			pNode=pNode->next;
		}
		/* Flush any trailing ASCII run on the last visible line */
		if(asciiLen>0 && y<=EDITERSCREENY2)
		{
			asciiBuf[asciiLen]='\0';
			outtextxy(asciiStartX, y, asciiBuf);
		}
	}

	/* Refresh active filename in the blue top title bar when changed */
	if(pEdit->file.fileflag==1)
	{
		setfillstyle(SOLID_FILL, LIGHTBLUE);
		bar(0, 0, 140, 25);
		setcolor(YELLOW);
		settextjustify(LEFT_TEXT, TOP_TEXT);
		outtextxy(5, 15, pEdit->file.fname);
		pEdit->file.fileflag=0;
	}
}
