/**********************************************
function:save file

programmer:XiaoYao

time：2011.10.10
***********************************************/

#include<graphics.h>
#include<string.h>
#include<stdio.h>
#include<bios.h>
#include"h/common.h"
#include"h/view.h"
#include"h/struct.h"
#include"h/mouse.h"
#include"h/hanzi.h"
#include"h/save.h"

void Savewarning(struct EDITTYPE *pEdit)
{
	setfillstyle(SOLID_FILL, LIGHTGRAY);
	bar(240, 160, 380, 240);
	button3d(240, 160, 380, 240);

	setcolor(BLACK);
	rectangle(240+6, 160+54, 380-74, 240-8);
	WhiteBlack(240+6, 160+54, 380-74, 240-8, WHITE);

	setcolor(BLACK);
	rectangle(245+69, 160+54, 380-6, 240-8);
	WhiteBlack(245+69, 160+54, 380-6, 240-8, WHITE);
	setcolor(RED);

	Printfhz(250, 165, "保存？", RED, pEdit);
	Printfhz(240+8, 160+55, "确定", RED, pEdit);
	outtextxy(283, 230, "(Y)");
	Printfhz(245+71, 160+55, "取消", RED, pEdit);
	outtextxy(351, 230, "(N)");
}


void Save(char *filename, struct EDITTYPE *pEdit)
{
	FILE *fp=NULL;
	struct NODE *p;
	char routine[64];

	pEdit->editFlag=1;   /* 重新显示屏幕以覆盖对话框 */
	pEdit->cursorFlag=1; /* 重新显示光标 */

	if(filename==NULL || filename[0]=='\0')
	{
		return;
	}

	sprintf(routine, "C://TC30//BIN//SAVE//%.15s", filename);
	fp=fopen(routine, "wt");
	if(fp==NULL)
	{
		sprintf(routine, "SAVE/%.15s", filename);
		fp=fopen(routine, "wt");
	}
	if(fp==NULL)
	{
		return;
	}

	p=pEdit->start;
	while(p!=NULL)
	{
		if(p->flag==0)
		{
			fputc('\n', fp);
		}
		else if(p->flag==1)
		{
			if(p->data[0]!='\0')
			{
				fputc((unsigned char)p->data[0], fp);
			}
		}
		else if(p->flag==2)
		{
			fputc((unsigned char)p->data[0], fp);
			fputc((unsigned char)p->data[1], fp);
		}
		p=p->next;
	}

	fclose(fp);
}

void Saveas(struct EDITTYPE *pEdit)
{
	if(pEdit->spell.name[0]=='\0')
	{
		pEdit->editFlag=1;
		return;
	}
	Save(pEdit->spell.name, pEdit);
	strncpy(pEdit->file.fname, pEdit->spell.name, sizeof(pEdit->file.fname)-1);
	pEdit->file.fname[sizeof(pEdit->file.fname)-1]='\0';
	pEdit->file.fileflag=1;
}
