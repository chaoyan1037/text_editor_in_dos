/*****************************************************************************
 * File: HANZI.C
 * Purpose: GB2312 16x16 monochrome bitmap font (HZK16) loader and renderer.
 * Programmer: Xiao (2011.9.25)
 *
 * HZK16 Binary File Format & Glyph Layout:
 *   - DOS provides no built-in Chinese font ROM, so Chinese text is drawn
 *     pixel-by-pixel from the standard GB2312-80 16x16 dot-matrix library
 *     file `HZK16`.
 *   - GB2312 encodes each character as two bytes (b0, b1), each in 0xA1..0xFE:
 *       zone (Qu, 0-based)     = b0 - 0xA1  (0..86)
 *       position (Wei, 0-based)= b1 - 0xA1  (0..93)
 *       byte offset in HZK16   = (94 * zone + position) * 32
 *   - Each 32-byte record encodes 16 rows x 2 bytes/row (MSB = left pixel):
 *
 *       Row i (0..15):
 *         mat[2*i]     -> pixels x0+0 .. x0+7  (bit 7=0x80 .. bit 0=0x01)
 *         mat[2*i + 1] -> pixels x0+8 .. x0+15 (bit 7=0x80 .. bit 0=0x01)
 *
 * Performance Optimization:
 *   - `pEdit->spell.phz` is opened once lazily and kept open until exit,
 *     avoiding hundreds of DOS fopen/fclose syscalls during ViewAll().
 *   - Displayhz() skips any zero byte outright and shifts a bitmask instead
 *     of computing `j / 8` and `j % 8` 256 times per character.
 *****************************************************************************/

#include<graphics.h>
#include<string.h>
#include<stdio.h>
#include<conio.h>
#include"h/common.h"
#include"h/struct.h"
#include"h/hanzi.h"

/*****************************************************************************
 * Function: open_hz
 * Purpose:  Lazily open the HZK16 font file into `pEdit->spell.phz`.
 *
 * @param pEdit Editor context holding the persistent `spell.phz` handle.
 * @post        If already open, returns immediately (O(1) no-op). Otherwise
 *              tries the TC 3.0 path first, then relative "hanziku/HZK16".
 *              Leaves `spell.phz == NULL` and prints a status notice if both
 *              paths fail (callers safely skip drawing when NULL).
 *****************************************************************************/
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

/*****************************************************************************
 * Function: close_hz
 * Purpose:  Close the persistent HZK16 font file handle on shutdown/OOM.
 *
 * @param pEdit Editor context whose `spell.phz` handle will be closed.
 * @post        `pEdit->spell.phz == NULL`. Safe to call multiple times.
 *****************************************************************************/
void close_hz(struct EDITTYPE *pEdit)
{
	if(pEdit->spell.phz!=NULL)
	{
		fclose(pEdit->spell.phz);
		pEdit->spell.phz=NULL;
	}
}

/*****************************************************************************
 * Function: Get_hz
 * Purpose:  Fetch the 32-byte 16x16 bitmap for one GB2312 character.
 *
 * @param incode 2-byte GB2312 code (`incode[0]`, `incode[1]`).
 * @param bytes  Caller-allocated 32-byte output buffer receiving the glyph.
 * @param pEdit  Editor context providing the open `spell.phz` handle.
 * @post         `bytes[0..31]` holds the glyph bitmap, or all zeros if the
 *               font is unavailable, the code is outside GB2312 (>= 0xA1),
 *               or fseek/fread fails.
 *****************************************************************************/
void Get_hz(char incode[], char bytes[], struct EDITTYPE *pEdit)
{
	unsigned char b0, b1;
	unsigned long offset;

	/* Zero output first so any early return renders blank space instead of noise */
	memset(bytes, 0, 32);
	if(pEdit->spell.phz==NULL)
	{
		return;
	}

	/* Cast to unsigned char to prevent sign-extension of bytes 0x80..0xFF */
	b0=(unsigned char)incode[0];
	b1=(unsigned char)incode[1];
	if(b0<0xa1 || b1<0xa1)
	{
		return;
	}

	/* Use 32-bit unsigned long constants so 16-bit Turbo C int never overflows */
	offset=(94UL*(unsigned long)(b0-0xa1)+(unsigned long)(b1-0xa1))*32UL;
	if(fseek(pEdit->spell.phz, (long)offset, SEEK_SET)==0)
	{
		if(fread(bytes, 32, 1, pEdit->spell.phz)!=1)
		{
			memset(bytes, 0, 32);
		}
	}
}

/*****************************************************************************
 * Function: Displayhz
 * Purpose:  Draw one 16x16 GB2312 Chinese character at top-left (x0, y0).
 *
 * @param x0    Left pixel coordinate of the 16x16 glyph cell.
 * @param y0    Top pixel coordinate of the 16x16 glyph cell.
 * @param code  Pointer to the 2-byte GB2312 character code.
 * @param color BGI palette color index (e.g., BLACK, WHITE, RED, YELLOW).
 * @param pEdit Editor context (lazily opens HZK16 if not yet open).
 * @complexity  O(1) per glyph (at most 256 pixel tests, skipping zero bytes).
 *****************************************************************************/
void Displayhz(int x0, int y0, char code[], int color, struct EDITTYPE *pEdit)
{
	int i, y, pos;
	char mat[32];
	unsigned char byteVal, bitMask;

	/* Ensure persistent font handle is ready before seeking */
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
		pos=i<<1; /* 2 bytes per 16-pixel scanline */

		/* Left half of scanline i (columns x0 .. x0+7, MSB first) */
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

		/* Right half of scanline i (columns x0+8 .. x0+15, MSB first) */
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

/*****************************************************************************
 * Function: Printfhz
 * Purpose:  Render a NUL-terminated GB2312 Chinese string horizontally.
 *
 * @param x     Starting left pixel X coordinate.
 * @param y     Top pixel Y coordinate for all glyphs in the string.
 * @param s     NUL-terminated string of 2-byte GB2312 character pairs.
 * @param color BGI palette color index.
 * @param pEdit Editor context owning the persistent HZK16 handle.
 * @note        Clips safely at `X_MAX` and checks both `s[0]` and `s[1]`
 *              against '\0' so an odd-length string never reads past NUL.
 *****************************************************************************/
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
