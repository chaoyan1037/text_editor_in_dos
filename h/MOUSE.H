#ifndef _MOUSE_H_
#define _MOUSE_H_

#define MOUSEID 0X33

struct EDITTYPE;

int InitMouse(void);
void ReadMouse(struct EDITTYPE *pEdit);
void MouseKey(struct EDITTYPE *pEdit);
void ShowMouse(struct EDITTYPE *pEdit);
void WhiteBlack(int x0, int y0, int x1, int y1, int color);
void button3d(int x0, int y0, int x1, int y1);

#endif
