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
#include<string.h>
#include"h/common.h"
#include"h/language.h" 
#include"h/struct.h"
#include"h/node.h"
#include"h/mouse.h"
#include"h/malloc.h"
#include"h/hanzi.h"
#include"h/funckey.h"

void MoveLeft(struct EDITTYPE *pEdit);
void MoveRight(struct EDITTYPE *pEdit);
int FindChar(struct EDITTYPE *pEdit, char *pGoal);
int FindString(struct EDITTYPE *pEdit, char *pGoal);
int FindHanziString(struct EDITTYPE *pEdit, char *pGoal);
int Find(struct EDITTYPE *pEdit, char *pGoal);
void Dialog(int x0, int y0, int x1, int y1, struct EDITTYPE *pEdit);
void ChoosenOne(struct EDITTYPE *pEdit, int x, int y);

/******exit editer,free all nod*****/
void ExitEditer(struct EDITTYPE *pEdit)
{
	delay(1000);
	closegraph();
	printf("thanks for using.\n");
	FreeAllNode(pEdit);
	ClearClip(pEdit);
	getch();
}

/******move cursor to left right or up down*******/
void MoveCursor(struct EDITTYPE *pEdit)
{
	int x=pEdit->x, y=pEdit->y;
	setcolor(WHITE);
	setwritemode(COPY_PUT);
	line(x, y, x+8, y);
	pEdit->cursorFlag=1; /*flag 1 reget cursor position*/
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
		if(y==EDITERSCREENY1+2*CHARHIGH&&x==EDITERSCREENX1)/*页首节点*/
		{
			pEdit->current=NULL;
		}
		else if(y>=EDITERSCREENY1+2*CHARHIGH)
		{
			y-=CHARHIGH;
			if( ConvertNodePosi(pEdit, x, y) );  /*******do nothing else*********/
			else
			{
				int xx=x-CHARSIZE;
				for( ;xx>=EDITERSCREENX1 ; xx-=CHARSIZE )
				{
					if(ConvertNodePosi(pEdit, xx, y)==1)
					break;
				}
			}
		}
		else if(y==EDITERSCREENY1+CHARHIGH)
		{
			struct NODE *pNode=pEdit->pageFirst->last;
			for( ; pNode!=NULL&&pNode->firstInLine!=1; pNode=pNode->last);
			if(pNode!=NULL)
			{
				pEdit->pageFirst=pNode;
				pEdit->editFlag=1;
			}
			else 		
			{
				pEdit->pageFirst=pEdit->start;
				pEdit->editFlag=1;
			}
		}
	}
	else if(pEdit->state.key==DOWN)
	{		
		if(y<EDITERSCREENY2) 
		{
			y+=CHARHIGH;
			if( ConvertNodePosi(pEdit, x, y) );
			else
			{
				int xx=x-CHARSIZE;
				for( ;xx>=EDITERSCREENX1 ; xx-=CHARSIZE )
				{
					if( ConvertNodePosi(pEdit, xx, y) )
					{
						if( y==EDITERSCREENY2 )
						{
							pEdit->current=pEdit->current->next;
						}
						break;
					}
				}
			}
		}

		else if(y==EDITERSCREENY2)
		{			
			int number=0;
			struct NODE *pNode=pEdit->pageFirst;
			for( ; pNode!=NULL; pNode=pNode->next)  
			{
				if(pNode->firstInLine==1)
				{	
					number++;
				}
			}
			if(number>=CHARINROW+1)
			{
				pNode=pEdit->pageFirst->next;
				for( ; pNode&&pNode->firstInLine!=1; pNode=pNode->next);
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
	if(pEdit->copyCutFlag==1) /*清空剪切操作后剪切板的内容*/
	{
		ClearClip(pEdit);
		pEdit->copyCutFlag=0;
	}
	if(pEdit->clipFlag==1) //清空剪切板
	{
		pEdit->clipStart=NULL;
		pEdit->clipLast=NULL;
		pEdit->clipFlag=0;
	}
	if(pEdit->state.key==CTRL_LEFT)
	{
		MoveLeft(pEdit);
	}
	/*pEdit->state.key==CTRL_RIGHT*/
	else
	{
		MoveRight(pEdit);
	}
}
/********move black and white left*********/
void MoveLeft(struct EDITTYPE *pEdit)
{
    int x=0, y=0;
    if( pEdit->current!=NULL)
    {
		if(pEdit->current!=pEdit->pageFirst)
		{
			if( GetNodePosi(pEdit, pEdit->current, &x, &y) )
			{

				if(x==EDITERSCREENX1)
				{
					x+=CHARSIZE*CHARINLINE;
					y-=CHARHIGH;
				}

				setfillstyle(SOLID_FILL, BLUE);
				setcolor(WHITE);
				if(pEdit->current->flag==1)
				{
					bar(x-CHARSIZE, y-CHARHIGH, x, y);
					outtextxy(x-CHARSIZE, y, pEdit->current->data);
				}
				else if(pEdit->current->flag==0)
				{
					GetNodePosi(pEdit, pEdit->current->last, &x, &y);
					bar(x, y-CHARHIGH, x+CHARSIZE, y);
				}
				else if(pEdit->current->flag==2)
				{

					bar(x-2*CHARSIZE, y-CHARHIGH, x, y);
					Printfhz(x-2*CHARSIZE, y-CHARHIGH, pEdit->current->data, WHITE, pEdit);
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
	    ChoosenOne( pEdit, x, y);
	    /*
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
                Printfhz(x, y-CHARHIGH, pEdit->current->data, WHITE, pEdit);
            }*/
            if(pEdit->clipLast==NULL)
            {
                pEdit->clipLast=pEdit->current;
            }
            pEdit->clipStart=pEdit->pageFirst;
        }
    }
}

/********move black and white right*********/
void MoveRight(struct EDITTYPE *pEdit)
{
    int x=0, y=0;
    if( pEdit->current==NULL&&pEdit->pageFirst!=NULL)
    {
        setfillstyle(SOLID_FILL, BLUE);
        setcolor(WHITE);
        x=EDITERSCREENX1;
		y=EDITERSCREENY1+CHARHIGH;
	pEdit->current=pEdit->pageFirst;
	ChoosenOne( pEdit, x, y);
	/*if(pEdit->pageFirst->flag==1)
	{
	    bar(x, y-CHARHIGH, x+CHARSIZE, y);
	    outtextxy(x, y, pEdit->pageFirst->data);
	}
	else if(pEdit->pageFirst->flag==0)
	{
	    bar(x, y-CHARHIGH, x+CHARSIZE, y);
	}
	else if(pEdit->pageFirst->flag==2)
	{
	    bar(x, y-CHARHIGH, x+2*CHARSIZE, y);
	    Printfhz(x, y-CHARHIGH, pEdit->pageFirst->data, WHITE, pEdit);
	}*/

	pEdit->clipStart=pEdit->pageFirst;
	pEdit->clipLast=pEdit->pageFirst;

    }
    else if( pEdit->current!=NULL&&pEdit->current!=pEdit->last)
    {
        if(pEdit->current!=pEdit->last)
        {
	    pEdit->current=pEdit->current->next;
	    if( GetNodePosi(pEdit, pEdit->current->last, &x, &y) )
	    {
		ChoosenOne( pEdit, x, y);
		/*
		setfillstyle(SOLID_FILL, BLUE);
		setcolor(WHITE);
		if(pEdit->current->next->flag==1)
		{
		    bar(x, y-CHARHIGH, x+CHARSIZE, y);
		    outtextxy(x, y, pEdit->current->next->data);
		}
		else if(pEdit->current->next->flag==0)
		{
		    bar(x, y-CHARHIGH, x+CHARSIZE, y);
		}
		else if(pEdit->current->next->flag==2)
		{
		    bar(x, y-CHARHIGH, x+2*CHARSIZE, y);
		    Printfhz(x, y-CHARHIGH, pEdit->current->next->data, WHITE, pEdit);
		}*/
	    }
	    if(pEdit->clipStart==NULL)
	    {
		pEdit->clipStart=pEdit->current;
	    }
	    pEdit->clipLast=pEdit->current;

	}
    }
}

/*****************paste********************/
void Paste(struct EDITTYPE *pEdit)
{
	if(pEdit->copyActive==1)
	{
		struct NODE *pNode=pEdit->clipStart;
		while(pNode!=NULL)
		{
			pEdit->editFlag=1;
			pEdit->cursorFlag=1;

			if(pEdit->current==pEdit->last)
			{
				if(pEdit->current==NULL)
				{
					struct NODE *p=MallocNode(pEdit);
					*p=*(pEdit->clipStart);
					pEdit->current=pEdit->pageFirst=pEdit->last=p;
				}
				else
				{
					AddNode(pEdit, &pNode->data[0]);
			        	pEdit->current->flag=pNode->flag;
				
					if(pNode==pEdit->clipLast)
					{
						break;
					}
					pNode=pNode->next;
				}
			}
			else   /*pEdit->current!=pEdit->last*/
			{
				InsertNode(pEdit, &pNode->data[0]);
			        pEdit->current->flag=pNode->flag;
				
				if(pNode==pEdit->clipLast)
				{
					break;
				}
				pNode=pNode->next;
			}
		}
	}
}
/****************copy and cut****************/
void CopyCut(struct EDITTYPE *pEdit)
{
	if( (pEdit->clipStart)&&(pEdit->clipLast) )
	{
		if(pEdit->clipStart==pEdit->pageFirst)
		{
			pEdit->pageFirst=pEdit->clipLast->next;
			pEdit->pageFirst->last=NULL;
			pEdit->current=NULL;

			if(pEdit->pageFirst==NULL)
			{
				pEdit->last=pEdit->current=NULL;		
			}	
		}
		else if(pEdit->clipLast==pEdit->last)
		{
			pEdit->last=pEdit->clipStart->last;
			pEdit->last->next=NULL;
			pEdit->current=pEdit->last;
		
		}
		else
		{
			pEdit->clipStart->last->next=pEdit->clipLast->next;
			pEdit->clipLast->next->last=pEdit->clipStart->last;
			pEdit->current=pEdit->clipStart->last;
		}
		pEdit->copyActive=1;
		
		pEdit->copyCutFlag=1;
		pEdit->cursorFlag=1;
		pEdit->editFlag=1;
		if(pEdit->state.key==BACKSPACE)
		{
			ClearClip(pEdit);
			pEdit->copyActive=0;
			pEdit->cursorFlag=0;
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

/*********find a char or hanzi since next goal*********/
/**************not black and white chosen*************/
int FindChar(struct EDITTYPE *pEdit, char *pGoal) 
{
	pGoal[1]='\0';
    if(pEdit->pageFirst==NULL)
    {
        return 0; /*list is null*/
    }
    else
    {
    	if(pEdit->current==NULL)
        {
            pEdit->current=pEdit->pageFirst;
        }
	else 
	{    
	    pEdit->current=pEdit->current->next;
	}
	/**********find a char***********/
    	if( pGoal[0]>=0x20&&pGoal[0]<=0x7e)
    	{        
	    for(; pEdit->current!=NULL; pEdit->current=pEdit->current->next)
	    {
                if(pEdit->current->flag==1)
	        {		
                    if(pGoal[0]==pEdit->current->data[0])
		    {
	                return 1;
		    }	
	    	    else return 0;	    
		}
		else return 0;
	    }
	    /************avoid find circularly*******/
	    if(pEdit->current==NULL) 
            {
                pEdit->current=pEdit->last;
            }
	    return 0;
        }
	else return 0;
    }
}
/**************function find*****************/
int FindString(struct EDITTYPE *pEdit, char *pGoal)
{
    int flag=0, i=0;
    char unit[2]="h";
    struct NODE *pNode;
    unit[0]=pGoal[0];
    while(1)   
    {
        if(FindChar(pEdit, unit)==1)  /*current is the first one*/
        {
       	    pNode=pEdit->current;
	    pEdit->clipStart=pEdit->current;
	    for(i=0; pGoal[i]!='\0'; i++)
	    {
		if(pNode==NULL)
		{
		    flag=0;
		    pEdit->clipStart=NULL;  /*防止未经复制和剪切操作复制出现错误*/
		    break;
		}
		else if(pNode->flag!=1||pNode->data[0]!=pGoal[i])
		{
	            flag=0;
		    pEdit->clipStart=NULL;
	            break;
		}
		flag=1;
		pEdit->clipLast=pNode;
		pNode=pNode->next;
	    }
	}
        if(flag==1)
	{
	    return 1;    
	}
	else if(pEdit->current==pEdit->last)
	{
	    return 0;   
	}	    
    }
}
/****************find hanzistring*******************/
int FindHanziString(struct EDITTYPE *pEdit, char *pGoal)
{
    int flag=0, i=0;
    char unit[3]="h";
    struct NODE *pNode;
    unit[0]=pGoal[0];
    unit[1]=pGoal[1];
    while(1)   
    {
        if(FindChar(pEdit, unit)==1)  /*current is the first one*/
        {
       	    pNode=pEdit->current;
	    for(i=0; pGoal[i]!='\0'; i+=2)
	    {
		if(pNode==NULL)
		{
		    flag=0;
		    break;
		}
		else if(pNode->flag!=2||pNode->data[0]!=pGoal[i]||pNode->data[1]!=pGoal[i+1])
		{
	            flag=0;
	            break;
		}
		flag=1;
		pNode=pNode->next;
	    }
	}
        if(flag==1)
	{
	    return 1;    
	}
	else if(pEdit->current==pEdit->last)
	{
	    return 0;   
	}	    
    }
}
/********************find function*************************/
int Find(struct EDITTYPE *pEdit, char *pGoal)
{
    int i=0, x=0, y=0;
    int okFlag=0;
    setfillstyle(SOLID_FILL, BLUE);
    setcolor(WHITE);

    if(pGoal[0]>=0x20&&pGoal[0]<=0x7e)	
    {
        okFlag=FindString(pEdit, pGoal);
    }
    else
    {
        okFlag=FindHanziString(pEdit, pGoal);
    }

    if(okFlag==1)
    {	
        if( GetNodePosi(pEdit, pEdit->current, &x, &y) )
        {
            for(i=0; i<strlen(pGoal); )
            {	        
                if(i>0)  
		{    
		    pEdit->current=pEdit->current->next;
		}
		i+=pEdit->current->flag;
		if(pEdit->current->flag==1)
                {
                    if(x==EDITERSCREENX1)
	            {
			int x1=EDITERSCREENX1+(CHARINLINE-1)*CHARSIZE;
			int y1=y-CHARHIGH;			
			bar(x1, y1-CHARHIGH, x1+CHARSIZE, y1);
                        outtextxy(x1, y1, pEdit->current->data);
		    }			    
		    else   
		    {
			bar(x-CHARSIZE, y-CHARHIGH, x, y);
                        outtextxy(x-CHARSIZE, y, pEdit->current->data);
		    }
                }             
                else if(pEdit->current->flag==2)
                {
                    bar(x-2*CHARSIZE, y-CHARHIGH, x, y);
  	            Printfhz(x-2*CHARSIZE, y-CHARHIGH, pEdit->current->data, WHITE, pEdit);
	        }
		x+=CHARSIZE*pEdit->current->flag;
		if( x>(CHARINLINE-1)*CHARSIZE+EDITERSCREENX1 )
		{
		    x=EDITERSCREENX1;
		    y+=CHARHIGH;
		}
            }
	}
        return 1;
    }
    else return 0;   
}
/*****************input goal and find*******************/
void  FindFunction(struct EDITTYPE *pEdit)
{
    int i=0;
    Dialog(240, 160, 400, 240, pEdit);  
    Printfhz(250, 165, "查找", RED, pEdit);
    while(1)
    {       
	if(bioskey(1)!=0)
	{
	    long int key=bioskey(0);
	    int ascii=key&0x00ff;
   	    if(ascii>=0x20&&ascii<=0x7e&&i<13)
	    {
	        pEdit->findGoal[i]=ascii;
		pEdit->findGoal[i+1]='\0';
		i++;
	    }
	    if(key==BACKSPACE)
	    {
	        if(i>0)
		{
		    pEdit->findGoal[i-1]='\0'; 
		    i--;
		    setfillstyle(SOLID_FILL, LIGHTGREEN);
		    bar(250, 185, 389, 205); 
		    setcolor(WHITE);   
  		    rectangle(250, 185, 389, 205); 
   		    WhiteBlack(250, 185, 389, 205, BLACK);
		}   
	    }
	    else if(key==ENTER) //&&pEdit->findGoal[0]!='\0')
	    {
		pEdit->state.functionActive=5;
		break;
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
    if( Find(pEdit, &pEdit->findGoal[0])==0 ) /*Find() includes blackwhite chosen */
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
	//覆盖查找块，重新输出替换
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
/********************cancle find function**************************/
void CancleFind(struct EDITTYPE *pEdit)
{
	pEdit->findGoal[0]='\0';       
    pEdit->state.functionActive=0; 
	pEdit->editFlag=1;            
}
/******************find and replace******************/
void FindAndReplace(struct EDITTYPE *pEdit)
{
    int i=0, j=0, inputFlag=0;
    Dialog(240, 130, 400, 240, pEdit);  
    //setcolor(LIGHTGREEN);
    //line(240, 130, 240, 240);
    //line(240, 130, 400, 130);
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
	    int ascii=key&0x00ff;
   	    if(ascii>=0x20&&ascii<=0x7e)
	    {
	        if(i<12&&inputFlag==0)
		{
		    pEdit->findGoal[i]=ascii;
		    pEdit->findGoal[i+1]='\0';
		    i++;
		}
		else if(j<12&&inputFlag==1)
		{
		    pEdit->replaceGoal[j]=ascii;
		    pEdit->replaceGoal[j+1]='\0';
		    j++;
		}
	    }
	    if(key==BACKSPACE)
	    {
	        if(i>0&&inputFlag==0)
		{
		    pEdit->findGoal[i-1]='\0'; 
		    i--;
		    setfillstyle(SOLID_FILL, LIGHTGREEN); 
		    bar(288, 155, 389, 175);
		    setcolor(WHITE);  
  		    rectangle(288, 155, 389, 175); 
   		    WhiteBlack(288, 155, 389, 175, BLACK); 	
		}
	        else if(j>0&&inputFlag==1)
		{
		    pEdit->replaceGoal[j-1]='\0'; 
		    j--;
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
		else if(inputFlag==1)
		{
		    pEdit->state.functionActive=6;
		    pEdit->state.mouseFlag=0; 
		    break;
		}
	    }

	    if(inputFlag==0)
	    {
	        setcolor(BLACK);
                outtextxy(290, 175, &pEdit->findGoal[0]);
            }
            else if(inputFlag==1)
	    {
	        setcolor(BLACK);
                outtextxy(290, 205, &pEdit->replaceGoal[0]);
	    }	    
	}
    }
}
/**********************replace finded goal *****************************/
void ReplaceFunction(struct EDITTYPE *pEdit)
{
    if(pEdit->clipStart!=NULL&&pEdit->clipLast!=NULL)
    {
	int i=0;
	struct NODE *pNode=pEdit->clipStart;
	for(i=0; pEdit->replaceGoal[i]!='\0'; i++ )
	{
	    pNode->data[0]=pEdit->replaceGoal[i];	    
	    if(pNode==pEdit->clipLast)
            {
	        pEdit->current=pEdit->clipLast;
		break; 
	    }
	    pEdit->current=pNode;
            pNode=pNode->next;
	}

	if(pEdit->replaceGoal[i]!='\0') /*如果查找到的字符串长度小于替换字符*/
	{
	    for(i=i+1; pEdit->replaceGoal[i]!='\0'; i++) 
	    {
	        pEdit->state.ascii=pEdit->replaceGoal[i];
	        GetEnglish(pEdit);
	    }
	}
	else  /*查找到的字符串长度大于替换字符*/
	{
	    if(pNode->last!=pEdit->clipLast)
	    {
	        while(pNode!=pEdit->clipLast)
	        {
	            DeleteNode(pNode, pEdit);
		    pNode=pNode->next;
                }
		DeleteNode(pNode, pEdit);
		pNode=NULL;
	    }
	}

	pEdit->editFlag=1;
	pEdit->replaceGoal[0]='\0';
	pEdit->state.functionActive=0;
    }
    pEdit->clipLast=NULL;
    pEdit->clipStart=NULL;
}
/********************Cancle replace function*************************/
void CancleReplace(struct EDITTYPE *pEdit)
{
    pEdit->clipLast=NULL;
    pEdit->clipStart=NULL;
    pEdit->findGoal[0]='\0';
    pEdit->replaceGoal[0]='\0';
    pEdit->state.functionActive=0;
    pEdit->editFlag=1;
}

/****************************selete all******************************/
void AllChosen(struct EDITTYPE *pEdit)
{
    struct NODE *pNode=pEdit->pageFirst;
    struct NODE *pageLast=NULL;
    struct NODE *temp=pEdit->current;
    int number=0, count=0;
    int x=EDITERSCREENX1, y=EDITERSCREENY1+CHARHIGH;
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
    for( ; pNode; pNode=pNode->next)
    {
        count++;
        if(pNode->flag==0||count==CHARINLINE)
        {
           pageLast=pNode;
           break;    
	}
    }

    pEdit->current=pEdit->pageFirst; /*从第一个节点开始*/
    ChoosenOne( pEdit, x, y);
    while(pEdit->current!=pEdit->last&&pEdit->current!=pageLast)
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
        Printfhz(x, y-CHARHIGH, pEdit->current->data, WHITE, pEdit);
    }
}
/***************Mouse click to choose string*********************************/
void MouseBWChosen(struct EDITTYPE *pEdit)
{
	int i=0;
	if(pEdit->state.button)
	{
	    for(i=0; i<50; i++) /*judge*/
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