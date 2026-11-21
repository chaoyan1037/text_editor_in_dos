/**********************************************
function：open file

programmer:XiaoYao

time：2011.10.10-2011.10.11
***********************************************/
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<graphics.h>
#include<bios.h>
#include"h/common.h"
#include"h/struct.h"
#include"h/node.h"
#include"h/spell.h"
#include"h/language.h"
#include"h/save.h"
#include"h/mouse.h"
#include"h/hanzi.h"
#include"h/open.h"


void Open(struct EDITTYPE *pEdit)
{
	FILE *fp=NULL;
	int r, r2;
	char routine[64];

	if(pEdit->spell.name[0]!='\0')
	{
		sprintf(routine, "C://TC30//BIN//SAVE//%.15s", pEdit->spell.name);
		fp=fopen(routine, "rt");
		if(fp==NULL)
		{
			sprintf(routine, "SAVE/%.15s", pEdit->spell.name);
			fp=fopen(routine, "rt");
		}
	}

	if(fp==NULL)
	{
		setfillstyle(SOLID_FILL, LIGHTGRAY);
		bar(240, 160, 380, 240);
		button3d(240, 160, 380, 240);
		setcolor(BLACK);
		rectangle(240+18, 160+54, 380-86, 240-8);
		WhiteBlack(240+18, 160+54, 380-86, 240-8, WHITE);
		Printfhz(250, 165, "无效文件名！", RED, pEdit);
		Printfhz(240+20, 160+55, "确定", RED, pEdit);
		pEdit->state.functionActive=1;
		pEdit->state.mouseFlag=0;
	}
	else
	{
		FreeAllNode(pEdit);
		while( (r=fgetc(fp))!=EOF )
		{
			if( (r>=0x20)&&(r<=0x7e) )
			{
				pEdit->state.ascii=r;
				GetEnglish(pEdit);
			}
			else if(r=='\n')
			{
				pEdit->state.ascii=13; /* KEY ENTER */
				GetEnter(pEdit);
			}
			else if(r=='\r')
			{
				continue;
			}
			else if(r=='\t')
			{
				int t;
				pEdit->state.ascii=' ';
				for(t=0; t<4; t++)
				{
					GetEnglish(pEdit);
				}
			}
			else if(r>=0x80)
			{
				r2=fgetc(fp);
				if(r2!=EOF)
				{
					pEdit->spell.pcode[0]=(char)r;
					pEdit->spell.pcode[1]=(char)r2;
					Gethanzi(pEdit);
				}
			}
		}
		strncpy(pEdit->file.fname, pEdit->spell.name, sizeof(pEdit->file.fname)-1);
		pEdit->file.fname[sizeof(pEdit->file.fname)-1]='\0';
		pEdit->file.fileflag=1; /* 置1表示需重新载入文件名 */
		pEdit->editFlag=1;
		pEdit->cursorFlag=1;
		pEdit->tempCurrentCount=0;

		fclose(fp);
	}
}

void New(struct EDITTYPE *pEdit)
{
	FreeAllNode(pEdit);
	setfillstyle(SOLID_FILL, WHITE);
	bar(EDITERSCREENX1, EDITERSCREENY1, EDITERSCREENX2, EDITERSCREENY2);
	strcpy(pEdit->file.fname, "NONAME.TXT");
	pEdit->file.fileflag=1; /* 置1表示需重新载入文件名 */
	pEdit->editFlag=1;      /* 重新显示屏幕 */
	pEdit->cursorFlag=1;
	pEdit->pageDownFlag=0;
	pEdit->tempCurrentCount=0;
	pEdit->lastPosX=-1;
}
