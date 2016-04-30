/**********************************************
function：edit file..and control..

promagromer:YanChao

time：2011.9.10-2011.11.4
***********************************************/
#include<graphics.h>
#include<stdlib.h>
#include<stdio.h>
#include<conio.h>
#include<bios.h>
#include"h/struct.h"
#include"h/drawmenu.h"
#include"h/mouse.h"
#include"h/common.h"
#include"h/malloc.h"
#include"h/node.h"
#include"h/view.h"
#include"h/language.h"
#include"h/funckey.h"
#include"h/hanzi.h"
#include"h/spell.h"
#include"h/submenus.h"
#include"h/save.h"
#include"h/open.h"
#include"h/inname.h"

void InitStruct(struct EDITTYPE *pEdit);
void SwitchFunction(struct EDITTYPE *pEdit);
void SwitchKey(struct EDITTYPE *pEdit);
void FunctionActiveSwitch(struct EDITTYPE *pEdit);

void main(void)
{
	int inputFlag=0; /*输入标志位，当有按键时置1*/
	struct EDITTYPE edit;
	InitStruct(&edit);
	DrawMainMenu(&edit);
	
	setcolor(YELLOW);
	outtextxy(5, 15, edit.file.fname);
	
	while(1)
	{
		setcolor(BLACK);
		settextjustify(LEFT_TEXT, BOTTOM_TEXT);
		
		/*当光标在最下面一行时按下移键或enter时执行，使光标下移*/
		if(edit.pageDownFlag==1||edit.pageDownFlag==2) 
		{
			edit.state.function=2;
			edit.pageDownFlag=0; 
			edit.state.key=DOWN;
		}

		if(edit.file.modifyFlag==1)
		{
			edit.state.functionActive=11;
			edit.file.modifyFlag=0;
		}
		else if(edit.file.modifyFlag==2)
		{
			edit.state.functionActive=12;
			edit.file.modifyFlag=0;
		}
		
		/*根据不同的function值执行不同操作*/
		if(edit.state.function!=0) 
		{
			SwitchFunction(&edit); 
			edit.state.function=0;
		}

		if(edit.editActive==0) 		/*编辑状态*/
		{
			inputFlag=0; 	/*输入标志位清0*/
			if( bioskey(1)!=0 )
			{
				long int i;
				i=bioskey(0); 
				edit.state.key=i; 	/*取按键的扫描码*/ 
				edit.state.ascii= i&0x00ff; 	/*取按键的ASCII码*/ 
				inputFlag=1; 		/*置标志位1*/
				if(edit.state.functionActive==0)
				{
					SwitchKey(&edit); /*编辑状态下根据不同的key值执行不同操作*/
				}
			}
			if(inputFlag) /*有按键则进入*/
			{	
				/*按键后根据不同的FunctionAvtive值执行不同操作*/
				FunctionActiveSwitch(&edit); 
			}
                        	
		}
		else /*edit.editActive!=0，表示有菜单被选中*/
		{
			Submenu(&edit); 
		}
		ShowPosition(&edit); /*显示行列坐标*/
		MouseKey(&edit);     /*顺序不可随意改变*/
		ViewAll(&edit);      /*显示链表里的所有内容*/
		ShowMouse(&edit);    /*显示鼠标*/
		DrawCursor(&edit);   /*显示光标*/
		RollBar(&edit);      /*滚动条*/
	}

}

