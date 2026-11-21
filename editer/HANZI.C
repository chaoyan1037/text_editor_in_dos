/**********************************************
function£ºoutput chinese

programmer:Xiao

time£º2011.9.25
***********************************************/

#include<graphics.h>
#include<string.h>
#include<stdio.h>
#include<conio.h>
#include"h/common.h"
#include"h/struct.h"
#include"h/hanzi.h"

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

void close_hz(struct EDITTYPE *pEdit)
{
	if(pEdit->spell.phz!=NULL)
	{
		fclose(pEdit->spell.phz);
		pEdit->spell.phz=NULL;
	}
}

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
