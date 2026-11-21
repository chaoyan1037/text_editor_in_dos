/*****************************************************************************
 * File: OPEN.C
 * Function: Load an existing text file (ASCII + GB2312 Chinese) into the
 *           editor linked list, or reset the editor to a blank document.
 * Programmer: XiaoYao (2011.10.10-2011.10.11)
 *****************************************************************************/
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

/*
 * Open - Load the file named in `pEdit->spell.name` from the SAVE directory.
 *
 * Tries the original TC 3.0 path ("C://TC30//BIN//SAVE//") first, then falls
 * back to relative "SAVE/". On open failure, displays an error dialog and
 * sets `functionActive = 1` so any keypress or click dismisses it cleanly.
 *
 * Byte decoding rules:
 *   - 0x20..0x7E: printable ASCII node (flag=1)
 *   - '\n':       newline node (flag=0)
 *   - '\r':       ignored (normalizes CRLF line endings)
 *   - '\t':       expanded to 4 spaces (prevents corrupting subsequent Hanzi)
 *   - >= 0x80:    paired with the next byte as a 2-byte GB2312 Hanzi (flag=2)
 */
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
		/* Show "invalid filename" modal alert */
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
		/* Copy filename into dedicated file.fname buffer (no aliasing) */
		strncpy(pEdit->file.fname, pEdit->spell.name, sizeof(pEdit->file.fname)-1);
		pEdit->file.fname[sizeof(pEdit->file.fname)-1]='\0';
		pEdit->file.fileflag=1; /* Refresh title bar */
		pEdit->editFlag=1;
		pEdit->cursorFlag=1;
		pEdit->tempCurrentCount=0;

		fclose(fp);
	}
}

/*
 * New - Clear the current document and reset editor state to "NONAME.TXT".
 * Note: Any save-before-new confirmation is handled beforehand by SwitchFunction.
 */
void New(struct EDITTYPE *pEdit)
{
	FreeAllNode(pEdit);
	setfillstyle(SOLID_FILL, WHITE);
	bar(EDITERSCREENX1, EDITERSCREENY1, EDITERSCREENX2, EDITERSCREENY2);
	strcpy(pEdit->file.fname, "NONAME.TXT");
	pEdit->file.fileflag=1; /* Refresh title bar */
	pEdit->editFlag=1;      /* Redraw empty viewport */
	pEdit->cursorFlag=1;
	pEdit->pageDownFlag=0;
	pEdit->tempCurrentCount=0;
	pEdit->lastPosX=-1;     /* Force status bar refresh */
}
