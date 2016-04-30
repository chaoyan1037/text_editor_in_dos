/**********************************************
function：open file

programmer:XiaoYao

time：2011.10.10-2011.10.11
***********************************************/
#include<stdlib.h>
#include<string.h>
#include<graphics.h>
#include<bios.h>
#include"h/common.h"
#include"h/struct.h"
#include"h/node.h"
#include"h/spell.h"
#include"h/language.h"
#include"h/save.h"
#include"h/mouse.h"


void Open(struct EDITTYPE *pEdit)
{
	FILE *fp;
	long int r;
	long int s;
	int i=0, j=0, k=0;
	char routine[40]="C://TC30//BIN//SAVE//";
	strcat(routine, pEdit->spell.name);

	if( (fp=fopen(routine,"rt"))==NULL )
	{	
		setfillstyle(SOLID_FILL, LIGHTGRAY);
		bar(240, 160, 380, 240);
		button3d(240, 160, 380, 240);
		setcolor(BLACK);
		rectangle(240+18, 160+54, 380-86, 240-8);
		WhiteBlack(240+18, 160+54, 380-86, 240-8, WHITE);
		Printfhz(250, 165, "无效文件名！", RED, pEdit);
		Printfhz(240+20, 160+55, "确定", RED, pEdit);
	}
	else
	{
		FreeAllNode( pEdit );
		while(feof(fp)==0)
		{
			r=fgetc(fp);
			if( (r>=0x20)&&(r<=0x7e) )
			{
				pEdit->state.ascii=r;
				GetEnglish(pEdit);
				i++;
			}
			else if(r==10)
			{	
				pEdit->state.ascii=13;	//KEY ENTER
				GetEnter(pEdit);
				j++;
			}
			else
			{
				pEdit->spell.pcode[0]=r;
				r=fgetc(fp);
				if(feof(fp)==0)
				{
					pEdit->spell.pcode[1]=r;
					Gethanzi(pEdit);
					k++;
				}
			}
		}
		pEdit->file.fname=pEdit->spell.name;	
		pEdit->file.fileflag=1;		//置1表示需重新载入文件名
		pEdit->editFlag=1;
		
		fclose(fp);
	}
}

void New(struct EDITTYPE *pEdit)
{
	if( pEdit->start!=NULL )
	{
		Savewarning(pEdit);
	}
	FreeAllNode( pEdit );
	setfillstyle(SOLID_FILL, WHITE);
	bar(EDITERSCREENX1, EDITERSCREENY1, EDITERSCREENX2, EDITERSCREENY2);
	pEdit->file.fname="NONAME.TXT";
	pEdit->file.fileflag=1;		//置1表示需重新载入文件名
	pEdit->editFlag=1;		//重新显示屏幕
	pEdit->tempCurrentCount=0;
}
	
