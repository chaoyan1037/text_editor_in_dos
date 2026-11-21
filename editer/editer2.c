/*****************************************************************************
 * File: EDITER2.C
 * Purpose: Application entry point, master event loop, context initialization,
 *          and command / keyboard / modal-dialog state-machine dispatchers.
 * Programmer: YanChao (2011.9.10-2011.11.4)
 *
 * Main Event-Loop Execution Order (per iteration):
 *   1. Resolve deferred flags:
 *        - `pageDownFlag` (1/2) -> inject synthetic DOWN key after scroll.
 *        - `file.modifyFlag` (1/2) -> promote to modal state 11 (Open) / 12 (New).
 *   2. Execute queued command `state.function` (1..25) via SwitchFunction().
 *   3. Process keyboard input:
 *        - If `editActive == 0`: poll bioskey(1)/bioskey(0).
 *            * If `functionActive == 0`: dispatch shortcuts via SwitchKey().
 *            * Always run FunctionActiveSwitch() for text/IME/modal keys.
 *        - Else (`editActive` in 1..4): delegate to Submenu().
 *   4. Refresh UI layers in strict order so XOR cursor & mouse never leave
 *      artifacts:
 *        ShowPosition -> MouseKey -> ViewAll -> ShowMouse -> DrawCursor -> RollBar.
 *
 * Modal Dialog State Machine (`state.functionActive`):
 *   0  (Normal editing)
 *    |-- ESC / Exit btn ------> 2  (Save before Exit: Y/N -> ExitEditer)
 *    |-- Ctrl+S / Save -------> 9  (Save confirm: Y -> Save, N -> 0)
 *    |-- Ctrl+O / Open -------> 10 (Open confirm: Y -> Open or 11, N -> 0)
 *    |                            `-> 11 (Save before Open: Y/N -> Open)
 *    |-- Ctrl+N / New --------> 12 (Save before New: Y/N -> New)
 *    |-- Ctrl+D / Save As ----> 15 (Save As confirm: Y -> Saveas, N -> 0)
 *    |-- Ctrl+F / Find -------> 5  (Find ready: F -> Find, C -> 0)
 *    |-- Ctrl+R / Replace ----> 6  (Replace ready: F -> 7 or 1, C -> 0)
 *    |                            `-> 7 (Match found: R -> Replace, C -> 0)
 *    |-- Ctrl+A / Select All -> 13 (Ctrl+C / Ctrl+X / BackSpace / cancel -> 0)
 *    `-- Alert / Help popup --> 1  (Any key or click -> redraw & 0)
 *****************************************************************************/
#include<graphics.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
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
 * ensure_project_cwd - If `SAVE/` is not in the current working directory
 *                      (e.g., when `./editor` is launched via Finder or from
 *                      another directory), switch to the directory containing
 *                      the executable so `SAVE/` is always found.
 */
static void ensure_project_cwd(const char *argv0)
{
	const char *slash;
	char dir[512];
	char probe[540];
	size_t len;

	if(access("SAVE", F_OK)==0)
	{
		return;
	}
	if(argv0==NULL)
	{
		return;
	}
	slash=strrchr(argv0, '/');
	if(slash==NULL)
	{
		return;
	}
	len=(size_t)(slash-argv0);
	if(len==0 || len>=sizeof(dir))
	{
		return;
	}
	memcpy(dir, argv0, len);
	dir[len]='\0';
	sprintf(probe, "%s/SAVE", dir);
	if(access(probe, F_OK)==0)
	{
		if(chdir(dir)!=0)
		{
			/* Ignore failure; Open/Save still fall back to CWD */
		}
	}
}

/*****************************************************************************
 * Function: main
 * Purpose:  Locate project assets (`SAVE/`, `spell/`), initialize `EDITTYPE`,
 *           start terminal UI, and run the event loop until ExitEditer().
 *
 * @return   0 (standard C89 signature; normal exit occurs via exit(0)).
 *****************************************************************************/
