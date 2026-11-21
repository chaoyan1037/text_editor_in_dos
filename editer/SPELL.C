/*****************************************************************************
 * File: SPELL.C
 * Purpose: Chinese Pinyin Input Method Editor (IME) for DOS.
 * Programmer: XiaoYao (2011.9.25)
 *
 * Pinyin IME State Flow:
 *
 *   ['a'..'z' in ZH mode]
 *           |
 *           v
 *    InputPinYin()  ----(ESC or empty SPACE)----> clear IME bar & return
 *           |
 *      (SPACE with non-empty pinyin)
 *           |
 *           v
 *     OutputHanZi()
 *       |-- pinYin == "v" -------> insert custom 'àê' (' ',' ', flag=2)
 *       |-- GetCode(pinYin) -----> load GB2312 pairs from `spell/<pin>.txt`
 *       |        |
 *       |        v
 *       |   Candidate Loop (4 chars/page, integer index `idx = page*4 + slot`):
 *       |     '=' (DENGHAO) : next page if (page+1)*4 < totalChars
 *       |     '-' (JIANHAO) : prev page if page > 0
 *       |     '1'..'4'/SPACE: commit candidate via Gethanzi() & exit
 *       |     ESC           : cancel without inserting & exit
 *       |
 *       +-- Special case: `yao.txt` has 35 Hanzi (pages 0..7 full, page 8 has
 *           3 chars in slots 0..2); slot 3 on page 8 displays custom 'àê'.
 *****************************************************************************/
#include<graphics.h>
#include<string.h>
#include<stdio.h>
#include<bios.h>
#include"h/hanzi.h"
#include"h/common.h"
#include"h/struct.h"
#include"h/malloc.h"
#include"h/node.h"
#include"h/language.h"
#include"h/mouse.h"
#include"h/spell.h"

void Shurukuang(int x1, int y1, int x2, int y2, char *pinYin);
void Hanzikuang(int x1, int y1, int x2, int y2);
int InputPinYin(char *pinYin, struct EDITTYPE *pEdit);
void OutputHanZi(int x, int y, char *pinYin, struct EDITTYPE *pEdit);
int GetCode(char *tcode, int maxLen, char *pin);
static void DrawHanZiPage(int x, int y, char *code, int totalChars, int page, int isYao, struct EDITTYPE *pEdit);

/*****************************************************************************
 * Function: Spell
 * Purpose:  Execute one complete Chinese pinyin composition & selection cycle.
 *
 * @param pEdit Editor context with initial trigger letter in `state.ascii`.
 * @post        Either inserts one Hanzi node (setting `editFlag=1`) or clears
 *              the bottom IME bar cleanly on cancel/lookup failure.
 *****************************************************************************/
void Spell(struct EDITTYPE *pEdit)
{
	char pinYin[11];
	if(InputPinYin(pinYin, pEdit))
	{
		OutputHanZi(217, 457, pinYin, pEdit);
	}
	else
	{
		/* Erase pinyin box on ESC cancel or empty SPACE */
		setfillstyle(SOLID_FILL, LIGHTGRAY);
		bar(100, 451, 400, 479);
	}
}

/*****************************************************************************
 * Function: InputPinYin
 * Purpose:  Collect up to 10 lowercase pinyin letters/digits in the bottom
 *           composition box (X=100..175, Y=455..475).
 *
 * @param pinYin Output buffer (at least 11 bytes) receiving the syllable.
 * @param pEdit  Editor context supplying the first letter in `state.ascii`.
 * @return       1 if user pressed SPACE with >= 1 char; 0 if cancelled (ESC)
 *               or confirmed empty.
 *****************************************************************************/
