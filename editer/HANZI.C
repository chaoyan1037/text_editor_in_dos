/*****************************************************************************
 * File: HANZI.C
 * Function: GB2312 16x16 bitmap font (HZK16) loading and pixel rendering.
 * Programmer: Xiao (2011.9.25)
 *
 * Font Layout (HZK16):
 *   - Stores 16x16 monochrome glyphs (32 bytes per character, 2 bytes per row,
 *     MSB = leftmost pixel) indexed by GB2312 Qu-Wei (zone-position) codes.
 *   - For a 2-byte GB2312 code (b0, b1) with b0 >= 0xA1 and b1 >= 0xA1:
 *       zone     = b0 - 0xA1  (0..86)
 *       position = b1 - 0xA1  (0..93)
 *       offset   = (94 * zone + position) * 32 bytes.
 *   - The font file is kept open in pEdit->spell.phz across redraws to avoid
 *     repeated DOS fopen/fclose overhead on every Chinese character.
 *****************************************************************************/

#include<graphics.h>
#include<string.h>
#include<stdio.h>
#include<conio.h>
#include"h/common.h"
#include"h/struct.h"
#include"h/hanzi.h"

/*
 * open_hz - Lazily open the HZK16 bitmap font file if not already open.
 * Tries the original Turbo C 3.0 installation path first, then falls back
 * to the repository-relative path "hanziku/HZK16".
 */
void open_hz(struct EDITTYPE *pEdit)
{
	if(pEdit->spell.phz!=NULL)
	{
		return;
	}
	pEdit->spell.phz=fopen("C://TC30//BIN//hanziku//HZK16", "rb");
	if(pEdit->spell.phz==NULL)
	{
		pEdit->spell.phz=fopen("hanziku/HZK16", "rb");
	}
	if(pEdit->spell.phz==NULL)
	{
		outtextxy(EDITERSCREENX1, EDITERSCREENY2+5, "Can not open hzk16.");
	}
}

/*
 * close_hz - Close the persistent HZK16 font file handle on editor exit.
 */
void close_hz(struct EDITTYPE *pEdit)
{
	if(pEdit->spell.phz!=NULL)
	{
		fclose(pEdit->spell.phz);
		pEdit->spell.phz=NULL;
	}
}

/*
 * Get_hz - Read the 32-byte 16x16 bitmap for one GB2312 character into bytes[].
 *
 * Validates that the font handle is open and both input bytes fall in the
 * GB2312 high-byte range (>= 0xA1). Uses 32-bit unsigned long arithmetic to
 * prevent 16-bit integer overflow under Turbo C. Outputs an all-zero bitmap
 * on invalid codes or short reads.
 */
void Get_hz(char incode[], char bytes[], struct EDITTYPE *pEdit)
{
	unsigned char b0, b1;
	unsigned long offset;

	memset(bytes, 0, 32);
	if(pEdit->spell.phz==NULL)
	{
		return;
	}
	b0=(unsigned char)incode[0];
	b1=(unsigned char)incode[1];
	if(b0<0xa1 || b1<0xa1)
	{
		return;
	}
	offset=(94UL*(unsigned long)(b0-0xa1)+(unsigned long)(b1-0xa1))*32UL;
	if(fseek(pEdit->spell.phz, (long)offset, SEEK_SET)==0)
	{
		if(fread(bytes, 32, 1, pEdit->spell.phz)!=1)
		{
			memset(bytes, 0, 32);
		}
	}
}

/*
 * Displayhz - Render a single 16x16 GB2312 character at top-left (x0, y0).
 *
 * Splits each 16-pixel scanline into two 8-bit halves (left: mat[2*i],
 * right: mat[2*i+1]) and skips entirely empty bytes to minimize putpixel()
 * calls and avoid division/modulo inside the inner loop.
 */
void Displayhz(int x0, int y0, char code[], int color, struct EDITTYPE *pEdit)
{
	int i, y, pos;
	char mat[32];
	unsigned char byteVal, bitMask;

	if(pEdit->spell.phz==NULL)
	{
		open_hz(pEdit);
		if(pEdit->spell.phz==NULL)
		{
			return;
		}
	}
	Get_hz(code, mat, pEdit);
	y=y0;
	for(i=0; i<16; i++)
	{
		pos=i<<1;

		/* Left 8 pixels of scanline i */
		byteVal=(unsigned char)mat[pos];
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

		/* Right 8 pixels of scanline i */
		byteVal=(unsigned char)mat[pos+1];
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

/*
 * Printfhz - Render a null-terminated GB2312 Chinese string starting at (x, y).
 *
 * Advances 16 pixels and 2 bytes per character. Stops cleanly if the right
 * screen edge (X_MAX) is reached or fewer than 2 bytes remain before NUL.
 */
void Printfhz(int x, int y, char *s, int color, struct EDITTYPE *pEdit)
{
	int x1=x, y1=y;
	if(s==NULL)
	{
		return;
	}
	open_hz(pEdit);
	if(pEdit->spell.phz==NULL)
	{
		return;
	}
	while( (x1<X_MAX) && (s[0]!='\0') && (s[1]!='\0') )
	{
		Displayhz(x1, y1, s, color, pEdit);
		x1=x1+16;
		s=s+2;
	}
}
