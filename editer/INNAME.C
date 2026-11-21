/*****************************************************************************
 * File: INNAME.C
 * Function: Modal dialog to read a DOS 8.3 filename (up to 12 characters)
 *           into `pEdit->spell.name` for Open and Save As operations.
 * Programmer: Xiao (2011.10.11)
 *****************************************************************************/

#include<graphics.h>
#include<bios.h>
#include"h/struct.h"
#include"h/common.h"
#include"h/mouse.h"
#include"h/hanzi.h"
#include"h/inname.h"

/*
 * InName - Draw the filename prompt dialog and poll BIOS keyboard input.
 *
 * Accepts up to 12 printable ASCII characters (matching DOS 8.3 format) and
 * stores a NUL-terminated string in `pEdit->spell.name` (size 16).
 *   - ENTER: confirms typed text and exits loop (caller handles Y/N confirm).
 *   - ESC:   clears `pEdit->spell.name` and cancels the dialog immediately.
 *   - BACKSPACE: deletes the last typed character.
 */
void InName(struct EDITTYPE *pEdit)
{
	long int key;
	int i=0;
	int letter;
	pEdit->spell.name[0]='\0';

	/* Dialog frame and OK / Cancel buttons */
	setfillstyle(SOLID_FILL, LIGHTGRAY);
	bar(240, 160, 380, 240);
	button3d(240, 160, 380, 240);

	setcolor(BLACK);
	rectangle(240+6, 160+54, 380-74, 240-8);
	WhiteBlack(240+6, 160+54, 380-74, 240-8, WHITE);
	setcolor(BLACK);
	rectangle(245+69, 160+54, 380-6, 240-8);
	WhiteBlack(245+69, 160+54, 380-6, 240-8, WHITE);
	setcolor(BLACK);

	/* Prompt label and green text input box */
	Printfhz(250, 165, "文件名？", RED, pEdit);
	setfillstyle(SOLID_FILL, LIGHTGREEN);
	bar(240+10, 160+25, 380-18, 240-35);

	setcolor(WHITE);
	rectangle(240+10, 160+25, 380-18, 240-35);
	WhiteBlack(240+10, 160+25, 380-18, 240-35, BLACK);
	Printfhz(240+8, 160+55, "确定", RED, pEdit);
	Printfhz(245+71, 160+55, "取消", RED, pEdit);

	setcolor(RED);
	outtextxy(283, 230, "(Y)");
	outtextxy(351, 230, "(N)");

	/* Modal keyboard input loop */
	while(1)
	{
		if(bioskey(1)!=0)
		{
			key=bioskey(0);
			letter=(int)(key&0x00ff);

			if(key==ENTER)
			{
				pEdit->spell.name[i]='\0';
				Frame1(240+10, 160+25, 380-18, 240-35, pEdit);
				break;
			}
			else if(key==ESC)
			{
				pEdit->spell.name[0]='\0';
				break;
			}
			else if(key==BACKSPACE)
			{
				if(i>0)
				{
					i--;
					pEdit->spell.name[i]='\0';
					Frame1(240+10, 160+25, 380-18, 240-35, pEdit);
				}
			}
			else if( (letter>=0x20)&&(letter<=0x7e)&&(i<12) )
			{
				pEdit->spell.name[i]=(char)letter;
				pEdit->spell.name[i+1]='\0';
				i++;
				Frame1(240+10, 160+25, 380-18, 240-35, pEdit);
			}
		}
	}
}

/*
 * Frame1 - Clear and redraw the filename input box with its current text.
 */
void Frame1(int x1, int y1, int x2, int y2, struct EDITTYPE *pEdit)
{
	setfillstyle(SOLID_FILL, LIGHTGREEN);
	bar(x1, y1, x2, y2);
	setcolor(WHITE);
	rectangle(x1, y1, x2, y2);
	WhiteBlack(x1, y1, x2, y2, BLACK);
	outtextxy(x1+2, y2-2, pEdit->spell.name);
}