int InputPinYin(char *pinYin, struct EDITTYPE *pEdit)
{
	long int key;
	int i=0;
	int letter;
	pinYin[0]='\0';
	while(1)
	{
		/* Seed composition buffer with the initial 'a'..'z' key from main() */
		if( pEdit->state.ascii>='a'&&pEdit->state.ascii<='z' )
		{
			pinYin[0]=(char)pEdit->state.ascii;
			pinYin[1]='\0';
			pEdit->state.ascii='a'-1; /* Sentinel: seed already consumed */
			Shurukuang(100, 455, 175, 475, pinYin);
			i=1;
		}

		if(bioskey(1)!=0)
		{
			key=bioskey(0);
			letter=(int)(key&0x00ff);

			if(key==ESC)
			{
				pinYin[0]='\0';
				return 0;
			}
			else if(key==SPACE) /* Finish syllable and show candidates */
			{
				pinYin[i]='\0';
				Shurukuang(100, 455, 175, 475, pinYin);
				return (i>0);
			}
			else if(key==BACKSPACE)
			{
				if(i>0)
				{
					i--;
					pinYin[i]='\0';
					Shurukuang(100, 455, 175, 475, pinYin);
				}
			}
			else if( ((letter>='a'&&letter<='z')||(letter>='0'&&letter<='9')) && i<10 )
			{
				/* Digits '0'..'9' are supported for symbol tables v1.txt..v9.txt */
				pinYin[i]=(char)letter;
				pinYin[i+1]='\0';
				i++;
				Shurukuang(100, 455, 175, 475, pinYin);
			}
		}
	}
}

/*****************************************************************************
 * Function: DrawHanZiPage
 * Purpose:  Repaint the candidate bar with up to 4 Hanzi glyphs for `page`.
 *
 * @param x          Left X pixel for slot 0 (typically 217, stride 48 px).
 * @param y          Top Y pixel for candidate glyphs (typically 457).
 * @param code       Contiguous buffer of 2-byte GB2312 candidate pairs.
 * @param totalChars Total number of GB2312 characters in `code`.
 * @param page       0-based page index (each page displays `page*4 .. page*4+3`).
 * @param isYao      1 if `pinYin` is "yao" (enables 'àê' in slot 3 on page 8).
 * @param pEdit      Editor context providing HZK16 font access.
 *****************************************************************************/
static void DrawHanZiPage(int x, int y, char *code, int totalChars, int page, int isYao, struct EDITTYPE *pEdit)
{
	int k;
	int xx=x;
	int base=page*4;

	Hanzikuang(200, 455, 400, 475);
	for(k=0; k<4 && (base+k)<totalChars && xx<X_MAX; k++)
	{
		Displayhz(xx, y, &code[(base+k)*2], BLACK, pEdit);
		xx+=16*3; /* 16px glyph + 32px spacing for "N. " label */
	}
	if(isYao && page==8)
	{
		OutputYao(362, y, BLACK);
	}
}

/*****************************************************************************
 * Function: OutputHanZi
 * Purpose:  Look up GB2312 candidates for `pinYin`, run the interactive page
 *           selection loop, and insert the chosen character via Gethanzi().
 *
 * @param x      Initial candidate slot X coordinate (217).
 * @param y      Candidate glyph top Y coordinate (457).
 * @param pinYin NUL-terminated pinyin syllable entered by the user.
 * @param pEdit  Master editor context.
 * @note         Uses bounds-checked integer indexing (`idx = page*4 + choice`)
 *               so pressing a key for an empty trailing slot is safely ignored.
 *****************************************************************************/
