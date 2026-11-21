#ifndef _HANZI_H_
#define _HANZI_H_

struct EDITTYPE;

void open_hz(struct EDITTYPE *pEdit);
void close_hz(struct EDITTYPE *pEdit);
void Get_hz(char incode[], char bytes[], struct EDITTYPE *pEdit);
void Displayhz(int x0, int y0, char code[], int color, struct EDITTYPE *pEdit);
void Printfhz(int x, int y, char *s, int color, struct EDITTYPE *pEdit);

#endif
