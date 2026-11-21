/**************************************************
function：get in a function key, then do command

programmer:YanChao

time：2011.9.15-2011.11.2
***********************************************/
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

/******exit editer,free all nodes*****/
void ExitEditer(struct EDITTYPE *pEdit)
{
	delay(1000);
	closegraph();
	printf("thanks for using.\n");
	FreeAllNode(pEdit);
	close_hz(pEdit);
	getch();
	exit(0);
}

/******move cursor to left right or up down*******/
void MoveCursor(struct EDITTYPE *pEdit)
{
	int x=pEdit->x, y=pEdit->y;
	setcolor(WHITE);
	setwritemode(COPY_PUT);
	line(x, y, x+8, y);
	pEdit->cursorFlag=1; /* flag 1 reget cursor position */

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
		if(y==EDITERSCREENY1+2*CHARHIGH && x==EDITERSCREENX1) /* 页首节点 */
		{
			pEdit->current=NULL;
		}
		else if(y>=EDITERSCREENY1+2*CHARHIGH)
		{
			y-=CHARHIGH;
			MoveCursorVertical(pEdit, x, y);
		}
		else if(y==EDITERSCREENY1+CHARHIGH)
		{
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

/***************black and white******************/
void BlackWhiteChoosen(struct EDITTYPE *pEdit)
{
	if(pEdit->copyCutFlag==1) /* 清空剪切/复制操作后剪切板的内容 */
	{
		ClearClip(pEdit);
		pEdit->copyActive=0;
	}
	if(pEdit->clipFlag==1) /* 清空选中状态 */
	{
		pEdit->clipStart=NULL;
		pEdit->clipLast=NULL;
		pEdit->clipFlag=0;
	}
	if(pEdit->state.key==CTRL_LEFT)
	{
		MoveLeft(pEdit);
	}
	else /* pEdit->state.key==CTRL_RIGHT */
	{
		MoveRight(pEdit);
	}
}

/********move black and white left*********/
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

/********move black and white right*********/
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

/*****************paste********************/
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

/****************copy and cut****************/
void CopyCut(struct EDITTYPE *pEdit)
{
	if( pEdit->copyCutFlag==0 && pEdit->clipStart!=NULL && pEdit->clipLast!=NULL )
	{
		struct NODE *pPrev=pEdit->clipStart->last;
		struct NODE *pNext=pEdit->clipLast->next;
		int pageFirstCut=0;
		struct NODE *pScan=pEdit->clipStart;

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

		if(pPrev==NULL || (pEdit->pageFirst!=NULL && pEdit->pageFirst->last==pPrev))
		{
			pEdit->current=NULL;
		}
		else
		{
			pEdit->current=pPrev;
		}

		/* 彻底断开剪切板与主链表的联系 */
		pEdit->clipStart->last=NULL;
		pEdit->clipLast->next=NULL;

		pEdit->copyActive=1;
		pEdit->copyCutFlag=1;
		pEdit->cursorFlag=1;
		pEdit->editFlag=1;

		if(pEdit->state.key==BACKSPACE)
		{
			ClearClip(pEdit);
			pEdit->copyActive=0;
		}
	}
}

/************press backspace to delete single node***************/
void BackSpace(struct EDITTYPE *pEdit)
{
	if(pEdit->current!=NULL)
	{
		DeleteNode(pEdit->current, pEdit);
		pEdit->editFlag=1;
		pEdit->cursorFlag=1;
	}
}

/********************find function*************************/
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

	if(pEdit->current==NULL)
	{
		pStart=pEdit->pageFirst;
	}
	else
	{
		pStart=pEdit->current->next;
	}

	for( ; pStart!=NULL; pStart=pStart->next)
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

	if(found)
	{
		int x=EDITERSCREENX1, y=EDITERSCREENY1+CHARHIGH;
		const int maxX=CHARINLINE*CHARSIZE+EDITERSCREENX1;
		struct NODE *pCur=pEdit->pageFirst;

		pEdit->findStart=pStart;
		pEdit->findLast=pEnd;
		pEdit->current=pEnd;

		/* 计算起始节点位置并高亮整个匹配串 */
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

/*****************input goal and find*******************/
void FindFunction(struct EDITTYPE *pEdit)
{
	int i=0;
	pEdit->findGoal[0]='\0';
	Dialog(240, 160, 400, 240, pEdit);
	Printfhz(250, 165, "查找", RED, pEdit);
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

/********************find specific goal**********************/
void FingGoal(struct EDITTYPE *pEdit)
{
	setwritemode(COPY_PUT);
	if( Find(pEdit, &pEdit->findGoal[0])==0 ) /* Find() includes blackwhite chosen */
	{
		setfillstyle(SOLID_FILL, LIGHTGRAY);
		if(pEdit->state.functionActive==5)
		{
			bar(250, 164, 283, 182);
			setcolor(BLACK);
			Printfhz(250, 165, "提示：未找到！", RED, pEdit);
		}
		else if(pEdit->state.functionActive==6)
		{
			bar(241, 134, 283, 152);
			Printfhz(250, 135, "提示：未找到！", RED, pEdit);
			pEdit->replaceGoal[0]='\0';

			setfillstyle(SOLID_FILL, LIGHTGRAY);
			bar(241, 206, 320, 239);
			setcolor(BLACK);
			rectangle(258, 214, 314, 232);
			WhiteBlack(258, 214, 314, 232, WHITE);
			Printfhz(260, 215, "确定", RED, pEdit);
			setcolor(RED);
			outtextxy(293, 231, "(S)");
		}
		pEdit->findGoal[0]='\0';
		pEdit->state.functionActive=1;
	}
	else if(pEdit->state.functionActive==6)
	{
		pEdit->state.functionActive=7;
		setwritemode(COPY_PUT);
		/* 覆盖查找块，重新输出替换 */
		setfillstyle(SOLID_FILL, LIGHTGRAY);
		bar(241, 206, 320, 239);
		setcolor(BLACK);
		rectangle(258, 214, 314, 232);
		WhiteBlack(258, 214, 314, 232, WHITE);
		Printfhz(260, 215, "替换", RED, pEdit);
		setcolor(RED);
		outtextxy(293, 231, "(R)");
	}
}

/**********************find_dialog*****************************/
void Dialog(int x0, int y0, int x1, int y1, struct EDITTYPE *pEdit)
{
	setfillstyle(SOLID_FILL, LIGHTGRAY);
	bar(x0, y0, x1, y1);
	button3d(x0, y0, x1, y1);

	setfillstyle(SOLID_FILL, LIGHTGREEN);
	bar(250, 185, 389, 205);

	setcolor(WHITE);
	rectangle(250, 185, 389, 205);
	WhiteBlack(250, 185, 389, 205, BLACK);

	setcolor(BLACK);
	rectangle(258, 214, 314, 232);
	WhiteBlack(258, 214, 314, 232, WHITE);

	setcolor(BLACK);
	rectangle(323, 214, 379, 232);
	WhiteBlack(323, 214, 379, 232, WHITE);

	Printfhz(260, 215, "查找", RED, pEdit);
	Printfhz(325, 215, "取消", RED, pEdit);

	setcolor(RED);
	outtextxy(293, 231, "(F)");
	outtextxy(357, 231, "(C)");
}

/********************cancel find function**************************/
void CancleFind(struct EDITTYPE *pEdit)
{
	pEdit->findStart=NULL;
	pEdit->findLast=NULL;
	pEdit->findGoal[0]='\0';
	pEdit->state.functionActive=0;
	pEdit->editFlag=1;
}

/******************find and replace******************/
void FindAndReplace(struct EDITTYPE *pEdit)
{
	int i=0, j=0, inputFlag=0;
	pEdit->findGoal[0]='\0';
	pEdit->replaceGoal[0]='\0';
	Dialog(240, 130, 400, 240, pEdit);

	/**************替换框***************/
	setfillstyle(SOLID_FILL, LIGHTGRAY);
	bar(250, 185, 288, 205);
	setcolor(BLACK);
	line(288, 185, 288, 205);

	/**************查找框***************/
	setfillstyle(SOLID_FILL, LIGHTGREEN);
	bar(288, 155, 389, 175);
	setcolor(WHITE);
	rectangle(288, 155, 389, 175);
	WhiteBlack(288, 155, 389, 175, BLACK);

	setcolor(RED);
	/**************文字输出***************/
	Printfhz(250, 135, "替换", RED, pEdit);
	Printfhz(250, 159, "查找", RED, pEdit);
	Printfhz(250, 189, "替换", RED, pEdit);
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

/**********************replace found goal *****************************/
void ReplaceFunction(struct EDITTYPE *pEdit)
{
	if(pEdit->findStart!=NULL && pEdit->findLast!=NULL)
	{
		int i=0;
		struct NODE *pNode=pEdit->findStart;
		struct NODE *pEnd=pEdit->findLast;

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

		if(pEdit->replaceGoal[i]!='\0') /* 查找到的字符串长度小于替换字符 */
		{
			for( ; pEdit->replaceGoal[i]!='\0'; i++)
			{
				pEdit->state.ascii=(unsigned char)pEdit->replaceGoal[i];
				GetEnglish(pEdit);
			}
		}
		else if(pNode!=NULL) /* 查找到的字符串长度大于替换字符 */
		{
			struct NODE *pNext;
			while(pNode!=NULL)
			{
				int isLast=(pNode==pEnd);
				pNext=pNode->next;
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

/********************Cancel replace function*************************/
void CancleReplace(struct EDITTYPE *pEdit)
{
	pEdit->findLast=NULL;
	pEdit->findStart=NULL;
	pEdit->findGoal[0]='\0';
	pEdit->replaceGoal[0]='\0';
	pEdit->state.functionActive=0;
	pEdit->editFlag=1;
}

/****************************select all******************************/
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
	for( ; pNode!=NULL; pNode=pNode->next)
	{
		count+=(pNode->flag==0) ? 1 : pNode->flag;
		if(pNode->flag==0 || count>=CHARINLINE)
		{
			pageLast=pNode;
			break;
		}
	}

	pEdit->current=pEdit->pageFirst; /* 从第一个节点开始 */
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

/*********************blackwhite chosen one **********************/
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

/***************Mouse click to choose string*********************************/
void MouseBWChosen(struct EDITTYPE *pEdit)
{
	int i=0;
	if(pEdit->state.button)
	{
		for(i=0; i<50; i++) /* judge */
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
