/**********************************************
function：establish linked list unit--node

programmer:YanChao

time：2011.9.11-2011.9.15
***********************************************/

#include<graphics.h>
#include<stdlib.h>
#include"h/malloc.h"
#include"h/common.h"
#include"h/struct.h"
#include"h/node.h"

/*****************链表尾追加节点*******************/
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

/***************插入节点*********************/
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

	/* insert after current node */
	if(pEdit->current!=NULL)
	{
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
	/* insert before pageFirst */
	else
	{
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

/**********************删除指定节点*************************/
void DeleteNode( struct NODE *pNode, struct EDITTYPE *pEdit )
{
	struct NODE *pPrev;
	struct NODE *pNext;
	if(pNode==NULL)
	{
		return;
	}
	pPrev=pNode->last;
	pNext=pNode->next;

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

	if(pEdit->pageFirst==pNode)
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

/**************删除链表中所有节点**************/
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

/*********************删除剪切板里的节点************************/
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

/*********************深拷贝选中内容到剪切板************************/
void CopySelection(struct EDITTYPE *pEdit)
{
	struct NODE *pSrc;
	struct NODE *newStart=NULL;
	struct NODE *newLast=NULL;

	if(pEdit->copyCutFlag==1)
	{
		/* 剪切板中已是独立拷贝 */
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

/*******************获取指定节点对应的坐标(在节点的后面)**********************/
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

/************指定光标的坐标对应的节点（单次遍历O(page)）*****************/
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

	/* 允许点击在文档最后一个字符紧右侧 */
	if(pNode==NULL && curY==targetY && targetColX==curX && pEdit->last!=NULL)
	{
		pEdit->current=pEdit->last;
		return 1;
	}

	return 0;
}

/************上下移动光标到目标行最接近列（单次遍历O(page)）*****************/
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

	if(foundLine && lastOnLine!=NULL)
	{
		if(lastOnLine->flag==0)
		{
			pEdit->current=(lastOnLine==pEdit->pageFirst) ? NULL : lastOnLine->last;
		}
		else
		{
			pEdit->current=lastOnLine;
		}
		return 1;
	}

	if(pNode==NULL && curY==targetY && pEdit->last!=NULL)
	{
		pEdit->current=pEdit->last;
		return 1;
	}

	return 0;
}
