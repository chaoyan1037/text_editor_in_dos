/*****************************************************************************
 * File: platform/graphics.h
 * Purpose: Drop-in Linux/POSIX compatibility header for Borland BGI
 *          <graphics.h>, supporting both the zero-dependency POSIX terminal
 *          backend (bgi_tui.c) and the pixel-accurate SDL2 backend (bgi_sdl2.c).
 *****************************************************************************/
#ifndef _PLATFORM_GRAPHICS_H_
#define _PLATFORM_GRAPHICS_H_

/* Graphics driver / mode constants (compatibility stubs for initgraph) */
#define VGA        9
#define VGAHI      2

/* Fill styles, pixel write modes, and text justification constants */
#define SOLID_FILL   1
#define COPY_PUT     0
#define XOR_PUT      1
#define LEFT_TEXT    0
#define TOP_TEXT     2
#define BOTTOM_TEXT  0

/* Standard 16-color EGA/VGA BGI palette indices */
#define BLACK         0
#define BLUE          1
#define GREEN         2
#define CYAN          3
#define RED           4
#define MAGENTA       5
#define BROWN         6
#define LIGHTGRAY     7
#define DARKGRAY      8
#define LIGHTBLUE     9
#define LIGHTGREEN   10
#define LIGHTCYAN    11
#define LIGHTRED     12
#define LIGHTMAGENTA 13
#define YELLOW       14
#define WHITE        15

/* Core Borland BGI primitives used by the editor */
void initgraph(int *gd, int *gm, const char *path);
void closegraph(void);
void cleardevice(void);
void setwritemode(int mode);
void setfillstyle(int pattern, int color);
void setcolor(int color);
void settextjustify(int horiz, int vert);
void bar(int left, int top, int right, int bottom);
void rectangle(int left, int top, int right, int bottom);
void line(int x1, int y1, int x2, int y2);
void moveto(int x, int y);
void lineto(int x, int y);
void floodfill(int x, int y, int border);
void outtextxy(int x, int y, const char *text);
void putpixel(int x, int y, int color);

/* Platform extension hooks (implemented by bgi_tui.c and bgi_sdl2.c):
 *   - bgi_present():           flush dirty screen state & throttle idle CPU
 *   - bgi_draw_hanzi_native(): render GB2312 glyph natively in TUI (0 in SDL2)
 *   - bgi_draw_yao_native():   render UTF-8 '垚' natively in TUI (0 in SDL2)
 *   - bgi_is_native_hanzi():   1 if HZK16 file is not needed for rendering
 *   - bgi_set_cursor():        position native cursor in TUI (0 in SDL2)
 *   - bgi_is_native_mouse():   1 if host provides cursor (skip XOR arrow) */
void bgi_present(void);
int  bgi_draw_hanzi_native(int x0, int y0, const char code[2], int color);
int  bgi_draw_yao_native(int x0, int y0, int color);
int  bgi_is_native_hanzi(void);
int  bgi_set_cursor(int x, int y, int visible);
int  bgi_is_native_mouse(void);

#endif
