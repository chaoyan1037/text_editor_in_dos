/*****************************************************************************
 * File: INNAME.C
 * Purpose: Modal filename input dialog (Save As) and interactive SAVE/ file
 *          browser dialog (Open) with keyboard and mouse support.
 * Programmer: Xiao (2011.10.11); enhanced with SAVE/ directory listing.
 *****************************************************************************/

#include<graphics.h>
#include<bios.h>
#include<dirent.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"h/struct.h"
#include"h/common.h"
#include"h/mouse.h"
#include"h/hanzi.h"
#include"h/inname.h"

#define MAX_LIST_FILES 64
#define VISIBLE_FILES  8

/* Dialog geometry (centered in 640x480 / 80x28 terminal grid):
 *   Box:        X=160..480 (cols 20..60), Y=98..354 (rows 5..20)
 *   Header:     Y=100 (row 5)
 *   File list:  Y=130..257 (rows 7..14, 8 visible entries x 16px)
 *   Input box:  X=176..464, Y=274..290 (row 16)
 *   Hint/Btns:  Y=306..338 (rows 18..19) */
#define DLG_X1 160
#define DLG_Y1 98
#define DLG_X2 480
#define DLG_Y2 354
#define LIST_Y 130
#define INPUT_X1 176
#define INPUT_Y1 274
#define INPUT_X2 464
#define INPUT_Y2 290

static int cmp_filenames(const void *a, const void *b)
{
	return strcmp((const char *)a, (const char *)b);
}

/*
 * load_save_files - Populate `files[0..count-1]` with sorted regular entries
 *                   found inside the `SAVE/` directory.
 */
static int load_save_files(char files[MAX_LIST_FILES][32])
{
	DIR *dir=opendir("SAVE");
	struct dirent *ent;
	int count=0;

	if(dir==NULL)
	{
		return 0;
	}
	while((ent=readdir(dir))!=NULL && count<MAX_LIST_FILES)
	{
		if(ent->d_name[0]=='.')
		{
			continue; /* Skip ".", "..", ".DS_Store", hidden files */
		}
		strncpy(files[count], ent->d_name, 31);
		files[count][31]='\0';
		count++;
	}
	closedir(dir);

	if(count>1)
	{
		qsort(files, (size_t)count, sizeof(files[0]), cmp_filenames);
	}
	return count;
}

/*
 * draw_file_list - Render `VISIBLE_FILES` filenames from `files[scroll..]`
 *                  inside the dialog, highlighting `selected` when >= 0.
 */
static void draw_file_list(char files[MAX_LIST_FILES][32], int count, int scroll, int selected)
{
	int row;

	setfillstyle(SOLID_FILL, WHITE);
	bar(DLG_X1+16, LIST_Y, DLG_X2-16, LIST_Y+VISIBLE_FILES*CHARHIGH);

	if(count==0)
	{
		setcolor(DARKGRAY);
		settextjustify(LEFT_TEXT, BOTTOM_TEXT);
		outtextxy(DLG_X1+24, LIST_Y+CHARHIGH, "(SAVE/ is empty)");
		return;
	}

	settextjustify(LEFT_TEXT, BOTTOM_TEXT);
	for(row=0; row<VISIBLE_FILES && (scroll+row)<count; row++)
	{
		int idx=scroll+row;
		int topY=LIST_Y+row*CHARHIGH;
		if(idx==selected)
		{
			setfillstyle(SOLID_FILL, BLUE);
			bar(DLG_X1+16, topY, DLG_X2-16, topY+CHARHIGH);
			setcolor(WHITE);
		}
		else
		{
			setcolor(BLACK);
		}
		outtextxy(DLG_X1+24, topY+CHARHIGH, files[idx]);
	}
}

/*
 * run_file_dialog - Shared modal filename dialog for both Open (`selectMode=1`)
 *                   and Save As (`selectMode=0`).
 *
 *   - Lists all existing files in `SAVE/` (scrollable via UP/DOWN arrows).
 *   - In Open mode (`selectMode=1`), item 0 is highlighted by default so
 *     pressing UP/DOWN + ENTER opens a file with zero typing; typing any
 *     character switches smoothly to custom filename input.
 *   - Clicking a file row with the mouse selects and opens/populates it.
 *   - ENTER confirms; ESC cancels (sets `pEdit->spell.name[0] = '\0'`).
 */
