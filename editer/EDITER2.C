/*****************************************************************************
 * File: EDITER2.C
 * Function: Program entry point, master event loop, state initialization,
 *           and command / keyboard / modal-dialog dispatch tables.
 * Programmer: YanChao (2011.9.10-2011.11.4)
 *
 * Event Loop Pipeline (one iteration):
 *   1. Resolve deferred flags (`pageDownFlag`, `file.modifyFlag`).
 *   2. Dispatch queued command `state.function` via SwitchFunction().
 *   3. If no pull-down menu is active (`editActive == 0`), poll BIOS keyboard:
 *        - In normal mode (`functionActive == 0`), map control/nav keys via
 *          SwitchKey().
 *        - Dispatch printable characters, Chinese IME, or modal Y/N/F/R/C
 *          responses via FunctionActiveSwitch().
 *      Otherwise delegate keys to Submenu().
 *   4. Refresh UI in strict draw order:
 *        ShowPosition -> MouseKey -> ViewAll -> ShowMouse -> DrawCursor -> RollBar.
 *****************************************************************************/
#include<graphics.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
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

/*
 * main - Initialize editor state and VGA graphics, then run the interactive
 *        event loop until ExitEditer() terminates the process.
 */
int main(void)
{
	int inputFlag=0; /* Set to 1 during a loop pass when a BIOS key was read */
	struct EDITTYPE edit;
	InitStruct(&edit);
	DrawMainMenu(&edit);

	/* Display initial "NONAME.TXT" filename in the title bar */
	setcolor(YELLOW);
	outtextxy(5, 15, edit.file.fname);

	while(1)
	{
		setcolor(BLACK);
		settextjustify(LEFT_TEXT, BOTTOM_TEXT);

		/* After scrolling down at the bottom row, step the caret onto the new row */
		if(edit.pageDownFlag==1 || edit.pageDownFlag==2)
		{
			edit.state.function=2;
			edit.pageDownFlag=0;
			edit.state.key=DOWN;
		}

		/* Promote deferred save-before-open (1) / save-before-new (2) dialogs */
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

		/* Execute any command queued by keyboard or mouse in the previous pass */
		if(edit.state.function!=0)
		{
			SwitchFunction(&edit);
			edit.state.function=0;
		}

		if(edit.editActive==0) /* Normal editing or modal dialog state */
		{
			inputFlag=0;
			if( bioskey(1)!=0 )
			{
				long int i;
				i=bioskey(0);
				edit.state.key=i;                 /* Full scan code + ASCII */
				edit.state.ascii=(int)(i&0x00ff); /* Low-byte ASCII value */
				inputFlag=1;
				if(edit.state.functionActive==0)
				{
					SwitchKey(&edit);
				}
			}
			if(inputFlag)
			{
				FunctionActiveSwitch(&edit);
			}
		}
		else /* Pull-down menu F1..F4 is open */
		{
			Submenu(&edit);
		}

		/* UI update order is intentional to keep XOR cursor/mouse clean */
		ShowPosition(&edit);
		MouseKey(&edit);
		ViewAll(&edit);
		ShowMouse(&edit);
		DrawCursor(&edit);
		RollBar(&edit);
	}

	return 0;
}

/*
 * InitStruct - Zero-initialize `pEdit`, configure default coordinates, menu
 *              counts, and filename, and then probe the DOS mouse driver.
 */