void OutputHanZi(int x, int y, char *pinYin, struct EDITTYPE *pEdit)
{
	char code[300]; /* Largest dictionary file (yi.txt) is 220 bytes */
	long int key;
	int letter;
	int totalChars=0;
	int page=0;
	int isYao=0;
	int choice;

	code[0]='\0';

	/* Shorthand pinyin "v" inserts custom character 'àê' directly */
	if( (pinYin[0]=='v')&&(pinYin[1]=='\0') )
	{
		pEdit->spell.pcode[0]=' ';
		pEdit->spell.pcode[1]=' ';
		Gethanzi(pEdit);
		setfillstyle(SOLID_FILL, LIGHTGRAY);
		bar(100, 451, 400, 479);
		return;
	}

	if(GetCode(code, (int)sizeof(code), pinYin)!=1)
	{
		Shurukuang(100, 455, 175, 475, "");
		return;
	}

	totalChars=(int)(strlen(code)/2);
	if(totalChars<=0)
	{
		Shurukuang(100, 455, 175, 475, "");
		return;
	}

	isYao=(strcmp(pinYin, "yao")==0);
	open_hz(pEdit);
	DrawHanZiPage(x, y, code, totalChars, page, isYao, pEdit);

	/* Modal candidate selection loop */
	while(1)
	{
		if(bioskey(1)!=0)
		{
			key=bioskey(0);
			letter=(int)(key&0x00ff);

			if(key==ESC)
			{
				setfillstyle(SOLID_FILL, LIGHTGRAY);
				bar(100, 451, 400, 479);
				break;
			}
			else if(key==DENGHAO) /* '=' advances to next page of 4 candidates */
			{
				if((page+1)*4<totalChars)
				{
					page++;
					DrawHanZiPage(217, 457, code, totalChars, page, isYao, pEdit);
				}
			}
			else if(key==JIANHAO) /* '-' returns to previous page */
			{
				if(page>0)
				{
					page--;
					DrawHanZiPage(217, 457, code, totalChars, page, isYao, pEdit);
				}
			}
			else if(key==SPACE || (letter>='1' && letter<='4'))
			{
				int idx;
				choice=(key==SPACE) ? 0 : (letter-'1');

				/* Special slot 4 on page 8 of "yao" commits custom 'àê' */
				if(isYao && page==8 && choice==3)
				{
					pEdit->spell.pcode[0]=' ';
					pEdit->spell.pcode[1]=' ';
					Gethanzi(pEdit);
					setfillstyle(SOLID_FILL, LIGHTGRAY);
					bar(100, 451, 400, 479);
					break;
				}

				idx=page*4+choice;
				if(idx<totalChars)
				{
					pEdit->spell.pcode[0]=code[idx*2];
					pEdit->spell.pcode[1]=code[idx*2+1];
					Gethanzi(pEdit);
					setfillstyle(SOLID_FILL, LIGHTGRAY);
					bar(100, 451, 400, 479);
					break;
				}
			}
		}
	}
}

/*****************************************************************************
 * Function: GetCode
 * Purpose:  Read the GB2312 candidate table `spell/<pin>.txt` into `tcode`.
 *
 * @param tcode  Destination buffer for NUL-terminated GB2312 byte pairs.
 * @param maxLen Total capacity of `tcode` in bytes (including NUL).
 * @param pin    Pinyin syllable name (up to 10 chars, without ".txt").
 * @return       1 on success; 0 if file is missing or arguments are invalid.
 * @note         Uses `int ch = fgetc(fp)` so EOF (-1) is never confused with
 *               high bytes (0x80..0xFF), and strips CR/LF bytes.
 *****************************************************************************/
int GetCode(char *tcode, int maxLen, char *pin)
{
	int i=0;
	int ch;
	char routine[64];
	FILE *fp=NULL;

	if(tcode==NULL || maxLen<=1 || pin==NULL || pin[0]=='\0')
	{
		if(tcode!=NULL && maxLen>0)
		{
			tcode[0]='\0';
		}
		return 0;
	}

	/* Try original TC 3.0 spell path first, then relative "spell/" */
	sprintf(routine, "C://TC30//BIN//spell//%.10s.txt", pin);
	fp=fopen(routine, "rt");
	if(fp==NULL)
	{
		sprintf(routine, "spell/%.10s.txt", pin);
		fp=fopen(routine, "rt");
	}
	if(fp==NULL)
	{
		tcode[0]='\0';
		setcolor(RED);
		outtextxy(240, 470, "error,input again.");
		return 0;
	}

	while( i<maxLen-1 && (ch=fgetc(fp))!=EOF )
	{
		if(ch=='\r' || ch=='\n')
		{
			continue;
		}
		tcode[i++]=(char)ch;
	}
	tcode[i]='\0';
	fclose(fp);
	return 1;
}

/* Shurukuang - Repaint the recessed green pinyin box with BLACK text */
void Shurukuang(int x1, int y1, int x2, int y2, char *pinYin)
{
	setfillstyle(SOLID_FILL, LIGHTGREEN);
	bar(x1, y1, x2, y2);
	setcolor(BLACK);
	outtextxy(x1+2, y2-2, pinYin);
	setcolor(WHITE);
	rectangle(x1, y1, x2, y2);
	WhiteBlack(x1, y1, x2, y2, BLACK);
}

/* Hanzikuang - Repaint the green candidate bar with "1. 2. 3. 4." slot labels */
void Hanzikuang(int x1, int y1, int x2, int y2)
{
	setfillstyle(SOLID_FILL, LIGHTGREEN);
	bar(x1, y1, x2, y2);
	setcolor(BLACK);
	outtextxy(x1+2, y2-2, "1.    2.    3.    4.");
	setcolor(WHITE);
	rectangle(x1, y1, x2, y2);
	WhiteBlack(x1, y1, x2, y2, BLACK);
}