void InitStruct(struct EDITTYPE *pEdit)
{
	/****鼠标结构初始化***/
	pEdit->state.x =320; /*记录鼠标位置*/
	pEdit->state.y =240;
	pEdit->state.function=0; 
	pEdit->state.functionActive=0;
	pEdit->state.mouseFlag=0; /*重画编辑器窗后画鼠标标志位*/

	if(!InitMouse())
	{
		closegraph();
		printf("InitMouse failed...\n");
		getch();
		ExitEditer(pEdit);
	}

	pEdit->start=NULL; /*链表操作指针*/
	pEdit->current=NULL;
	pEdit->last=NULL;
	pEdit->clipStart=NULL;
	pEdit->clipLast=NULL;
	pEdit->pageFirst=NULL; /*当前显示页的第一个节点*/

	pEdit->x=EDITERSCREENX1; /*编辑处的位置*/
	pEdit->y=EDITERSCREENY1+CHARHIGH;
	pEdit->line=0;  /*编辑处的行列坐标*/
	pEdit->row=0;	
	pEdit->lineCount=0; /*总行数*/ 
	pEdit->cursorFlag=0; 
	pEdit->editFlag=0; /*编辑标志位，为1则重画编辑框*/
	pEdit->editActive=0; /*编辑状态标志位，为0则为编辑状态*/
	pEdit->copyActive=0; /*复制标志位，为1则可执行复制操作*/
	pEdit->copyCutFlag=0; /*剪切标志位， 为1则可执行剪切后的复制操作*/
	pEdit->clipFlag=0; /*剪切板标志位，为0则为空*/
	pEdit->spell.phz=NULL;
	pEdit->language=0; /*语言标志位*/
	pEdit->pageDownFlag=0; /*光标下移换页的操作标志位*/
	pEdit->tempLineCount=CHARINROW;
    pEdit->tempCurrentCount=CHARINROW;
	
	pEdit->file.modifyFlag=0;
	pEdit->file.fname="NONAME.TXT";
	pEdit->file.fileflag=0;

	pEdit->submenu.menuflag=0; /*菜单标志位，为0则重画子菜单*/
	pEdit->submenu.flag=1;  /*记录当前活跃菜单的活跃子菜单的行号*/
	pEdit->submenu.SubmenuNum[0]=5;  /*各菜单含的子菜单数*/
	pEdit->submenu.SubmenuNum[1]=4;
	pEdit->submenu.SubmenuNum[2]=2;
	pEdit->submenu.SubmenuNum[3]=2;

	pEdit->spell.pcode[2]='\0'; /*初始化字符串*/
	pEdit->findGoal[0]='\0';
	pEdit->replaceGoal[0]='\0';
}
/***********function switch-case************************************/
void SwitchFunction(struct EDITTYPE *pEdit)
{
	switch(pEdit->state.function)
  	{
		case 1: 	Savewarning(pEdit);
					pEdit->state.functionActive=2;
					break;
		case 2:		MoveCursor(pEdit);
					break;
		case 3: 	GetEnter(pEdit);
					break;
		case 4: 	Paste(pEdit);
					break;
		case 5: 	FindFunction(pEdit); /*step1 in find*/
					break;
		case 6: 	GetSubmenu(pEdit);
					break;	
		case 7: 	GetEnglish(pEdit); /*包含各种可显示字符及空格*/				
					break;
		case 8: 	ExchangeLanguage(pEdit);
					ShowLanguage(pEdit);
					break;
		case 9: 	Savewarning(pEdit);
					pEdit->state.functionActive=9;
					break;
		case 10:	InName(pEdit);
					pEdit->state.functionActive=10;
					break;
		case 11:	New(pEdit);
					break;
		case 12:	pEdit->copyActive=1;
					pEdit->cursorFlag=1;
					pEdit->clipFlag=1;
					break;
		case 13:	CopyCut(pEdit);
					break;
		case 14:	if( pEdit->start!=NULL )
					{
						Savewarning(pEdit);					
						pEdit->file.modifyFlag=2;
					}
					else
					{
						New(pEdit);
						pEdit->state.mouseFlag=0;
					}
					break;
		case 15:	InName(pEdit);
					pEdit->state.functionActive=15;
					break;
		case 16:	FingGoal(pEdit); /*step2 in both find and replace*/
					break;
		case 17:	Save(pEdit->file.fname, pEdit);
					break;
		case 18:	if( pEdit->start!=NULL )
					{
						Savewarning(pEdit);					
						pEdit->file.modifyFlag=1;
					}
					else
					{
						Open(pEdit);
					}						 
					break;
		case 19:	Open(pEdit);
					break;
		case 20:	CancleFind(pEdit);
					break;
		case 21:	FindAndReplace(pEdit); /*step1 in replace*/
					break;
		case 22:	ReplaceFunction(pEdit); /*step2 in  replace*/
					break;
		case 23:	CancleReplace(pEdit);
					break;
		case 24:	BlackWhiteChoosen(pEdit);
					break;
		case 25:	AllChosen(pEdit); /*choosen all*/
					break;	 
		default:	break;
	}
}
/*************Getkey switch-case****************************/
void SwitchKey(struct EDITTYPE *pEdit)
{
	switch( pEdit->state.key )
	{
		case ESC:     pEdit->state.function=1;
						break;
		case UP:
		case DOWN:
		case LEFT:
		case RIGHT:     pEdit->state.function=2;
						break;
		case ENTER:     if(pEdit->state.functionActive==0)
						{
							pEdit->state.function=3;
						}
						break;
		case BACKSPACE: BackSpace(pEdit);
						break;
		case CTRL_LEFT:
		case CTRL_RIGHT:pEdit->state.function=24;
						break;
		case CTRL_V:    pEdit->state.function=4;
						break;
		case CTRL_F:    pEdit->state.function=5;
						break;
		case CTRL_R:	pEdit->state.function=21;
						break;
		case CTRL_S:	pEdit->state.function=9;
						break;
		case CTRL_O:    pEdit->state.function=10;
						break;
		case CTRL_N:	pEdit->state.function=14;
						break;
		case    F1:
		case    F2:
		case    F3:
		case    F4:     pEdit->state.function=6;
						break;
		case    F8:     pEdit->state.function=8;
						break;
		case CTRL_C:   	pEdit->state.function=12;
						break;
		case CTRL_X:    pEdit->state.function=13;
						break;
		case CTRL_D:    pEdit->state.function=15;
						break;
		case CTRL_A:    pEdit->state.function=25;
						break;
		default:        break;
	}
}
/****************functionActive switch-case********************/
void FunctionActiveSwitch(struct EDITTYPE *pEdit)
{
	if(pEdit->language==0&&pEdit->state.functionActive==0)
	{
        if( pEdit->state.ascii>=0x20 && pEdit->state.ascii<=0x7e )
		{
			pEdit->state.function=7;	
        }
	}
	else if(pEdit->language==1&&pEdit->state.functionActive==0)
	{
		if( pEdit->state.ascii>=0x20 && pEdit->state.ascii<=0x7e )
		{                    			
			if(pEdit->state.ascii>='a'&&pEdit->state.ascii<='z')
			{
				Spell(pEdit);
			}		
			else	pEdit->state.function=7;				
		}
	}
	/*一些操作后的按键确认*/
	else if(pEdit->state.functionActive==1) 
	{
		pEdit->editFlag=1;
		pEdit->state.functionActive=0;
	}
	/*退出前保存确认*/
	else if(pEdit->state.functionActive==2) 
	{
		if( (pEdit->state.ascii=='Y')||(pEdit->state.ascii=='y') )
		{
			Save(pEdit->file.fname, pEdit);
			pEdit->state.functionActive=0;				
			ExitEditer(pEdit);
		}
		else if( (pEdit->state.ascii=='N')||(pEdit->state.ascii=='n') )
		{
			pEdit->state.functionActive=0;
			ExitEditer(pEdit);
		}
	}
	else if(pEdit->state.functionActive==5||pEdit->state.functionActive==6) /*find*/
	{
		if(pEdit->state.ascii=='f'||pEdit->state.ascii=='F')
		{
			pEdit->state.function=16;
		}
		else if(pEdit->state.ascii=='c'||pEdit->state.ascii=='C')
		{
			if(pEdit->state.functionActive==5)
			{
				pEdit->state.function=20; /*cancle find in find*/
			}
			else 
			{
				pEdit->state.function=23; /*cancle find in replace*/
			}	
		}
	}
	else if(pEdit->state.functionActive==7) /*choose to replace*/
	{
		if(pEdit->state.ascii=='r'||pEdit->state.ascii=='R')
		{
			pEdit->state.function=22;
		}
		else if(pEdit->state.ascii=='c'||pEdit->state.ascii=='C')
		{
			pEdit->state.function=23;
		}
	}

	else if(pEdit->state.functionActive==9) /*save*/
	{
		if( (pEdit->state.ascii=='Y')||(pEdit->state.ascii=='y') )
		{
			pEdit->state.function=17;
			pEdit->state.functionActive=0;
		}
		else if( (pEdit->state.ascii=='N')||(pEdit->state.ascii=='n') )
		{
			pEdit->state.functionActive=0;
			pEdit->editFlag=1;
		}
	}
	else if(pEdit->state.functionActive==10) /*open input*/
	{
		if( (pEdit->state.ascii=='Y')||(pEdit->state.ascii=='y') )
		{
			pEdit->state.function=18;
			pEdit->state.functionActive=0;
		}
		else if( (pEdit->state.ascii=='N')||(pEdit->state.ascii=='n') )
		{
			pEdit->state.functionActive=0;
			pEdit->editFlag=1;
		}
	}
	else if(pEdit->state.functionActive==11) /*open save*/
	{
		if( (pEdit->state.ascii=='Y')||(pEdit->state.ascii=='y') )
		{
			Save(pEdit->file.fname, pEdit);
			pEdit->state.function=19;							
			pEdit->state.functionActive=0;			
		}
		else if( (pEdit->state.ascii=='N')||(pEdit->state.ascii=='n') )
		{
			pEdit->state.function=19;
			pEdit->state.functionActive=0;
		}
	}
	else if(pEdit->state.functionActive==12) /*new save*/
	{
		if( (pEdit->state.ascii=='Y')||(pEdit->state.ascii=='y') )
		{
			Save(pEdit->file.fname, pEdit);
			pEdit->state.function=11;							
			pEdit->state.functionActive=0;
		}
		else if( (pEdit->state.ascii=='N')||(pEdit->state.ascii=='n') )
		{
			pEdit->state.function=11;
			pEdit->state.functionActive=0;
			pEdit->state.mouseFlag=0;						
		}
	}
	else if(pEdit->state.functionActive==13) /*black white chosen select all*/
	{
		switch( pEdit->state.key )
		{
			case CTRL_C:   	pEdit->state.function=12;
							break;
			case BACKSPACE:
			case CTRL_X:    pEdit->state.function=13;
							break;
			default:        break;
		}
		pEdit->state.functionActive=0;
	}
	else if(pEdit->state.functionActive==15) /*save as*/
	{
		if( (pEdit->state.ascii=='Y')||(pEdit->state.ascii=='y') )
		{
			Saveas(pEdit);
			pEdit->state.functionActive=0;
		}
		else if( (pEdit->state.ascii=='N')||(pEdit->state.ascii=='n') )
		{
			pEdit->state.functionActive=0;
			pEdit->editFlag=1;
		}
	}
}


