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
#include"h/view.h"


/***********show what is in linked list**********/
void ViewAll(struct EDITTYPE *pEdit)
{
	if( pEdit->editFlag==1 )
	{
		int x=EDITERSCREENX1, y=EDITERSCREENY1+CHARHIGH;
		struct NODE *pNode=pEdit->pageFirst;
		char data[3]="hh";

		setcolor(BLACK);
		settextjustify(LEFT_TEXT,BOTTOM_TEXT);
		setfillstyle(SOLID_FILL, WHITE);
		bar(EDITERSCREENX1, EDITERSCREENY1, EDITERSCREENX2-20, EDITERSCREENY2);
		pEdit->state.mouseFlag=0;
        pEdit->editFlag=0;
		pEdit->cursorFlag=1;

		pEdit->lineCount=0;   
		while(pNode!=NULL)
		{
			if(pNode->flag==0)
			{
				x=EDITERSCREENX1;
				y+=CHARHIGH;
				if(pNode->next!=NULL)
				{
					pNode->next->firstInLine=1;/*标记行首节点*/
					pEdit->lineCount++; 
				}
			}
			else if(pNode->flag==1)
			{
				data[0]=pNode->data[0];
				data[1]='\0';
				if(y<=EDITERSCREENY2)
				{
					outtextxy(x, y, data);
				}
				x=x+CHARSIZE*(pNode->flag);
			}
			else if(pNode->flag==2)
			{
				data[0]=pNode->data[0];
				data[1]=pNode->data[1];
				data[2]='\0';
			        if( (data[0]==' ')&&(data[1]==' ') ) 
				{
					OutputYao(x,y-CHARHIGH,BLACK);
    				}
				else if(y<=EDITERSCREENY2)
				{
					Printfhz(x, y-CHARHIGH, data, BLACK, pEdit);
				}
				x=x+CHARSIZE*(pNode->flag);
			}		
			
			if( x>(CHARINLINE-1)*CHARSIZE+EDITERSCREENX1 )
			{
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
	}
	if(pEdit->file.fileflag==1)
	{
		setfillstyle(SOLID_FILL, LIGHTBLUE);
		bar(0, 0, 100, 25);
		setcolor(YELLOW);
		settextjustify(LEFT_TEXT,TOP_TEXT);
		outtextxy(5, 15, pEdit->file.fname); 
		pEdit->file.fileflag=0;
	}
}