static void run_file_dialog(struct EDITTYPE *pEdit, int selectMode)
{
	char files[MAX_LIST_FILES][32];
	int count;
	int selected;
	int scroll=0;
	int i=0;
	int userTyped=0;
	long int key;
	int letter;

	count=load_save_files(files);
	selected=(selectMode && count>0) ? 0 : -1;

	if(selected>=0)
	{
		strncpy(pEdit->spell.name, files[selected], sizeof(pEdit->spell.name)-1);
		pEdit->spell.name[sizeof(pEdit->spell.name)-1]='\0';
		i=(int)strlen(pEdit->spell.name);
	}
	else
	{
		pEdit->spell.name[0]='\0';
		i=0;
	}

	/* Outer dialog frame */
	setfillstyle(SOLID_FILL, LIGHTGRAY);
	bar(DLG_X1, DLG_Y1, DLG_X2, DLG_Y2);
	button3d(DLG_X1, DLG_Y1, DLG_X2, DLG_Y2);

	/* Dialog header showing SAVE/ directory */
	settextjustify(LEFT_TEXT, BOTTOM_TEXT);
	if(selectMode)
	{
		Printfhz(DLG_X1+16, DLG_Y1+2, "\xb4\xf2\xbf\xaa", RED, pEdit);
		setcolor(BLACK);
		outtextxy(DLG_X1+56, DLG_Y1+16, "(SAVE/ - Up/Down, Enter, Esc)");
	}
	else
	{
		Printfhz(DLG_X1+16, DLG_Y1+2, "\xc1\xed\xb4\xe6\xce\xaa", RED, pEdit);
		setcolor(BLACK);
		outtextxy(DLG_X1+72, DLG_Y1+16, "(SAVE/ - Enter=Save, Esc)");
	}

	draw_file_list(files, count, scroll, selected);
	Frame1(INPUT_X1, INPUT_Y1, INPUT_X2, INPUT_Y2, pEdit);

	/* Footer buttons: OK (Enter) / Cancel (Esc) */
	setcolor(BLACK);
	rectangle(DLG_X1+32, DLG_Y2-36, DLG_X1+136, DLG_Y2-18);
	WhiteBlack(DLG_X1+32, DLG_Y2-36, DLG_X1+136, DLG_Y2-18, WHITE);
	Printfhz(DLG_X1+38, DLG_Y2-34, "\xc8\xb7\xb6\xa8", RED, pEdit);
	setcolor(RED);
	outtextxy(DLG_X1+74, DLG_Y2-20, "(Enter)");

	setcolor(BLACK);
	rectangle(DLG_X2-136, DLG_Y2-36, DLG_X2-32, DLG_Y2-18);
	WhiteBlack(DLG_X2-136, DLG_Y2-36, DLG_X2-32, DLG_Y2-18, WHITE);
	Printfhz(DLG_X2-130, DLG_Y2-34, "\xc8\xa1\xcf\xfb", RED, pEdit);
	setcolor(RED);
	outtextxy(DLG_X2-94, DLG_Y2-20, "(Esc)");

	while(1)
	{
		/* Handle mouse clicks inside the file list or OK/Cancel buttons */
		ReadMouse(pEdit);
		if(pEdit->state.button==1)
		{
			int mx=pEdit->state.x;
			int my=pEdit->state.y;
			if(mx>=DLG_X1+16 && mx<=DLG_X2-16 && my>=LIST_Y && my<LIST_Y+VISIBLE_FILES*CHARHIGH)
			{
				int row=(my-LIST_Y)/CHARHIGH;
				int idx=scroll+row;
				if(idx>=0 && idx<count)
				{
					strncpy(pEdit->spell.name, files[idx], sizeof(pEdit->spell.name)-1);
					pEdit->spell.name[sizeof(pEdit->spell.name)-1]='\0';
					if(selectMode)
					{
						break; /* Clicking a file in Open mode opens it immediately */
					}
					selected=idx;
					i=(int)strlen(pEdit->spell.name);
					userTyped=1;
					draw_file_list(files, count, scroll, selected);
					Frame1(INPUT_X1, INPUT_Y1, INPUT_X2, INPUT_Y2, pEdit);
				}
			}
			else if(mx>=DLG_X1+32 && mx<=DLG_X1+136 && my>=DLG_Y2-36 && my<=DLG_Y2-18)
			{
				break; /* OK button */
			}
			else if(mx>=DLG_X2-136 && mx<=DLG_X2-32 && my>=DLG_Y2-36 && my<=DLG_Y2-18)
			{
				pEdit->spell.name[0]='\0';
				break; /* Cancel button */
			}
		}

		if(bioskey(1)!=0)
		{
			key=bioskey(0);
			letter=(key & HANZI_KEY_TAG) ? 0 : (int)(key&0x00ff);

			if(key==ENTER)
			{
				pEdit->spell.name[i]='\0';
				Frame1(INPUT_X1, INPUT_Y1, INPUT_X2, INPUT_Y2, pEdit);
				break;
			}
			else if(key==ESC)
			{
				pEdit->spell.name[0]='\0';
				break;
			}
			else if(key==UP && count>0)
			{
				if(selected<0) selected=0;
				else if(selected>0) selected--;
				if(selected<scroll) scroll=selected;
				strncpy(pEdit->spell.name, files[selected], sizeof(pEdit->spell.name)-1);
				pEdit->spell.name[sizeof(pEdit->spell.name)-1]='\0';
				i=(int)strlen(pEdit->spell.name);
				userTyped=0;
				draw_file_list(files, count, scroll, selected);
				Frame1(INPUT_X1, INPUT_Y1, INPUT_X2, INPUT_Y2, pEdit);
			}
			else if(key==DOWN && count>0)
			{
				if(selected<0) selected=0;
				else if(selected<count-1) selected++;
				if(selected>=scroll+VISIBLE_FILES) scroll=selected-VISIBLE_FILES+1;
				strncpy(pEdit->spell.name, files[selected], sizeof(pEdit->spell.name)-1);
				pEdit->spell.name[sizeof(pEdit->spell.name)-1]='\0';
				i=(int)strlen(pEdit->spell.name);
				userTyped=0;
				draw_file_list(files, count, scroll, selected);
				Frame1(INPUT_X1, INPUT_Y1, INPUT_X2, INPUT_Y2, pEdit);
			}
			else if(key==BACKSPACE)
			{
				if(i>0)
				{
					i--;
					pEdit->spell.name[i]='\0';
					userTyped=1;
					Frame1(INPUT_X1, INPUT_Y1, INPUT_X2, INPUT_Y2, pEdit);
				}
			}
			else if( (letter>=0x20)&&(letter<=0x7e) )
			{
				/* In Open mode, first typed character replaces auto-filled list selection */
				if(selectMode && !userTyped)
				{
					i=0;
					pEdit->spell.name[0]='\0';
					selected=-1;
					draw_file_list(files, count, scroll, selected);
				}
				userTyped=1;
				if(i<24)
				{
					pEdit->spell.name[i]=(char)letter;
					pEdit->spell.name[i+1]='\0';
					i++;
					Frame1(INPUT_X1, INPUT_Y1, INPUT_X2, INPUT_Y2, pEdit);
				}
			}
		}
	}
}

