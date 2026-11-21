/*****************************************************************************
 * File: HANZI.C
 * Purpose: GB2312 Chinese character and string rendering wrappers.
 *
 * Notes:
 *   - Under Linux/POSIX, Chinese glyphs are converted from GB2312 to UTF-8
 *     via iconv() inside platform/bgi_tui.c and drawn natively by the
 *     terminal, so the legacy DOS HZK16 dot-matrix font file is no longer
 *     needed.
 *****************************************************************************/

#include<graphics.h>
#include<stddef.h>
#include"h/common.h"
#include"h/struct.h"
#include"h/hanzi.h"

/*****************************************************************************
 * Function: Displayhz
 * Purpose:  Render one 2-byte GB2312 Chinese character at (x0, y0).
 *
 * @param x0    Left pixel coordinate of the 16x16 glyph cell.
 * @param y0    Top pixel coordinate of the 16x16 glyph cell.
 * @param code  Pointer to the 2-byte GB2312 character code.
 * @param color BGI palette color index (e.g., BLACK, WHITE, RED, YELLOW).
 * @param pEdit Master editor context (kept for API symmetry).
 *****************************************************************************/
void Displayhz(int x0, int y0, char code[], int color, struct EDITTYPE *pEdit)
{
	(void)pEdit;
	bgi_draw_hanzi_native(x0, y0, code, color);
}

/*****************************************************************************
 * Function: Printfhz
 * Purpose:  Render a NUL-terminated GB2312 Chinese string horizontally.
 *
 * @param x     Starting left pixel X coordinate.
 * @param y     Top pixel Y coordinate for all glyphs in the string.
 * @param s     NUL-terminated string of 2-byte GB2312 character pairs.
 * @param color BGI palette color index.
 * @param pEdit Master editor context.
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
	while( (x1<X_MAX) && (s[0]!='\0') && (s[1]!='\0') )
	{
		Displayhz(x1, y1, s, color, pEdit);
		x1=x1+16;
		s=s+2;
	}
}
