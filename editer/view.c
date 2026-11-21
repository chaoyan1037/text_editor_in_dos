/*****************************************************************************
 * File: VIEW.C
 * Purpose: Viewport text renderer, visual line-wrap metadata synchronizer,
 *          and title-bar filename updater.
 * Programmer: YanChao (2011.9.11-2011.9.12)
 *
 * Design & Optimization Details:
 *   1. Dirty-flag driven: only repaints the viewport when `editFlag == 1` and
 *      the title bar when `file.fileflag == 1`.
 *   2. ASCII Run Batching: BGI's default 8x8 bitmap font advances 8 px per
 *      character (matching `CHARSIZE`). Collecting consecutive `flag == 1`
 *      nodes on the same row into `asciiBuf` reduces `outtextxy()` calls by
 *      up to 75x per line with pixel-identical output.
 *   3. `firstInLine` Resynchronization: Every edit can shift soft-wrap points
 *      on subsequent lines. ViewAll() clears `pNode->next->firstInLine = 0`
 *      before each wrap/newline test and continues scanning past EDITERSCREENY2
 *      (without drawing) so `lineCount` and `firstInLine` remain exact across
 *      the entire tail of the document.
 *****************************************************************************/
#include<graphics.h>
#include<stdlib.h>
#include"h/hanzi.h"
#include"h/struct.h"
#include"h/malloc.h"
#include"h/common.h"
#include"h/spell.h"
#include"h/view.h"

/*****************************************************************************
 * Function: ViewAll
 * Purpose:  Redraw visible document contents from `pageFirst` and refresh the
 *           title-bar filename whenever their respective dirty flags are set.
 *
 * @param pEdit Master editor context.
 * @post        - When `editFlag == 1`: clears viewport (X=5..614, Y=50..450),
 *                draws visible text, updates `firstInLine` and `lineCount`,
 *                clears `editFlag = 0`, sets `cursorFlag = 1`, and resets
 *                `state.mouseFlag = 0` (since the mouse arrow was painted over).
 *              - When `file.fileflag == 1`: repaints active filename at (5,15)
 *                and resets `file.fileflag = 0`.
 * @complexity  O(R) where R = number of nodes from `pageFirst` to `last`.
 *****************************************************************************/
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

		/* Repaint white canvas inside viewport (leaving 20px scrollbar intact) */
		setcolor(BLACK);
		settextjustify(LEFT_TEXT, BOTTOM_TEXT);
		setfillstyle(SOLID_FILL, WHITE);
		bar(EDITERSCREENX1, EDITERSCREENY1, EDITERSCREENX2-20, EDITERSCREENY2);
		pEdit->state.mouseFlag=0;
		pEdit->editFlag=0;
		pEdit->cursorFlag=1;

		pEdit->lineCount=0;
		if(pNode!=NULL)
		{
			pNode->firstInLine=1;
		}
		while(pNode!=NULL)
		{
			/* Reset next node's line-start flag before evaluating wrap/newline */
			if(pNode->next!=NULL)
			{
				pNode->next->firstInLine=0;
			}

			if(pNode->flag==0)
			{
				/* Hard newline: flush buffered ASCII run and move to next row */
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
				/* Buffer visible ASCII glyphs into a contiguous horizontal run */
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
				/* Flush pending ASCII run before plotting a 16x16 Hanzi bitmap */
				if(asciiLen>0)
				{
					asciiBuf[asciiLen]='\0';
					outtextxy(asciiStartX, y, asciiBuf);
					asciiLen=0;
				}
				if(y<=EDITERSCREENY2)
				{
					/* Special pair (' ', ' ') with flag=2 draws hardcoded glyph '垚' */
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

			/* Soft wrap at column CHARINLINE (605 px) to match GetNodePosi() */
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
		/* Flush any remaining ASCII characters on the final visible line */
		if(asciiLen>0 && y<=EDITERSCREENY2)
		{
			asciiBuf[asciiLen]='\0';
			outtextxy(asciiStartX, y, asciiBuf);
		}
	}

	/* Redraw top-left title-bar filename whenever Open/New/SaveAs changes it */
	if(pEdit->file.fileflag==1)
	{
		setfillstyle(SOLID_FILL, LIGHTBLUE);
		bar(0, 0, 260, 25);
		setcolor(YELLOW);
		settextjustify(LEFT_TEXT, TOP_TEXT);
		outtextxy(5, 15, pEdit->file.fname);
		pEdit->file.fileflag=0;
	}
}
