/*****************************************************************************
 * File: SPELL.C
 * Function: Chinese Pinyin Input Method Editor (IME) for DOS.
 * Programmer: XiaoYao (2011.9.25)
 *
 * Workflow:
 *   1. When `language == 1` and the user presses 'a'..'z', Spell() launches
 *      InputPinYin() to read a pinyin syllable (up to 10 chars) terminated by
 *      SPACE (or cancelled with ESC).
 *   2. OutputHanZi() loads the matching GB2312 candidate table from
 *      `spell/<pinyin>.txt` via GetCode(), displaying 4 candidates per page.
 *   3. '-' (JIANHAO) and '=' (DENGHAO) page backward/forward; '1'..'4' or
 *      SPACE commits the selected character via Gethanzi(); ESC cancels.
 *   4. Special Easter-egg / author glyph: 'àê' (not present in GB2312 HZK16)
 *      is encoded as two spaces (' ', ' ') with flag=2, selectable via pinyin
 *      "v" or slot 4 on page 8 of "yao", and drawn by OutputYao().
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

/*
 * Spell - Top-level entry point for one Chinese pinyin input cycle.
 * Clears the IME bar automatically if the user cancels pinyin entry.
 */
void Spell(struct EDITTYPE *pEdit)
{
	char pinYin[11];
	if(InputPinYin(pinYin, pEdit))
	{
		OutputHanZi(217, 457, pinYin, pEdit);
	}
	else
	{
		setfillstyle(SOLID_FILL, LIGHTGRAY);
		bar(100, 451, 400, 479);
	}
}

/*
 * InputPinYin - Read a pinyin syllable (up to 10 lowercase letters/digits)
 *               into `pinYin`, seeded with the initial letter in `state.ascii`.
 *
 * Returns: 1 if a non-empty pinyin was confirmed with SPACE, 0 if cancelled
 *          with ESC or confirmed while empty.
 */
int InputPinYin(char *pinYin, struct EDITTYPE *pEdit)
{
	long int key;
	int i=0;
	int letter;
	pinYin[0]='\0';
	while(1)
	{
		/* Consume the initial 'a'..'z' keypress that triggered Spell() */
		if( pEdit->state.ascii>='a'&&pEdit->state.ascii<='z' )
		{
			pinYin[0]=(char)pEdit->state.ascii;
			pinYin[1]='\0';
			pEdit->state.ascii='a'-1; /* Mark seed letter consumed */
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
			else if(key==SPACE) /* Confirm pinyin and open candidate list */
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
				pinYin[i]=(char)letter;
				pinYin[i+1]='\0';
				i++;
				Shurukuang(100, 455, 175, 475, pinYin);
			}
		}
	}
}

/*
 * DrawHanZiPage - Render up to 4 candidate Hanzi for 0-based `page` inside
 *                 the bottom candidate bar. For pinyin "yao" on page 8, also
 *                 draws the custom 'àê' bitmap in slot 4.
 */
static void DrawHanZiPage(int x, int y, char *code, int totalChars, int page, int isYao, struct EDITTYPE *pEdit)
{
	int k;
	int xx=x;
	int base=page*4;

	Hanzikuang(200, 455, 400, 475);
	for(k=0; k<4 && (base+k)<totalChars && xx<X_MAX; k++)
	{
		Displayhz(xx, y, &code[(base+k)*2], BLACK, pEdit);
		xx+=16*3;
	}
	if(isYao && page==8)
	{
		OutputYao(362, y, BLACK);
	}
}

/*
 * OutputHanZi - Display paginated GB2312 candidates for `pinYin` and commit
 *               the user's chosen character into the document.
 *
 * Uses 0-based integer page/slot indexing (`idx = page*4 + choice`) rather
 * than pointer arithmetic so out-of-range slots are safely ignored instead of
 * causing buffer overruns or NULL dereferences.
 */
void OutputHanZi(int x, int y, char *pinYin, struct EDITTYPE *pEdit)
{
	char code[300];
	long int key;
	int letter;
	int totalChars=0;
	int page=0;
	int isYao=0;
	int choice;

	code[0]='\0';

	/* Shorthand "v" inserts custom character 'àê' directly */
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
			else if(key==DENGHAO) /* '=' key: next page */
			{
				if((page+1)*4<totalChars)
				{
					page++;
					DrawHanZiPage(217, 457, code, totalChars, page, isYao, pEdit);
				}
			}
			else if(key==JIANHAO) /* '-' key: previous page */
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

/*
 * GetCode - Load the GB2312 candidate bytes for pinyin `pin` into `tcode`.
 *
 * Reads at most `maxLen - 1` bytes using an `int` buffer for fgetc() so EOF
 * cannot be confused with 0xFF bytes, stripping any CR/LF line breaks.
 * Returns: 1 on success, 0 if the pinyin dictionary file does not exist.
 */
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

/* Shurukuang - Draw the bottom pinyin composition box with current text */
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

/* Hanzikuang - Draw the numbered "1. 2. 3. 4." Hanzi candidate bar frame */
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

/*
 * Gethanzi - Insert the 2-byte GB2312 character stored in `pEdit->spell.pcode`
 *            as a document NODE with `flag = 2` at the active caret position.
 */
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

/* ShowLanguage - Draw the bottom-left EN ("”¢") / ZH ("∫∫") indicator button */
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

/*
 * OutputYao - Render the hardcoded 16x16 monochrome bitmap for 'àê' (not in
 *             GB2312 HZK16) at top-left (x0, y0) using the same byte-skipping
 *             scanline loop as Displayhz().
 */
void OutputYao(int x0, int y0, int color)
{
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