/*****************************************************************************
 * Function: InName
 * Purpose:  Show the Save As dialog listing existing `SAVE/` files and read
 *           a target filename into `pEdit->spell.name`.
 *****************************************************************************/
void InName(struct EDITTYPE *pEdit)
{
	run_file_dialog(pEdit, 0);
}

/*****************************************************************************
 * Function: InNameOpen
 * Purpose:  Show the Open File browser listing all files in `SAVE/` so the
 *           user can pick with UP/DOWN/mouse or type a filename directly.
 *****************************************************************************/
void InNameOpen(struct EDITTYPE *pEdit)
{
	run_file_dialog(pEdit, 1);
}

/*****************************************************************************
 * Function: Frame1
 * Purpose:  Clear and repaint the filename text box with `pEdit->spell.name`.
 *****************************************************************************/
void Frame1(int x1, int y1, int x2, int y2, struct EDITTYPE *pEdit)
{
	setfillstyle(SOLID_FILL, LIGHTGREEN);
	bar(x1, y1, x2, y2);
	setcolor(WHITE);
	rectangle(x1, y1, x2, y2);
	WhiteBlack(x1, y1, x2, y2, BLACK);
	settextjustify(LEFT_TEXT, BOTTOM_TEXT);
	outtextxy(x1+4, y2, pEdit->spell.name);
}
