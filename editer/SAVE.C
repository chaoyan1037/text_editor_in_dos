/*****************************************************************************
 * File: SAVE.C
 * Purpose: Save confirmation modal dialog and text serialization (Save/SaveAs).
 * Programmer: XiaoYao (2011.10.10)
 *****************************************************************************/

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

/*****************************************************************************
 * Function: Savewarning
 * Purpose:  Render the centered "保存？ 确定(Y) / 取消(N)" dialog box
 *           (240..380 x 160..240) shared by Exit, Save, Open, and New prompts.
 *
 * @param pEdit Editor context (used for Hanzi label rendering).
 * @note        Caller is responsible for setting `state.functionActive`
 *              (2, 9, 11, or 12) so keyboard/mouse Y/N clicks route properly.
 *****************************************************************************/
void Savewarning(struct EDITTYPE *pEdit)
{
	setfillstyle(SOLID_FILL, LIGHTGRAY);
	bar(240, 160, 380, 240);
	button3d(240, 160, 380, 240);

	/* Left OK (Y) button: X=246..306, Y=214..232 */
	setcolor(BLACK);
	rectangle(240+6, 160+54, 380-74, 240-8);
	WhiteBlack(240+6, 160+54, 380-74, 240-8, WHITE);

	/* Right Cancel (N) button: X=314..374, Y=214..232 */
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

/*****************************************************************************
 * Function: Save
 * Purpose:  Serialize all document nodes from `pEdit->start` to `filename`
 *           under the SAVE/ directory in text mode ("wt").
 *
 * @param filename Target DOS 8.3 filename (e.g., "NONAME.TXT").
 * @param pEdit    Master editor context.
 * @post           Always sets `editFlag = 1` and `cursorFlag = 1` to clear
 *                 the dialog overlay, even if fopen() fails. Never mutates
 *                 node contents while serializing.
 * @complexity     O(N) where N = total nodes in document.
 *****************************************************************************/
void Save(char *filename, struct EDITTYPE *pEdit)
{
	FILE *fp=NULL;
	struct NODE *p;
	char routine[64];

	pEdit->editFlag=1;   /* Erase confirmation dialog on next ViewAll() */
	pEdit->cursorFlag=1; /* Re-sync caret position */

	if(filename==NULL || filename[0]=='\0')
	{
		return;
	}

	/* Try original TC 3.0 SAVE directory first, then relative "SAVE/" */
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
			/* Text-mode '\n' expands to CRLF automatically on DOS */
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

/*****************************************************************************
 * Function: Saveas
 * Purpose:  Save the document to the filename typed in `pEdit->spell.name`
 *           and update the active title-bar filename (`pEdit->file.fname`).
 *
 * @param pEdit Master editor context with `spell.name` populated by InName().
 *****************************************************************************/
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
	pEdit->file.fileflag=1; /* Repaint title bar with new filename */
}
