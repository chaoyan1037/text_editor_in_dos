#ifndef _STRUCT_H_
#define _STRUCT_H_

#include<stdio.h>

struct MOUSESTATE
{
    int x;
    int y;
    int mouseFlag;
    int button;
    int function;
    int functionActive;
    long int key;
    int ascii;
};

struct FILETYPE
{
    int fileflag;    /* 文件名变更时置1 */
    char fname[16];  /* 当前文件名缓冲区 */
    int modifyFlag;
};

struct NODE
{
    int flag;        /* 字符默认是1,汉字是2，回车是0 */
    int firstInLine; /* 标识该节点是否为该行第一个节点 */
    char data[3];
    struct NODE *last;
    struct NODE *next;
};

struct SPELLTYPE
{
    char name[16];   /* 输入的文件名缓冲区（最多12字符+结束符） */
    char pcode[3];
    FILE *phz;
};

struct MENUTYPE
{
    int menuflag;    /* 出现下拉条是1，否则是0 */
    int flag;        /* 当前光标的位置 */
    int SubmenuNum[4];
};

struct EDITTYPE
{
    struct NODE *start;
    struct NODE *current;
    struct NODE *last;
    struct NODE *clipStart;
    struct NODE *clipLast;
    struct NODE *findStart;
    struct NODE *findLast;
    struct NODE *pageFirst;

    int lineCount;
    int language;
    int x;
    int y;
    int line;
    int row;

    int tempLineCount;
    int tempCurrentCount;

    struct NODE *lastPosCurrent;
    struct NODE *lastPosPageFirst;
    int lastPosX;
    int lastPosY;
    struct NODE *lastRollPageFirst;

    struct MOUSESTATE state;
    int cursorFlag;
    int editFlag;
    int editActive;
    int copyActive;
    int copyCutFlag;
    int clipFlag;
    int pageDownFlag;
    char findGoal[14];
    char replaceGoal[14];

    struct SPELLTYPE spell;
    struct MENUTYPE submenu;
    struct FILETYPE file;
};

#endif
