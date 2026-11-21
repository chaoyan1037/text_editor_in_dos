/*********************************************
function: view

programmer: YanChao

time：2011.9.11-2011.9.12
***********************************************/
#include<graphics.h>
#include<stdlib.h>
#include"h/hanzi.h"
#include"h/struct.h"
#include"h/malloc.h"
#include"h/common.h"
#include"h/spell.h"
#include"h/view.h"


/***********show what is in linked list**********/
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
			if(pNode->next!=NULL)
			{
				pNode->next->firstInLine=0;
			}

			if(pNode->flag==0)
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
					pNode->next->firstInLine=1; /* 标记行首节点 */
					pEdit->lineCount++;
				}
			}
			else if(pNode->flag==1)
			{
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
				if(asciiLen>0)
				{
					asciiBuf[asciiLen]='\0';
					outtextxy(asciiStartX, y, asciiBuf);
					asciiLen=0;
				}
				if(y<=EDITERSCREENY2)
				{
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
		if(asciiLen>0 && y<=EDITERSCREENY2)
		{
			asciiBuf[asciiLen]='\0';
			outtextxy(asciiStartX, y, asciiBuf);
		}
	}
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
