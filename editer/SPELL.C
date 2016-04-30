/**********************************************
function: edit chinese

programmer:XiaoYao

time：2011.9.25
***********************************************/
#include<graphics.h>
#include<string.h>
#include<bios.h>
#include"h//hanzi.h"
#include"h//common.h"
#include"h//struct.h"
#include"h//malloc.h"
#include"h//node.h"
#include"h//language.h"
#include"h//spell.h"

void Shurukuang(int x1, int y1, int x2, int y2, char *pinYin);
void Hanzikuang(int x1, int y1, int x2, int y2);
void InputPinYin(char *pinYin, struct EDITTYPE *pEdit);
void  OutputHanZi(int x, int y, char *pinYin, struct EDITTYPE *pEdit);
int  GetCode(char *tcode, char *pin);
void Gethanzi(struct EDITTYPE *pEdit);


void Spell(struct EDITTYPE *pEdit)
{
	char pinYin[11];
	InputPinYin(pinYin, pEdit);
	OutputHanZi(217, 457, pinYin, pEdit);	
}

void InputPinYin(char *pinYin, struct EDITTYPE *pEdit)
{
	long int key;
	int i=0;
	char letter;
    	pinYin[i]='\0';
	while(1)
	{
		if( pEdit->state.ascii>='a'&&pEdit->state.ascii<='z' )
		{
			pinYin[0]=pEdit->state.ascii;
			pinYin[1]='\0';
			pEdit->state.ascii='a'-1;
			Shurukuang(100,455,175,475,pinYin);
			i=1;
		}
            
		if(bioskey(1)!=0)
		{
			key=bioskey(0);
			letter=key&0x00ff;  
			if(i>=10)
			{
				break;
			}
			if(key==SPACE)		//结束拼音输入
			{
				pinYin[i]='\0';
				Shurukuang(100,455,175,475,pinYin);
				break;
			}
			else if(key==BACKSPACE)
			{
				if(i!=0)
				{
					pinYin[i-1]='\0';
					Shurukuang(100,455,175,475,pinYin);
					i--;
				}
			}
			else if( (letter>='a'&&letter<='z')||(letter>='0'&&letter<='9') )
			{
				pinYin[i] = letter;
				pinYin[i+1]='\0';
				i++;
                                Shurukuang(100,455,175,475,pinYin);
			}
		}

	}
}
void  OutputHanZi(int x, int y, char *pinYin, struct EDITTYPE *pEdit)
{	
	char code[300], *s=code;
   	long int key;
	int flag=0,flag2=0,letter;	//flag以区别特殊汉字“垚”，flag2以区别汉字选择页数
	int xx=x, i=0, length=0;
	for(i=0; i<300; i++)
	{
		code[i]='\0';
	}

	if( (pinYin[0]=='v')&&(pinYin[1]=='\0') )
	{
		pEdit->spell.pcode[0]=' ';
		pEdit->spell.pcode[1]=' ';
		flag=1;
		Gethanzi(pEdit);
		setfillstyle(SOLID_FILL, LIGHTGRAY);
		bar(100, 451, 400, 479);
	}

	if(flag==0)
	{
		if(GetCode(code, pinYin)==1)
		{
			Hanzikuang(200,455,400,475);
		}
		else
		{	
			Shurukuang(100,455,175,475, "\0");
			return;
		}
		open_hz(pEdit);
		for(i=0; code[i]!='\0'; i++)	
		length=i;
		i=0;

		while( (xx< X_MAX) && (*s!=NULL) && (i<4) )
		{
			Displayhz(xx, y, s, BLACK, pEdit);
			xx+=16*3;
			s+=2;
			i++;
		}

		pEdit->spell.pcode[0]=s[0];
		pEdit->spell.pcode[1]=s[1];

		while(1)
		{
			if(bioskey(1)!=0)
			{
				i=0;
				xx=217;
				key=bioskey(0);
				letter=key&0x00ff;

				if( (key==DENGHAO)&&(*s!=0)&&(length>8) )
				{
					Hanzikuang(200,455,400,475);
					while( (xx<X_MAX)&&(*s!=0)&&(i<4) )
					{
						Displayhz(xx,457,s,BLACK,pEdit);
						xx+=16*3;
						s+=2;
						i++;
											
					}
					while(i<4)
					{
						s+=2;
						*s=0;
						i++;
					}	
					flag2++;
				}
				else if( (key==JIANHAO)&&(s-8!=code)&&(length>8) )
				{
					Hanzikuang(200,455,400,475);
					s-=16;
					while( (xx<X_MAX)&&(s!=NULL)&&(i<4) )
		        		{
			       			Displayhz(xx,457,s,BLACK,pEdit);
						xx+=16*3;
						s+=2;
						i++;	
					}
					flag2--;
				}
				else if( (key==SPACE)||(letter=='1') )
				{
					if(length>=8)
					{
						s-=8;
					}
					else
					{	
						s=code;
					}
					if(*s=='\0')
					{
						s=NULL;
					}				
					pEdit->spell.pcode[0]=s[0];
					pEdit->spell.pcode[1]=s[1];
					s+=8;
					Gethanzi(pEdit);
					setfillstyle(SOLID_FILL, LIGHTGRAY);
					bar(100, 451, 400, 479);
					break;
				}
				else if(letter=='2')
				{
					if(length>=8)
					{
						s-=6;
					}
					else
					{	
						s=code+2;
					}
					if(*s=='\0')
					{
						s=NULL;
					}				
					pEdit->spell.pcode[0]=s[0];
					pEdit->spell.pcode[1]=s[1];
					s+=6;
					Gethanzi(pEdit);
					setfillstyle(SOLID_FILL, LIGHTGRAY);
					bar(100, 451, 400, 479);
					break;
				}
				else if(letter=='3')
				{
					if(length>=8)
					{
						s-=4;
					}
					else
					{	
						s=code+4;
					}
					if(*s=='\0')
					{
						s=NULL;
					}
					pEdit->spell.pcode[0]=s[0];
					pEdit->spell.pcode[1]=s[1];
					s+=4;
					Gethanzi(pEdit);
					setfillstyle(SOLID_FILL, LIGHTGRAY);
					bar(100, 451, 400, 479);
					break;
				}
				else if(letter=='4')
				{	
					if(flag==2)
					{
						pEdit->spell.pcode[0]=' ';
						pEdit->spell.pcode[1]=' ';
						Gethanzi(pEdit);
						setfillstyle(SOLID_FILL, LIGHTGRAY);
						bar(100, 451, 400, 479);
						break;
					}
					if(length>=8)
					{
						s-=2;
					}
					else
					{
						s=code+6;
					}		
					if(*s=='\0')
					{
						s=NULL;
					}	
					pEdit->spell.pcode[0]=s[0];
					pEdit->spell.pcode[1]=s[1];
					s+=2;
					Gethanzi(pEdit);
					setfillstyle(SOLID_FILL, LIGHTGRAY);
					bar(100, 451, 400, 479);
					break;
				}
				
				if( (pinYin[0]=='y')&&(pinYin[1]=='a')&&(pinYin[2]=='o')&&(pinYin[3]=='\0')&&(flag2==8) )
				{	
					OutputYao(362 , 457, BLACK);
					flag=2;
				}
			}
		}	
	
		fclose(pEdit->spell.phz);
	}
	
	
}

