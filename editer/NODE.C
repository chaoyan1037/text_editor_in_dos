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

void DeleteNode( struct NODE *pNode, struct EDITTYPE *pEdit );

/*****************链表尾追加节点*******************/
void AddNode( struct EDITTYPE *pEdit, char data[] )
{
    struct NODE *pNode=MallocNode(pEdit);
    pNode->data[0]=data[0];
    pNode->data[1]=data[1];
                                   
    pNode->last=pEdit->current;
    pEdit->current->next=pNode;
    pEdit->current=pNode;
    pEdit->last=pNode;
}

/***************插入节点*********************/
void InsertNode( struct EDITTYPE *pEdit, char data[] )
{
    struct NODE *pNode=MallocNode(pEdit);
    pNode->data[0]=data[0];
    pNode->data[1]=data[1];
                                                             
    /*insert between two nodes*/
    if(pEdit->current!=NULL)
    {
        pNode->next=pEdit->current->next;
        pNode->last=pEdit->current;
                                          
        pEdit->current->next->last=pNode;   
        pEdit->current->next=pNode;

        pEdit->current=pNode;
    }
    /*insert before first nodes*/
    else              
    {          
		pNode->next=pEdit->pageFirst;
        pEdit->pageFirst->last=pNode;
        pNode->last=NULL;

		if(pEdit->start!=pEdit->pageFirst)
		{
			pEdit->pageFirst=pEdit->current=pNode;
        }
		else  pEdit->start=pEdit->pageFirst=pEdit->current=pNode;
    }
}
/**********************删除指定节点*************************/
void DeleteNode( struct NODE *pNode, struct EDITTYPE *pEdit )
{
	struct NODE *pNodeTemp=pNode;
	if(pNode)
	{
		if(pNode->last==NULL)
		{
			pEdit->start=pNode->next;
			pEdit->start->last=NULL;
			pEdit->pageFirst=pEdit->start;
			if(pNode==pEdit->current)
			{
				pEdit->current=NULL;
			}
			free(pNode);
			pNode=NULL;
		}
		else if(pNode->next==NULL)
		{
			pEdit->last=pNode->last;
			pEdit->last->next=NULL;
			if(pEdit->current==pNode)
			{
				pEdit->current=pEdit->last;
			}
			free(pNode);
			pNode=NULL;
		}
		else
		{
			pNode->next->last=pNode->last;
			pNode->last->next=pNode->next;
			free(pNode);
			pNode=NULL;
			pEdit->current=pNodeTemp->last;
		}
	}
}
/**************删除链表中所有节点**************/
void FreeAllNode( struct EDITTYPE *pEdit )
{
	struct NODE *pNode=pEdit->start;
	struct NODE *pTemp=pEdit->start;
	for( ; pNode!=NULL; pNode=pTemp->next)
	{
		pTemp=pNode;
		free(pNode);
	}
	ClearClip(pEdit);
	pEdit->start=NULL;
	pEdit->last=NULL;
	pEdit->current=NULL;
	pEdit->pageFirst=NULL;
}
/*********************删除剪切板里的节点************************/
void ClearClip(struct EDITTYPE *pEdit)
{
	struct NODE *pNode=pEdit->clipStart;
	struct NODE *pTemp;
	while(pNode!=NULL)
	{
		pTemp=pNode;
		free(pNode);
		pNode=NULL;
		if(pTemp==pEdit->clipLast)
		{
			break;
		}
		pNode=pTemp->next;	
	}
	pEdit->clipStart=pEdit->clipLast=NULL;
}
/*******************获取指定节点对应的坐标(在节点的后面)**********************/
int GetNodePosi( struct EDITTYPE *pEdit, struct NODE *pGoalNode, int *xx, int *yy)
{
	struct NODE *pNode=pEdit->pageFirst;
	int x=EDITERSCREENX1;
	int y=EDITERSCREENY1+CHARHIGH;

	if( pNode==NULL||pGoalNode==NULL)
	{
		*xx=EDITERSCREENX1;
		*yy=EDITERSCREENY1+CHARHIGH;
		return 0;
	}

	else
	{
		if(pEdit->pageFirst!=pGoalNode)
		{
			while(pNode!=NULL)
			{
				x=x+(pNode->flag)*CHARSIZE;
				if( x>= CHARINLINE*CHARSIZE+EDITERSCREENX1 )
				{
					x=EDITERSCREENX1; y=y+CHARHIGH;
				}
				if( y>=EDITERSCREENY1+CHARINROW*CHARHIGH )
				{
					y=EDITERSCREENY1+CHARINROW*CHARHIGH;
				}
				if(pNode->flag==0)
				{
					x=EDITERSCREENX1; y=y+CHARHIGH;
				}
				pNode=pNode->next;
				if(pNode==pGoalNode)
				{
					x=x+(pNode->flag)*CHARSIZE;
					if( x >= CHARINLINE*CHARSIZE+EDITERSCREENX1 )
					{
						x=EDITERSCREENX1; y=y+CHARHIGH;
					}					
					if(pNode->flag==0)
					{
						x=EDITERSCREENX1; y=y+CHARHIGH;
					}
					if( y>=EDITERSCREENY1+CHARINROW*CHARHIGH )
					{
						y=EDITERSCREENY1+CHARINROW*CHARHIGH;
					}
					*xx=x;
					*yy=y;
					return 1;
				}
			}
			return 0;
		}
		else
		{
			x=EDITERSCREENX1+(pEdit->pageFirst->flag)*CHARSIZE;
			if(pNode->flag==0)
			{
				x=EDITERSCREENX1;
				y=y+CHARHIGH;
			}
			*xx=x;
			*yy=y;
			return 1;
		}
	}
}
/************指定光标的坐标对应的节点（如果存在）*****************/
int ConvertNodePosi(struct EDITTYPE *pEdit, int x, int y)
{
    int line=1, row=1;
    struct NODE *pNode=pEdit->pageFirst;
    line=1+(y-1-EDITERSCREENY1)/CHARHIGH;
    row=1+(x-EDITERSCREENX1)/CHARSIZE;

    line=line*CHARHIGH+EDITERSCREENY1;
    row=row*CHARSIZE+EDITERSCREENX1;
       
    if(line>EDITERSCREENY1&&line<=EDITERSCREENY2&&row>EDITERSCREENX1&&row<=EDITERSCREENX2)
    {
		for( ; pNode!=NULL; pNode=pNode->next )
        {
            if( GetNodePosi( pEdit, pNode, &x, &y))
			{
                if(pNode->flag==2)
				{
                    if( (x==row||x==row+8)&&y==line )
					{
						pEdit->current=pNode->last;
						return 1;
					}
				}
				else if(pNode->flag==0)
				{
					if(pNode==pEdit->pageFirst)
					{
						if(row==EDITERSCREENX1&&line==EDITERSCREENY1+CHARHIGH)
						{
                            pEdit->current=pNode->last;
							return 1;    
						}
					}
					else if( GetNodePosi( pEdit, pNode->last, &x, &y))
					{
                        if(x+8==row&&y==line)
						{
							pEdit->current=pNode->last;
							return 1;
						}
					}
				}		
				else if(x==row&&y==line)
				{
                    pEdit->current=pNode->last;
					return 1;
				}
			}
        }
    }
    return 0;    
}