/*****************************************************************************
 * Function: Gethanzi
 * Purpose:  Insert the 2-byte GB2312 glyph in `pEdit->spell.pcode[0..1]` at
 *           the active caret position as a `flag = 2` document node.
 *
 * @param pEdit Master editor context with `spell.pcode[0..1]` populated.
 * @post        Inserts node at `pEdit->current` and sets `editFlag=1`,
 *              `cursorFlag=1`. Safe when document is empty or `current==NULL`.
 *****************************************************************************/
void Gethanzi(struct EDITTYPE *pEdit)
{
	char data[3]="hh";
	pEdit->editFlag=1;
	pEdit->cursorFlag=1;
	data[0]=pEdit->spell.pcode[0];
	data[1]=pEdit->spell.pcode[1];
	data[2]='\0';

	if( pEdit->start==NULL )
	{
		pEdit->start=MallocNode(pEdit);
		pEdit->start->data[0]=data[0];
		pEdit->start->data[1]=data[1];
		pEdit->start->flag=2;
		pEdit->start->firstInLine=1;
		pEdit->pageFirst=pEdit->start;
		pEdit->last=pEdit->current=pEdit->start;
	}
	else if( pEdit->current!=NULL && pEdit->current->next==NULL )
	{
		AddNode(pEdit, data);
		pEdit->current->flag=2;
	}
	else
	{
		InsertNode(pEdit, data);
		pEdit->current->flag=2;
	}
}

/*****************************************************************************
 * Function: ShowLanguage
 * Purpose:  Draw the bottom-left EN ("”¢") / ZH ("∫∫") indicator button
 *           (X=5..28, Y=455..475) and clear the IME bar when switching to EN.
 *
 * @param pEdit Master editor context providing `language`.
 *****************************************************************************/
void ShowLanguage(struct EDITTYPE *pEdit)
{
	setfillstyle(SOLID_FILL, LIGHTGRAY);
	setwritemode(COPY_PUT);
	bar(4, 451, 40, 479);
	setcolor(BLACK);
	rectangle(5, 455, 28, 475);
	WhiteBlack(5, 455, 28, 475, WHITE);
	if(pEdit->language==0)
	{
		Printfhz(10, 458, "”¢", RED, pEdit);
		setfillstyle(SOLID_FILL, LIGHTGRAY);
		bar(100, 451, 400, 479);
	}
	else
	{
		Printfhz(10, 458, "∫∫", RED, pEdit);
	}
}

/*****************************************************************************
 * Function: OutputYao
 * Purpose:  Plot the hardcoded 16x16 dot-matrix bitmap for 'àê' (three Õ¡
 *           radicals, absent from GB2312-80 HZK16) at top-left (x0, y0).
 *
 * @param x0    Left pixel coordinate of the 16x16 glyph cell.
 * @param y0    Top pixel coordinate of the 16x16 glyph cell.
 * @param color BGI palette color index.
 *****************************************************************************/
void OutputYao(int x0, int y0, int color)
{
	/* 16 rows x 2 bytes/row (MSB = leftmost pixel), identical to HZK16 layout */
	static const unsigned char mat[32]={
		0x01,0x00,0x01,0x00,0x3F,0xF8,0x01,0x00,
		0x01,0x00,0x01,0x00,0xFF,0xFE,0x10,0x10,
		0x10,0x10,0x10,0x10,0x7C,0xFC,0x10,0x10,
		0x10,0x10,0x1E,0x10,0xF0,0xFE,0x40,0x00
	};
	int y=y0, i, pos;
	unsigned char byteVal, bitMask;

	for(i=0; i<16; i++)
	{
		pos=i<<1;
		byteVal=mat[pos];
		if(byteVal!=0)
		{
			int x=x0;
			for(bitMask=0x80; bitMask!=0; bitMask>>=1)
			{
				if((byteVal&bitMask)!=0)
				{
					putpixel(x, y, color);
				}
				++x;
			}
		}
		byteVal=mat[pos+1];
		if(byteVal!=0)
		{
			int x=x0+8;
			for(bitMask=0x80; bitMask!=0; bitMask>>=1)
			{
				if((byteVal&bitMask)!=0)
				{
					putpixel(x, y, color);
				}
				++x;
			}
		}
		++y;
	}
}