int  GetCode(char *tcode, char *pin)
{
	long int i=0;
	char routine[40]="C://TC30//BIN//spell//";
	char *pinyin=pin;
	char *suffix=".txt";
	FILE *fp;
	strcat(routine, pinyin);
	strcat(routine, suffix);
	if(  ( fp=fopen(routine, "rt") )==NULL )
	{
		tcode[0] = 0;
		outtextxy(240, 470, "error,input again.");
		return 0;
	}

	while( (tcode[i] = fgetc(fp)) != EOF )
	{
		i++;
	}
	tcode[i]=0;
	fclose(fp);
	return 1;
}


void Shurukuang(int x1, int y1, int x2, int y2, char *pinYin)
{
    setfillstyle(SOLID_FILL, LIGHTGREEN);    
    bar(x1, y1, x2, y2);
    outtextxy(x1+2, y2-2, pinYin);
    setcolor(WHITE);
    rectangle(x1, y1, x2, y2);
    WhiteBlack(x1, y1, x2, y2, BLACK);
}

void Hanzikuang(int x1, int y1, int x2, int y2)
{
    setfillstyle(SOLID_FILL, LIGHTGREEN);
    bar(x1, y1, x2, y2);
    outtextxy(x1+2, y2-2, "1.    2.    3.    4.");
    setcolor(WHITE);
    rectangle(x1, y1, x2, y2);
    WhiteBlack(x1, y1, x2, y2, BLACK);
}

void Gethanzi(struct EDITTYPE *pEdit)
{
    char data[3]="hh";
    pEdit->editFlag=1;
    pEdit->cursorFlag=1;
    data[0]=pEdit->spell.pcode[0];
    data[1]=pEdit->spell.pcode[1];

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
    else if( (pEdit->start!=NULL)&&(pEdit->current->next==NULL) )
    {
        AddNode( pEdit, data);
		pEdit->current->flag=2;
    }
    else
    {
		InsertNode( pEdit, data);
		pEdit->current->flag=2;
    }

    if( (data[0]==' ')&&(data[1]==' ') )
    {
		OutputYao(200,200,BLACK);
    }
}

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
		Printfhz(10, 458,"英", RED, pEdit);
		setfillstyle(SOLID_FILL, LIGHTGRAY);
		bar(100, 451, 400, 479);
    }
    else
    {
		Printfhz(10, 458,"汉", RED, pEdit);
    }
}

void OutputYao(int x0, int y0, int color)
{
	int x, y, i, j, pos;
	unsigned char mask[]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
	char mat[32]={0x01,0x00,0x01,0x00,0x3F,0xF8,0x01,0x00,0x01,0x00,0x01,0x00,0xFF,0xFE,0x10,0x10,0x10,0x10,0x10,0x10,0x7C,0xFC,0x10,0x10,0x10,0x10,0x1E,0x10,0xF0,0xFE,0x40,0x00};
	y=y0;
	for(i=0; i<16; i++)
	{
		x=x0;
		pos=2*i;
		for(j=0; j<16; j++)
		{
			if((mask[j%8]&mat[pos+j/8])!=0)
			{
				putpixel(x, y, color);
			}
			++x;
		}
		++y;
	}
}


