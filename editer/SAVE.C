/**********************************************
function:save file

programmer:XiaoYao

time：2011.10.10
***********************************************/

#include<graphics.h>
#include<string.h>
#include<bios.h>
#include"h/common.h"
#include"h/view.h"
#include"h/struct.h"
#include"h/mouse.h"
#include"h/save.h"

void Savewarning(struct EDITTYPE *pEdit)
{
	long int key;	
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
	FILE *fp;

	struct NODE *p;
	char routine[40]="C://TC30//BIN//SAVE//";

		pEdit->editFlag=1;		//重新显示屏幕以覆盖对话框
		pEdit->cursorFlag=1;		//重新显示光标

		strcat(routine, filename);
		p=pEdit->start;
		
		fp=fopen(routine,"wt");

		while(p!=NULL)
		{
			if(p->data[0]==0)
			{
				fprintf(fp,"%c",p->data[0]);
				fclose(fp);
					
			}
			else
			{
				if(p->flag==0)
				{
					p->data[0]=10;
                                        fprintf(fp,"%c",p->data[0]);
				}
				if(p->flag==1)
				{
					fprintf(fp,"%c",p->data[0]);
				}
				else if(p->flag==2)
				{
					fprintf(fp,"%c",p->data[0]);
					fprintf(fp,"%c",p->data[1]);
				}
				p=p->next;
			}
		}
	
		fclose(fp);

}

void Saveas(struct EDITTYPE *pEdit)
{
	char routine[40]="C://TC30//BIN//SAVE//";
	strcat(routine, pEdit->spell.name);
	Save(pEdit->spell.name, pEdit);
}

	
