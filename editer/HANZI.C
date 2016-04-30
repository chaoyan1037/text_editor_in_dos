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
	pEdit->spell.phz=fopen("C://TC30//BIN//hanziku//HZK16", "rb");
	if(pEdit->spell.phz==NULL)
	{
		outtextxy(EDITERSCREENX1, EDITERSCREENY2+5, "Can not open hzk16.\n");
		getch();
	}

}

void Get_hz(char incode[], char bytes[], struct EDITTYPE *pEdit)
{
	unsigned char qh,wh;
	unsigned long offset;
	qh=incode[0]-0xa0;
	wh=incode[1]-0xa0;
	offset=(94*(qh-1)+(wh-1))*32l;
	fseek(pEdit->spell.phz, offset, SEEK_SET);
	fread(bytes, 32, 1, pEdit->spell.phz);
}

void Displayhz(int x0, int y0, char code[], int color, struct EDITTYPE *pEdit)
{
	unsigned char mask[]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
	int i, j, x, y, pos;
	char mat[32];
	Get_hz(code, mat, pEdit);
	y=y0;
	for(i=0; i<16; i++)
	{
		x=x0;
		pos=2*i;
		for(j=0; j<16; j++)
		{
			if((mask[j%8]&mat[pos+j/8])!=NULL)
			{
				putpixel(x, y, color);
			}
			++x;
		}
		++y;
	}
}

void Printfhz(int x, int y, char *s, int color, struct EDITTYPE *pEdit)
{
	int x1=x,y1=y;
	open_hz(pEdit);
	while(*s!=NULL)
	{
		while( (x1<X_MAX) && (*s!=NULL) )
		{
			Displayhz(x1,y1,s,color, pEdit);
			x1=x1+16;
			s=s+2;
		}
	}
	fclose(pEdit->spell.phz);
}