int main(int argc, char *argv[])
{
	int inputFlag=0; /* 1 when a BIOS keystroke was dequeued in this pass */
	struct EDITTYPE edit;
	(void)argc;
	ensure_project_cwd((argc>0) ? argv[0] : NULL);
	InitStruct(&edit);
	DrawMainMenu(&edit);

	/* Render default title-bar filename ("NONAME.TXT") */
	setcolor(YELLOW);
	outtextxy(5, 15, edit.file.fname);

	while(1)
	{
		setcolor(BLACK);
		settextjustify(LEFT_TEXT, BOTTOM_TEXT);

		/* After scrolling down at row 25, step caret onto the newly exposed row */
		if(edit.pageDownFlag==1 || edit.pageDownFlag==2)
		{
			edit.state.function=2;
			edit.pageDownFlag=0;
			edit.state.key=DOWN;
		}

		/* Promote deferred save-before-open (1) or save-before-new (2) prompts */
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

		/* Dispatch any command queued by keyboard, mouse, or menu selection */
		if(edit.state.function!=0)
		{
			SwitchFunction(&edit);
			edit.state.function=0;
		}

		/* Only poll/sleep for input once all queued commands & redraws have rendered */
		if(edit.state.function==0 && edit.pageDownFlag==0
			&& edit.editFlag==0 && edit.file.fileflag==0 && edit.cursorFlag==0)
		{
			if(edit.editActive==0) /* Normal editing or modal dialog state */
			{
				inputFlag=0;
				if( bioskey(1)!=0 )
				{
					long int i;
					i=bioskey(0);
					edit.state.key=i;
					edit.state.ascii=(i & HANZI_KEY_TAG) ? 0 : (int)(i&0x00ff);
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
		}

		/* Strict render/poll sequence preserves XOR mouse & caret coherence */
		ShowPosition(&edit);
#ifndef __MSDOS__
		/* Fetch mouse packets that woke bioskey(1) on this same frame (<1ms latency) */
		ReadMouse(&edit);
#endif
		MouseKey(&edit);
		ViewAll(&edit);
		ShowMouse(&edit);
		DrawCursor(&edit);
		RollBar(&edit);
#ifndef __MSDOS__
		bgi_present();
#endif
	}

	return 0;
}

/*****************************************************************************
 * Function: InitStruct
 * Purpose:  Zero-initialize all fields of `EDITTYPE`, set startup defaults
 *           ("NONAME.TXT", English mode, submenu counts, invalid caches), and
 *           probe the DOS mouse driver via InitMouse().
 *
 * @param pEdit Uninitialized master context allocated in main().
 * @post        If InitMouse() fails, prints an error and calls exit(1) safely
 *              without freeing uninitialized pointers.
 *****************************************************************************/
void InitStruct(struct EDITTYPE *pEdit)
{
	memset(pEdit, 0, sizeof(struct EDITTYPE));

	/* Center initial mouse coordinates on the 640x480 screen */
	pEdit->state.x=320;
	pEdit->state.y=240;
	pEdit->state.function=0;
	pEdit->state.functionActive=0;
	pEdit->state.mouseFlag=0;

	/* Empty document, clipboard, and find-match linked lists */
	pEdit->start=NULL;
	pEdit->current=NULL;
	pEdit->last=NULL;
	pEdit->clipStart=NULL;
	pEdit->clipLast=NULL;
	pEdit->findStart=NULL;
	pEdit->findLast=NULL;
	pEdit->pageFirst=NULL;

	/* Caret at viewport top-left; cache sentinels force initial UI paint */
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
	pEdit->language=0;
	pEdit->pageDownFlag=0;
	pEdit->tempLineCount=CHARINROW;
	pEdit->tempCurrentCount=0;
	pEdit->lastPosX=-1;
	pEdit->lastPosY=-1;

	pEdit->file.modifyFlag=0;
	strcpy(pEdit->file.fname, "NONAME.TXT");
	pEdit->file.fileflag=0;

	/* Item counts for menus F1..F4 */
	pEdit->submenu.menuflag=0;
	pEdit->submenu.flag=1;
	pEdit->submenu.SubmenuNum[0]=5;
	pEdit->submenu.SubmenuNum[1]=4;
	pEdit->submenu.SubmenuNum[2]=2;
	pEdit->submenu.SubmenuNum[3]=2;

	pEdit->spell.pcode[2]='\0';
	pEdit->findGoal[0]='\0';
	pEdit->replaceGoal[0]='\0';

	/* Initialize DOS mouse driver only after struct pointers are valid */
	if(!InitMouse())
	{
		printf("InitMouse failed...\n");
		getch();
		exit(1);
	}
}

/*****************************************************************************
 * Function: SwitchFunction
 * Purpose:  Central command table executing `pEdit->state.function` (1..25).
 *
 * Command ID Reference:
 *   1  = Prompt Save before Exit   14 = New (prompt Save if non-empty)
 *   2  = MoveCursor                15 = Save As filename prompt
 *   3  = GetEnter                  16 = Execute Find() (FingGoal)
 *   4  = Paste clipboard           17 = Save to active file.fname
 *   5  = Find dialog               18 = Open (prompt Save if non-empty)
 *   6  = Open pull-down menu       19 = Execute Open()
 *   7  = GetEnglish (ASCII)        20 = Cancel Find
 *   8  = Toggle EN/ZH + redraw     21 = Find & Replace dialog
 *   9  = Save confirmation prompt  22 = Execute ReplaceFunction()
 *   10 = Open filename prompt      23 = Cancel Replace
 *   11 = Execute New()             24 = Extend selection (BlackWhiteChoosen)
 *   12 = CopySelection (Ctrl+C)    25 = Select All on page (AllChosen)
 *   13 = CopyCut (Ctrl+X/BS)
 *
 * @param pEdit Master editor context.
 *****************************************************************************/
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
			InNameOpen(pEdit);
			if(pEdit->spell.name[0]!='\0')
			{
				/* Pressing ENTER in Open browser opens immediately (or prompts save if non-empty) */
				if(pEdit->start!=NULL)
				{
					Savewarning(pEdit);
					pEdit->state.functionActive=11;
				}
				else
				{
					Open(pEdit);
				}
			}
			else
			{
				pEdit->editFlag=1; /* ESC cancel in InNameOpen: restore screen */
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
				pEdit->state.functionActive=12;
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
				/* Pressing ENTER in InName saves immediately to SAVE/<name> */
				Saveas(pEdit);
			}
			else
			{
				pEdit->editFlag=1; /* ESC cancel in InName: restore screen */
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
				pEdit->state.functionActive=11;
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

/*****************************************************************************
 * Function: SwitchKey
 * Purpose:  Map BIOS control, navigation, and function keys in normal editing
 *           mode (`functionActive == 0`) to queued `state.function` commands.
 *
 * @param pEdit Master editor context with `state.key` set by bioskey(0).
 *****************************************************************************/
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

/*****************************************************************************
 * Function: FunctionActiveSwitch
 * Purpose:  Handle printable character input (ASCII or Pinyin IME) during
 *           normal editing (`functionActive == 0`), or process modal dialog
 *           confirmation keys (`functionActive` in 1, 2, 5..7, 9..13, 15).
 *
 * @param pEdit Master editor context with freshly read `state.{key,ascii}`.
 *****************************************************************************/
void FunctionActiveSwitch(struct EDITTYPE *pEdit)
{
	if(pEdit->state.functionActive==0)
	{
		if((pEdit->state.key & HANZI_KEY_TAG)!=0)
		{
			/* Native OS/terminal UTF-8 -> GB18030 Chinese character input */
			pEdit->spell.pcode[0]=(char)((pEdit->state.key >> 8) & 0xff);
			pEdit->spell.pcode[1]=(char)(pEdit->state.key & 0xff);
			pEdit->spell.pcode[2]='\0';
			Gethanzi(pEdit);
		}
		else if( pEdit->state.ascii>=0x20 && pEdit->state.ascii<=0x7e )
		{
			/* Printable ASCII byte (0x20..0x7E) -> GetEnglish */
			pEdit->state.function=7;
		}
	}
	else if(pEdit->state.functionActive==1) /* Dismiss alert / Help popup on any key */
	{
		pEdit->editFlag=1;
		pEdit->state.functionActive=0;
	}
	else if(pEdit->state.functionActive==2) /* Save-before-Exit (Y/ENTER or N/ESC) */
	{
		if( pEdit->state.ascii=='Y' || pEdit->state.ascii=='y' || pEdit->state.key==ENTER )
		{
			Save(pEdit->file.fname, pEdit);
			pEdit->state.functionActive=0;
			ExitEditer(pEdit);
		}
		else if( pEdit->state.ascii=='N' || pEdit->state.ascii=='n' )
		{
			pEdit->state.functionActive=0;
			ExitEditer(pEdit);
		}
		else if( pEdit->state.key==ESC )
		{
			pEdit->state.functionActive=0;
			pEdit->editFlag=1;
		}
	}
	else if(pEdit->state.functionActive==5 || pEdit->state.functionActive==6) /* Find / Replace (F/C) */
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
	else if(pEdit->state.functionActive==7) /* Replace matched occurrence (R/C) */
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
	else if(pEdit->state.functionActive==9) /* Save confirmation (Y/ENTER or N/ESC) */
	{
		if( pEdit->state.ascii=='Y' || pEdit->state.ascii=='y' || pEdit->state.key==ENTER )
		{
			pEdit->state.function=17;
			pEdit->state.functionActive=0;
		}
		else if( pEdit->state.ascii=='N' || pEdit->state.ascii=='n' || pEdit->state.key==ESC )
		{
			pEdit->state.functionActive=0;
			pEdit->editFlag=1;
		}
	}
	else if(pEdit->state.functionActive==10) /* Open filename confirmation (Y/N) */
	{
		if( pEdit->state.ascii=='Y' || pEdit->state.ascii=='y' || pEdit->state.key==ENTER )
		{
			pEdit->state.function=18;
			pEdit->state.functionActive=0;
		}
		else if( pEdit->state.ascii=='N' || pEdit->state.ascii=='n' || pEdit->state.key==ESC )
		{
			pEdit->state.functionActive=0;
			pEdit->editFlag=1;
		}
	}
	else if(pEdit->state.functionActive==11) /* Save-before-Open (Y/ENTER or N/ESC) */
	{
		if( pEdit->state.ascii=='Y' || pEdit->state.ascii=='y' || pEdit->state.key==ENTER )
		{
			Save(pEdit->file.fname, pEdit);
			pEdit->state.function=19;
			pEdit->state.functionActive=0;
		}
		else if( pEdit->state.ascii=='N' || pEdit->state.ascii=='n' )
		{
			pEdit->state.function=19;
			pEdit->state.functionActive=0;
		}
		else if( pEdit->state.key==ESC )
		{
			pEdit->state.functionActive=0;
			pEdit->editFlag=1;
		}
	}
	else if(pEdit->state.functionActive==12) /* Save-before-New (Y/ENTER or N/ESC) */
	{
		if( pEdit->state.ascii=='Y' || pEdit->state.ascii=='y' || pEdit->state.key==ENTER )
		{
			Save(pEdit->file.fname, pEdit);
			pEdit->state.function=11;
			pEdit->state.functionActive=0;
		}
		else if( pEdit->state.ascii=='N' || pEdit->state.ascii=='n' )
		{
			pEdit->state.function=11;
			pEdit->state.functionActive=0;
			pEdit->state.mouseFlag=0;
		}
		else if( pEdit->state.key==ESC )
		{
			pEdit->state.functionActive=0;
			pEdit->editFlag=1;
		}
	}
	else if(pEdit->state.functionActive==13) /* Select-All action (Ctrl+C/X/BS) */
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
				/* Any other key cancels the page selection highlight cleanly */
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
	else if(pEdit->state.functionActive==15) /* Save-As filename confirmation (Y/N) */
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
