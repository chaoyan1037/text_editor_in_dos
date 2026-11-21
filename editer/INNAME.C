/*****************************************************************************
 * File: INNAME.C
 * Purpose: Modal DOS 8.3 filename input dialog used by Open (Ctrl+O) and
 *          Save As (Ctrl+D).
 * Programmer: Xiao (2011.10.11)
 *****************************************************************************/

#include<graphics.h>
#include<bios.h>
#include"h/struct.h"
#include"h/common.h"
#include"h/mouse.h"
#include"h/hanzi.h"
#include"h/inname.h"

/*****************************************************************************
 * Function: InName
 * Purpose:  Draw a centered modal prompt (240..380 x 160..240) and collect up
 *           to 12 printable ASCII characters into `pEdit->spell.name`.
 *
 * @param pEdit Editor context whose `spell.name` buffer (16 bytes) receives
 *              the NUL-terminated filename.
 * @post        - On ENTER: `spell.name` contains the typed filename (0..12 chars).
 *              - On ESC:   `spell.name[0] = '\0'` (caller cancels operation).
 *              - BACKSPACE works even when all 12 characters have been typed.
 *****************************************************************************/
void InName(struct EDITTYPE *pEdit)
{
	long int key;
	int i=0;
	int letter;
	pEdit->spell.name[0]='\0';

	/* Outer dialog box (240..380 x 160..240) */
	setfillstyle(SOLID_FILL, LIGHTGRAY);
	bar(240, 160, 380, 240);
	button3d(240, 160, 380, 240);

	/* Left OK(Y) button (246..306 x 214..232) and Right Cancel(N) (314..374) */
	setcolor(BLACK);
	rectangle(240+6, 160+54, 380-74, 240-8);
	WhiteBlack(240+6, 160+54, 380-74, 240-8, WHITE);
	setcolor(BLACK);
	rectangle(245+69, 160+54, 380-6, 240-8);
	WhiteBlack(245+69, 160+54, 380-6, 240-8, WHITE);
	setcolor(BLACK);

	/* Prompt header and recessed green input field (250..362 x 185..205) */
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

	/* Poll BIOS keyboard until user confirms (ENTER) or aborts (ESC) */
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
				/* Safe: spell.name has 16 bytes, max index written is 12 */
				pEdit->spell.name[i]=(char)letter;
				pEdit->spell.name[i+1]='\0';
				i++;
				Frame1(240+10, 160+25, 380-18, 240-35, pEdit);
			}
		}
	}
}

/*****************************************************************************
 * Function: Frame1
 * Purpose:  Clear and repaint the filename text box with `pEdit->spell.name`.
 *
 * @param x1,y1,x2,y2 Bounding box of the recessed input field.
 * @param pEdit       Editor context supplying `spell.name`.
 *****************************************************************************/
void Frame1(int x1, int y1, int x2, int y2, struct EDITTYPE *pEdit)
{
	setfillstyle(SOLID_FILL, LIGHTGREEN);
	bar(x1, y1, x2, y2);
	setcolor(WHITE);
	rectangle(x1, y1, x2, y2);
	WhiteBlack(x1, y1, x2, y2, BLACK); /* Also sets color to BLACK for text */
	outtextxy(x1+2, y2-2, pEdit->spell.name);
}