void InitStruct(struct EDITTYPE *pEdit)
{
	memset(pEdit, 0, sizeof(struct EDITTYPE));

	/* Initial mouse position centered on the 640x480 screen */
	pEdit->state.x=320;
	pEdit->state.y=240;
	pEdit->state.function=0;
	pEdit->state.functionActive=0;
	pEdit->state.mouseFlag=0;

	/* Empty document, clipboard, and find-match lists */
	pEdit->start=NULL;
	pEdit->current=NULL;
	pEdit->last=NULL;
	pEdit->clipStart=NULL;
	pEdit->clipLast=NULL;
	pEdit->findStart=NULL;
	pEdit->findLast=NULL;
	pEdit->pageFirst=NULL;

	/* Caret starts at top-left of viewport; caches force initial draw */
	pEdit->x=EDITERSCREENX1;
	pEdit->y=EDITERSCREENY1+CHARHIGH;
	pEdit->line=0;
	pEdit->row=0;
	pEdit->lineCount=0;
	pEdit->cursorFlag=0;
	pEdit->editFlag=0;
	pEdit->editActive=0;
	pEdit->copyActive=0;
	pEdit->copyCutFlag=0;
	pEdit->clipFlag=0;
	pEdit->spell.phz=NULL;
	pEdit->language=0; /* Start in English (ASCII) input mode */
	pEdit->pageDownFlag=0;
	pEdit->tempLineCount=CHARINROW;
	pEdit->tempCurrentCount=0;
	pEdit->lastPosX=-1;
	pEdit->lastPosY=-1;

	pEdit->file.modifyFlag=0;
	strcpy(pEdit->file.fname, "NONAME.TXT");
	pEdit->file.fileflag=0;

	/* Submenu item counts: File(5), Edit(4), Tools(2), Help(2) */
	pEdit->submenu.menuflag=0;
	pEdit->submenu.flag=1;
	pEdit->submenu.SubmenuNum[0]=5;
	pEdit->submenu.SubmenuNum[1]=4;
	pEdit->submenu.SubmenuNum[2]=2;
	pEdit->submenu.SubmenuNum[3]=2;

	pEdit->spell.pcode[2]='\0';
	pEdit->findGoal[0]='\0';
	pEdit->replaceGoal[0]='\0';

	/* Verify DOS mouse driver is present after all fields are safely initialized */
	if(!InitMouse())
	{
		printf("InitMouse failed...\n");
		getch();
		exit(1);
	}
}

/*
 * SwitchFunction - Central command dispatcher keyed by `pEdit->state.function`.
 *   1: Exit prompt      9: Save prompt      17: Execute Save
 *   2: MoveCursor      10: Open filename    18: Open (prompt save if non-empty)
 *   3: GetEnter        11: Execute New      19: Execute Open
 *   4: Paste           12: CopySelection    20: Cancel Find
 *   5: Find dialog     13: CopyCut          21: Find & Replace dialog
 *   6: Open submenu    14: New (prompt)     22: Execute Replace
 *   7: GetEnglish      15: Save As dialog   23: Cancel Replace
 *   8: Toggle EN/ZH    16: Execute Find     24: Extend selection
 *                                           25: Select All on page
 */
void SwitchFunction(struct EDITTYPE *pEdit)
{
	switch(pEdit->state.function)
	{
		case 1:
			Savewarning(pEdit);
			pEdit->state.functionActive=2;
			break;
		case 2:
			MoveCursor(pEdit);
			break;
		case 3:
			GetEnter(pEdit);
			break;
		case 4:
			Paste(pEdit);
			break;
		case 5:
			FindFunction(pEdit);
			break;
		case 6:
			GetSubmenu(pEdit);
			break;
		case 7:
			GetEnglish(pEdit);
			break;
		case 8:
			ExchangeLanguage(pEdit);
			ShowLanguage(pEdit);
			break;
		case 9:
			Savewarning(pEdit);
			pEdit->state.functionActive=9;
			break;
		case 10:
			InName(pEdit);
			if(pEdit->spell.name[0]!='\0')
			{
				pEdit->state.functionActive=10;
			}
			else
			{
				pEdit->editFlag=1;
			}
			break;
		case 11:
			New(pEdit);
			break;
		case 12:
			CopySelection(pEdit);
			break;
		case 13:
			CopyCut(pEdit);
			break;
		case 14:
			if( pEdit->start!=NULL )
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
		case 15:
			InName(pEdit);
			if(pEdit->spell.name[0]!='\0')
			{
				pEdit->state.functionActive=15;
			}
			else
			{
				pEdit->editFlag=1;
			}
			break;
		case 16:
			FingGoal(pEdit);
			break;
		case 17:
			Save(pEdit->file.fname, pEdit);
			break;
		case 18:
			if( pEdit->start!=NULL )
			{
				Savewarning(pEdit);
				pEdit->file.modifyFlag=1;
			}
			else
			{
				Open(pEdit);
			}
			break;
		case 19:
			Open(pEdit);
			break;
		case 20:
			CancleFind(pEdit);
			break;
		case 21:
			FindAndReplace(pEdit);
			break;
		case 22:
			ReplaceFunction(pEdit);
			break;
		case 23:
			CancleReplace(pEdit);
			break;
		case 24:
			BlackWhiteChoosen(pEdit);
			break;
		case 25:
			AllChosen(pEdit);
			break;
		default:
			break;
	}
}

/*
 * SwitchKey - Translate special/shortcut BIOS keys in normal editing mode
 *             (`functionActive == 0`) into queued command IDs.
 */
