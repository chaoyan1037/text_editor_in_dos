/*****************************************************************************
 * File: OPEN.C
 * Purpose: Load text files (ASCII + GB2312 Chinese) into the document list
 *          and reset the editor to a fresh blank document ("NONAME.TXT").
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

/*****************************************************************************
 * Function: Open
 * Purpose:  Open and parse the file named in `pEdit->spell.name` from the
 *           SAVE directory, replacing the current document buffer on success.
 *
 * @param pEdit Editor context providing `spell.name`.
 * @post        - On open failure: draws "无效文件名！" alert box and sets
 *                `state.functionActive = 1` so any key or click dismisses it.
 *              - On success: frees prior nodes, populates the document list,
 *                copies the filename into `file.fname`, and flags title-bar,
 *                viewport, caret, and scrollbar refreshes.
 *
 * Byte Stream Decoding:
 *   - 0x20..0x7E : printable ASCII -> GetEnglish() (flag=1)
 *   - '\n'       : line break      -> GetEnter()   (flag=0)
 *   - '\r'       : skipped         (handles DOS CRLF files cleanly)
 *   - '\t'       : expanded to 4 spaces (prevents mis-syncing GB2312 pairs)
 *   - >= 0x80    : paired with next byte -> Gethanzi() (flag=2)
 *****************************************************************************/
void Open(struct EDITTYPE *pEdit)
{
	FILE *fp=NULL;
	int r, r2;
	char routine[64];

	if(pEdit->spell.name[0]!='\0')
	{
		/* Try original TC 3.0 path first, then relative repository path */
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
		/* Display dismissible "invalid filename" modal alert */
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
		/* Use int r with EOF check (avoids classic feof() extra-iteration bug) */
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
				/* Read second byte of GB2312/GBK character pair */
				r2=fgetc(fp);
				if(r2!=EOF)
				{
					pEdit->spell.pcode[0]=(char)r;
					pEdit->spell.pcode[1]=(char)r2;
					Gethanzi(pEdit);
				}
			}
		}

		/* Store filename in dedicated file.fname buffer (never alias spell.name) */
		strncpy(pEdit->file.fname, pEdit->spell.name, sizeof(pEdit->file.fname)-1);
		pEdit->file.fname[sizeof(pEdit->file.fname)-1]='\0';
		pEdit->file.fileflag=1;
		pEdit->editFlag=1;
		pEdit->cursorFlag=1;
		pEdit->tempCurrentCount=0; /* Force RollBar() recalculation */

		fclose(fp);
	}
}

/*****************************************************************************
 * Function: New
 * Purpose:  Discard the current document buffer and initialize a blank
 *           document titled "NONAME.TXT".
 *
 * @param pEdit Master editor context.
 * @pre         Caller (SwitchFunction case 14 / modal state 12) has already
 *              handled any save-before-new user confirmation.
 * @post        Document list is empty, viewport is cleared, title bar shows
 *              "NONAME.TXT", and scrollbar/status-bar caches are invalidated.
 *****************************************************************************/
void New(struct EDITTYPE *pEdit)
{
	FreeAllNode(pEdit);
	setfillstyle(SOLID_FILL, WHITE);
	bar(EDITERSCREENX1, EDITERSCREENY1, EDITERSCREENX2, EDITERSCREENY2);
	strcpy(pEdit->file.fname, "NONAME.TXT");
	pEdit->file.fileflag=1;
	pEdit->editFlag=1;
	pEdit->cursorFlag=1;
	pEdit->pageDownFlag=0;
	pEdit->tempCurrentCount=0;
	pEdit->lastPosX=-1;
}