void SwitchKey(struct EDITTYPE *pEdit)
{
	switch( pEdit->state.key )
	{
		case ESC:        pEdit->state.function=1;  break;
		case UP:
		case DOWN:
		case LEFT:
		case RIGHT:      pEdit->state.function=2;  break;
		case ENTER:
			if(pEdit->state.functionActive==0)
			{
				pEdit->state.function=3;
			}
			break;
		case BACKSPACE:  BackSpace(pEdit);         break;
		case CTRL_LEFT:
		case CTRL_RIGHT: pEdit->state.function=24; break;
		case CTRL_V:     pEdit->state.function=4;  break;
		case CTRL_F:     pEdit->state.function=5;  break;
		case CTRL_R:     pEdit->state.function=21; break;
		case CTRL_S:     pEdit->state.function=9;  break;
		case CTRL_O:     pEdit->state.function=10; break;
		case CTRL_N:     pEdit->state.function=14; break;
		case F1:
		case F2:
		case F3:
		case F4:         pEdit->state.function=6;  break;
		case F8:         pEdit->state.function=8;  break;
		case CTRL_C:     pEdit->state.function=12; break;
		case CTRL_X:     pEdit->state.function=13; break;
		case CTRL_D:     pEdit->state.function=15; break;
		case CTRL_A:     pEdit->state.function=25; break;
		default:         break;
	}
}

/*
 * FunctionActiveSwitch - Route printable text input (EN or ZH) when
 *                        `functionActive == 0`, or handle keyboard responses
 *                        (Y/N/F/R/C/Ctrl+C/Ctrl+X) for the active modal state.
 */
void FunctionActiveSwitch(struct EDITTYPE *pEdit)
{
	if(pEdit->language==0 && pEdit->state.functionActive==0)
	{
		/* English mode: insert any printable ASCII character */
		if( pEdit->state.ascii>=0x20 && pEdit->state.ascii<=0x7e )
		{
			pEdit->state.function=7;
		}
	}
	else if(pEdit->language==1 && pEdit->state.functionActive==0)
	{
		/* Chinese mode: 'a'..'z' starts pinyin IME; symbols/digits insert as ASCII */
		if( pEdit->state.ascii>=0x20 && pEdit->state.ascii<=0x7e )
		{
			if(pEdit->state.ascii>='a' && pEdit->state.ascii<='z')
			{
				Spell(pEdit);
			}
			else
			{
				pEdit->state.function=7;
			}
		}
	}
	else if(pEdit->state.functionActive==1) /* Dismiss info/help alert on any key */
	{
		pEdit->editFlag=1;
		pEdit->state.functionActive=0;
	}
	else if(pEdit->state.functionActive==2) /* Save-before-Exit confirmation */
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
	else if(pEdit->state.functionActive==5 || pEdit->state.functionActive==6) /* Find / Replace search */
	{
		if(pEdit->state.ascii=='f' || pEdit->state.ascii=='F')
		{
			pEdit->state.function=16;
		}
		else if(pEdit->state.ascii=='c' || pEdit->state.ascii=='C')
		{
			if(pEdit->state.functionActive==5)
			{
				pEdit->state.function=20; /* Cancel Find */
			}
			else
			{
				pEdit->state.function=23; /* Cancel Replace */
			}
		}
	}
	else if(pEdit->state.functionActive==7) /* Confirm replacement on found match */
	{
		if(pEdit->state.ascii=='r' || pEdit->state.ascii=='R')
		{
			pEdit->state.function=22;
		}
		else if(pEdit->state.ascii=='c' || pEdit->state.ascii=='C')
		{
			pEdit->state.function=23;
		}
	}
	else if(pEdit->state.functionActive==9) /* Save confirmation */
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
	else if(pEdit->state.functionActive==10) /* Open filename confirmation */
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
	else if(pEdit->state.functionActive==11) /* Save-before-Open confirmation */
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
	else if(pEdit->state.functionActive==12) /* Save-before-New confirmation */
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
	else if(pEdit->state.functionActive==13) /* Select-All action dispatch */
	{
		switch( pEdit->state.key )
		{
			case CTRL_C:
				pEdit->state.function=12;
				break;
			case BACKSPACE:
			case CTRL_X:
				pEdit->state.function=13;
				break;
			default:
				/* Any other key cancels the selection highlight cleanly */
				if(pEdit->copyCutFlag==0)
				{
					pEdit->clipStart=NULL;
					pEdit->clipLast=NULL;
				}
				pEdit->editFlag=1;
				break;
		}
		pEdit->state.functionActive=0;
	}
	else if(pEdit->state.functionActive==15) /* Save-As filename confirmation */
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
