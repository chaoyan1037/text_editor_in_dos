/*****************************************************************************
 * File: platform/bgi_tui.c
 * Purpose: Zero-dependency Linux/POSIX terminal backend implementing the
 *          Borland BGI graphics and DOS keyboard/mouse APIs over ANSI/VT100.
 *
 * Terminal Grid Mapping (640x480 VGA -> 80 cols x 28 rows):
 *   - Row  0     (Y =   0.. 25): Blue title bar (filename, title, Exit button)
 *   - Row  1     (Y =  26.. 49): Top menu bar (File, Edit, Tools, Help)
 *   - Rows 2..26 (Y =  50..450): 25-row text viewport (cols 0..76) +
 *                                right-hand scrollbar (cols 77..79)
 *   - Row 27     (Y = 451..479): EN/ZH toggle, Pinyin box, Candidate bar,
 *                                and Line/Col status readout
 *
 * Features:
 *   - Raw termios mode with alternate screen buffer and signal-safe cleanup.
 *   - Double-buffered diff renderer (only changed cells are emitted).
 *   - GB2312/GB18030 -> UTF-8 conversion via POSIX iconv() for crisp terminal
 *     Chinese rendering (including double-width CJK cell hygiene).
 *   - Full ANSI escape decoder for Arrows, Ctrl+Arrows, Shift+Arrows, F1..F8,
 *     Ctrl+A..Z, Enter, BackSpace, Esc, and xterm SGR mouse clicks.
 *****************************************************************************/
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <time.h>
#include <iconv.h>

#include "graphics.h"
#include "bios.h"
#include "conio.h"
#include "dos.h"
#include "h/common.h"

#define TUI_COLS 80
#define TUI_ROWS 28

typedef struct {
    char utf8[5];          /* NUL-terminated UTF-8 glyph (1..4 bytes) */
    unsigned char fg;      /* BGI foreground color index (0..15) */
    unsigned char bg;      /* BGI background color index (0..15) */
    unsigned char wide;    /* 0=normal 1-col, 1=left half of CJK, 2=right half */
} TuiCell;

static TuiCell g_back[TUI_ROWS][TUI_COLS];
static TuiCell g_front[TUI_ROWS][TUI_COLS];
static int g_dirty = 1;
static int g_full_redraw = 1;
static int g_active = 0;

/* Current BGI graphics context state */
static int g_fg_color = WHITE;
static int g_fill_color = BLACK;
static int g_write_mode = COPY_PUT;
static int g_text_vert = TOP_TEXT;
static int g_cur_x = 0;
static int g_cur_y = 0;

/* Native terminal cursor location (0-based row/col) */
static int g_cursor_col = 0;
static int g_cursor_row = 2;
static int g_cursor_visible = 1;

/* Terminal state & iconv handle */
static struct termios g_orig_termios;
static int g_termios_saved = 0;
static iconv_t g_iconv_cd = (iconv_t)-1;

/* Keyboard queue (ring buffer of 16-bit BIOS key codes) */
#define KEYQ_SIZE 64
static int g_keyq[KEYQ_SIZE];
static int g_keyq_head = 0;
static int g_keyq_tail = 0;

/* Virtual DOS mouse state (updated by xterm SGR mouse packets) */
static int g_mouse_x = 320;
static int g_mouse_y = 240;
static int g_mouse_btn = 0;
static int g_mouse_pressed_unseen = 0;

/* Map 16 BGI colors to ANSI SGR foreground/background codes */
static int bgi_to_ansi_fg(int c)
{
    static const int map[16] = {
        30, 34, 32, 36, 31, 35, 33, 37,
        90, 94, 92, 96, 91, 95, 93, 97
    };
    return map[c & 15];
}

static int bgi_to_ansi_bg(int c)
{
    static const int map[16] = {
        40, 44, 42, 46, 41, 45, 43, 47,
        100, 104, 102, 106, 101, 105, 103, 107
    };
    return map[c & 15];
}

/* Convert 640px X coordinate -> 0..79 terminal column */
static int px_to_col(int x)
{
    int col;
    if (x < 0)
    {
        return 0;
    }
    col = x / CHARSIZE;
    if (col >= TUI_COLS) col = TUI_COLS - 1;
    return col;
}

/* Convert 480px Y coordinate -> 0..27 terminal row */
static int px_to_row(int y, int is_bottom_aligned)
{
    int row;
    if (y <= 25)
    {
        return 0;
    }
    if (y <= 49)
    {
        return 1;
    }
    if (y >= 451)
    {
        return TUI_ROWS - 1;
    }
    if (is_bottom_aligned)
    {
        /* BGI BOTTOM_TEXT y is baseline of an 8px glyph; y-8 aligns with its cell */
        row = 2 + (y - 8 - EDITERSCREENY1) / CHARHIGH;
    }
    else
    {
        row = 2 + (y - EDITERSCREENY1) / CHARHIGH;
    }
    if (row < 2) row = 2;
    if (row > 26) row = 26;
    return row;
}

/* Reverse-map 1-based terminal mouse (tcol, trow) -> 640x480 pixel coords */
static void term_to_px(int tcol, int trow, int *px, int *py)
{
    int c = tcol - 1;
    int r = trow - 1;
    if (c < 0) c = 0;
    if (c >= TUI_COLS) c = TUI_COLS - 1;
    if (r < 0) r = 0;
    if (r >= TUI_ROWS) r = TUI_ROWS - 1;

    if (r >= 2 && r <= 26 && c < 76)
    {
        /* Viewport text column c occupies [EDITERSCREENX1 + c*8 .. +7] */
        *px = EDITERSCREENX1 + c * CHARSIZE + 2;
    }
    else if (c == 0)
    {
        *px = EDITERSCREENX1 + 2;
    }
    else
    {
        *px = c * CHARSIZE + 4;
    }

    if (r == 0)
    {
        *py = 12;
    }
    else if (r == 1)
    {
        *py = 35;
    }
    else if (r >= TUI_ROWS - 1)
    {
        *py = 465;
    }
    else
    {
        *py = EDITERSCREENY1 + (r - 2) * CHARHIGH + 8;
    }
}

/* Clear any orphaned half when overwriting part of a double-width CJK glyph */
static void clear_wide_conflict(int r, int c)
{
    if (r < 0 || r >= TUI_ROWS || c < 0 || c >= TUI_COLS)
    {
        return;
    }
    if (g_back[r][c].wide == 1 && c + 1 < TUI_COLS)
    {
        g_back[r][c + 1].utf8[0] = ' ';
        g_back[r][c + 1].utf8[1] = '\0';
        g_back[r][c + 1].wide = 0;
    }
    else if (g_back[r][c].wide == 2 && c > 0)
    {
        g_back[r][c - 1].utf8[0] = ' ';
        g_back[r][c - 1].utf8[1] = '\0';
        g_back[r][c - 1].wide = 0;
    }
}

static void put_ascii_cell(int r, int c, char ch, int fg, int bg, int keep_bg)
{
    if (r < 0 || r >= TUI_ROWS || c < 0 || c >= TUI_COLS)
    {
        return;
    }
    clear_wide_conflict(r, c);
    g_back[r][c].utf8[0] = ch;
    g_back[r][c].utf8[1] = '\0';
    g_back[r][c].fg = (unsigned char)(fg & 15);
    if (!keep_bg)
    {
        g_back[r][c].bg = (unsigned char)(bg & 15);
    }
    g_back[r][c].wide = 0;
    g_dirty = 1;
}

static void put_wide_cell(int r, int c, const char *utf8, int fg)
{
    if (r < 0 || r >= TUI_ROWS || c < 0 || c + 1 >= TUI_COLS)
    {
        return;
    }
    clear_wide_conflict(r, c);
    clear_wide_conflict(r, c + 1);
    strncpy(g_back[r][c].utf8, utf8, 4);
    g_back[r][c].utf8[4] = '\0';
    g_back[r][c].fg = (unsigned char)(fg & 15);
    g_back[r][c].wide = 1;

    g_back[r][c + 1].utf8[0] = '\0';
    g_back[r][c + 1].fg = (unsigned char)(fg & 15);
    g_back[r][c + 1].bg = g_back[r][c].bg;
    g_back[r][c + 1].wide = 2;
    g_dirty = 1;
}

static void keyq_push(int code)
{
    int next = (g_keyq_tail + 1) % KEYQ_SIZE;
    if (next != g_keyq_head)
    {
        g_keyq[g_keyq_tail] = code;
        g_keyq_tail = next;
    }
}

static int keyq_empty(void)
{
    return g_keyq_head == g_keyq_tail;
}

static int keyq_pop(void)
{
    int code = g_keyq[g_keyq_head];
    g_keyq_head = (g_keyq_head + 1) % KEYQ_SIZE;
    return code;
}

static void restore_terminal(void);

/* Read one byte from stdin with timeout in milliseconds (-1 on timeout).
 * Exits cleanly on EOF or fatal read/select error so a closed terminal or
 * redirected /dev/null stdin can never trigger a 100% CPU busy loop. */
static int read_byte_timeout(int timeout_ms)
{
    fd_set rfds;
    struct timeval tv;
    unsigned char ch;
    int r;

    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    r = select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv);
    if (r < 0)
    {
        if (errno == EINTR)
        {
            return -1;
        }
        restore_terminal();
        exit(0);
    }
    if (r > 0)
    {
        ssize_t n = read(STDIN_FILENO, &ch, 1);
        if (n == 1)
        {
            return (int)ch;
        }
        if (n == 0 || (n < 0 && errno != EINTR && errno != EAGAIN))
        {
            /* EOF or broken terminal: shut down cleanly instead of spinning */
            restore_terminal();
            exit(0);
        }
    }
    return -1;
}

/* Translate a single control or ASCII byte to its 16-bit BIOS key code */
static int byte_to_bioskey(int b)
{
    switch (b)
    {
        case 0x01: return CTRL_A;
        case 0x03: return CTRL_C;
        case 0x04: return CTRL_D;
        case 0x05: return CTRL_E;
        case 0x06: return CTRL_F;
        case 0x08:
        case 0x7f: return BACKSPACE;
        case 0x0a:
        case 0x0d: return ENTER;
        case 0x0e: return CTRL_N;
        case 0x0f: return CTRL_O;
        case 0x11: return CTRL_Q;
        case 0x12: return CTRL_R;
        case 0x13: return CTRL_S;
        case 0x16: return CTRL_V;
        case 0x18: return CTRL_X;
        case ' ':  return SPACE;
        case '-':  return JIANHAO;
        case '=':  return DENGHAO;
        default:
            if (b >= 0x20 && b <= 0x7e)
            {
                return b;
            }
            return 0;
    }
}

/* Parse ANSI/VT100 escape sequences (arrows, F-keys, Ctrl+Arrows, SGR mouse) */
static void parse_escape_seq(void)
{
    int b1 = read_byte_timeout(25);
    if (b1 < 0)
    {
        /* Standalone ESC keypress */
        keyq_push(ESC);
        return;
    }

    if (b1 == 'O')
    {
        /* SS3 sequences: F1..F4 (\033OP..S) */
        int b2 = read_byte_timeout(25);
        if (b2 == 'P') keyq_push(F1);
        else if (b2 == 'Q') keyq_push(F2);
        else if (b2 == 'R') keyq_push(F3);
        else if (b2 == 'S') keyq_push(F4);
        else if (b2 == 'A') keyq_push(UP);
        else if (b2 == 'B') keyq_push(DOWN);
        else if (b2 == 'C') keyq_push(RIGHT);
        else if (b2 == 'D') keyq_push(LEFT);
        return;
    }

    if (b1 != '[')
    {
        return;
    }

    {
        char seq[32];
        int len = 0;
        int ch;
        while (len < (int)sizeof(seq) - 1)
        {
            ch = read_byte_timeout(25);
            if (ch < 0) break;
            seq[len++] = (char)ch;
            if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '~')
            {
                break;
            }
        }
        seq[len] = '\0';

        /* xterm SGR mouse event: "<btn;col;rowM" (press) or "...m" (release) */
        if (seq[0] == '<')
        {
            int btn = 0, col = 1, row = 1;
            char kind = seq[len > 0 ? len - 1 : 0];
            if (sscanf(seq + 1, "%d;%d;%d", &btn, &col, &row) == 3)
            {
                if (kind == 'M' && (btn & 3) == 0 && (btn & 64) == 0)
                {
                    term_to_px(col, row, &g_mouse_x, &g_mouse_y);
                    g_mouse_btn = 1;
                    g_mouse_pressed_unseen = 1;
                }
                else if (kind == 'm')
                {
                    if (!g_mouse_pressed_unseen)
                    {
                        term_to_px(col, row, &g_mouse_x, &g_mouse_y);
                    }
                    g_mouse_btn = 0;
                }
                else if (kind == 'M' && btn == 64)
                {
                    keyq_push(UP);   /* Scroll wheel up */
                }
                else if (kind == 'M' && btn == 65)
                {
                    keyq_push(DOWN); /* Scroll wheel down */
                }
                else if (!g_mouse_pressed_unseen)
                {
                    term_to_px(col, row, &g_mouse_x, &g_mouse_y);
                }
            }
            return;
        }

        if (strcmp(seq, "A") == 0) keyq_push(UP);
        else if (strcmp(seq, "B") == 0) keyq_push(DOWN);
        else if (strcmp(seq, "C") == 0) keyq_push(RIGHT);
        else if (strcmp(seq, "D") == 0) keyq_push(LEFT);
        else if (strcmp(seq, "1;5C") == 0 || strcmp(seq, "1;2C") == 0) keyq_push(CTRL_RIGHT);
        else if (strcmp(seq, "1;5D") == 0 || strcmp(seq, "1;2D") == 0) keyq_push(CTRL_LEFT);
        else if (strcmp(seq, "11~") == 0) keyq_push(F1);
        else if (strcmp(seq, "12~") == 0) keyq_push(F2);
        else if (strcmp(seq, "13~") == 0) keyq_push(F3);
        else if (strcmp(seq, "14~") == 0) keyq_push(F4);
        else if (strcmp(seq, "15~") == 0) keyq_push(F5);
        else if (strcmp(seq, "17~") == 0) keyq_push(F6);
        else if (strcmp(seq, "18~") == 0) keyq_push(F7);
        else if (strcmp(seq, "19~") == 0) keyq_push(F8);
        else if (strcmp(seq, "5~") == 0)  keyq_push(PAGEUP);
        else if (strcmp(seq, "6~") == 0)  keyq_push(PAGEDOWN);
    }
}

/* Poll stdin with optional wait (ms) and enqueue decoded keys/mouse events */
static void poll_stdin(int timeout_ms)
{
    int b = read_byte_timeout(timeout_ms);
    while (b >= 0)
    {
        if (b == 0x1b)
        {
            parse_escape_seq();
        }
        else
        {
            int code = byte_to_bioskey(b);
            if (code != 0)
            {
                keyq_push(code);
            }
        }
        /* Drain any additional buffered bytes immediately */
        b = read_byte_timeout(0);
    }
}

static void restore_terminal(void)
{
    if (g_active)
    {
        /* Disable SGR mouse, reset colors, show cursor, leave alt screen */
        fputs("\033[?1006l\033[?1000l\033[0m\033[?25h\033[?1049l", stdout);
        fflush(stdout);
        g_active = 0;
    }
    if (g_termios_saved)
    {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
        g_termios_saved = 0;
    }
    if (g_iconv_cd != (iconv_t)-1)
    {
        iconv_close(g_iconv_cd);
        g_iconv_cd = (iconv_t)-1;
    }
}

/* Async-signal-safe terminal restore for SIGTERM / SIGHUP / SIGINT */
static void handle_fatal_signal(int sig)
{
    static const char reset_seq[] = "\033[?1006l\033[?1000l\033[0m\033[?25h\033[?1049l";
    if (g_active)
    {
        ssize_t ignored = write(STDOUT_FILENO, reset_seq, sizeof(reset_seq) - 1);
        (void)ignored;
        g_active = 0;
    }
    if (g_termios_saved)
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_orig_termios);
        g_termios_saved = 0;
    }
    _exit(128 + sig);
}

/* ==================== BGI Public API Implementation ==================== */

void initgraph(int *gd, int *gm, const char *path)
{
    struct termios raw;
    (void)gd;
    (void)gm;
    (void)path;

    if (isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &g_orig_termios) == 0)
    {
        g_termios_saved = 1;
        raw = g_orig_termios;
        /* Raw input so Ctrl+S/C/X/V/O/N/F/R/D/A reach the editor cleanly */
        raw.c_iflag &= (tcflag_t)~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
        raw.c_lflag &= (tcflag_t)~(ECHO | ICANON | IEXTEN | ISIG);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    }

    if (g_iconv_cd == (iconv_t)-1)
    {
        g_iconv_cd = iconv_open("UTF-8", "GB18030");
    }

    atexit(restore_terminal);
    signal(SIGTERM, handle_fatal_signal);
    signal(SIGHUP, handle_fatal_signal);
    signal(SIGINT, handle_fatal_signal);

    /* Enter alternate screen buffer and enable xterm SGR mouse tracking */
    fputs("\033[?1049h\033[2J\033[H\033[?1000h\033[?1006h", stdout);
    fflush(stdout);
    g_active = 1;
    g_full_redraw = 1;
    cleardevice();
}

void closegraph(void)
{
    restore_terminal();
}

void cleardevice(void)
{
    int r, c;
    for (r = 0; r < TUI_ROWS; r++)
    {
        for (c = 0; c < TUI_COLS; c++)
        {
            g_back[r][c].utf8[0] = ' ';
            g_back[r][c].utf8[1] = '\0';
            g_back[r][c].fg = WHITE;
            g_back[r][c].bg = BLACK;
            g_back[r][c].wide = 0;
        }
    }
    g_dirty = 1;
    g_full_redraw = 1;
}

void setwritemode(int mode) { g_write_mode = mode; }
void setfillstyle(int pattern, int color) { (void)pattern; g_fill_color = color & 15; }
void setcolor(int color) { g_fg_color = color & 15; }
void settextjustify(int horiz, int vert) { (void)horiz; g_text_vert = vert; }

void bar(int left, int top, int right, int bottom)
{
    int c1, c2, r1, r2, r, c;
    if (left > right) { int t = left; left = right; right = t; }
    if (top > bottom) { int t = top; top = bottom; bottom = t; }

    c1 = px_to_col(left);
    if (right >= EDITERSCREENX2)
    {
        c2 = TUI_COLS - 1;
    }
    else if (right == EDITERSCREENX2 - 20)
    {
        c2 = 75; /* Preserve scrollbar in cols 76..79 */
    }
    else if (right > left)
    {
        int cols = (right - left + CHARSIZE / 2) / CHARSIZE;
        if (cols < 1) cols = 1;
        c2 = c1 + cols - 1;
        if (c2 >= TUI_COLS) c2 = TUI_COLS - 1;
    }
    else
    {
        c2 = c1;
    }

    r1 = px_to_row(top, 0);
    r2 = (bottom > top) ? px_to_row(bottom - 1, 0) : r1;
    if (bottom >= Y_MAX) r2 = TUI_ROWS - 1;

    for (r = r1; r <= r2; r++)
    {
        for (c = c1; c <= c2; c++)
        {
            put_ascii_cell(r, c, ' ', g_fg_color, g_fill_color, 0);
        }
    }
}

void rectangle(int left, int top, int right, int bottom)
{
    (void)left; (void)top; (void)right; (void)bottom;
    /* Panel backgrounds from bar() already separate controls cleanly in TUI */
}

void line(int x1, int y1, int x2, int y2)
{
    (void)x1; (void)y1; (void)x2; (void)y2;
}

void moveto(int x, int y) { g_cur_x = x; g_cur_y = y; }
void lineto(int x, int y) { g_cur_x = x; g_cur_y = y; }

void floodfill(int x, int y, int border)
{
    /* Draw scrollbar up/down arrow glyphs cleanly in cols 78 row 2 & row 26 */
    (void)border;
    if (x >= EDITERSCREENX2 - 20)
    {
        if (y <= EDITERSCREENY1 + 16)
        {
            put_ascii_cell(2, 78, '^', BLACK, LIGHTBLUE, 0);
        }
        else if (y >= EDITERSCREENY2 - 16)
        {
            put_ascii_cell(26, 78, 'v', BLACK, LIGHTBLUE, 0);
        }
    }
}

void outtextxy(int x, int y, const char *text)
{
    int col, row, is_bottom;
    if (text == NULL)
    {
        return;
    }
    is_bottom = (g_text_vert == BOTTOM_TEXT && y > EDITERSCREENY1 && y <= EDITERSCREENY2);
    col = px_to_col(x);
    row = px_to_row(y, is_bottom);
    while (*text != '\0' && col < TUI_COLS)
    {
        put_ascii_cell(row, col, *text, g_fg_color, 0, 1);
        col++;
        text++;
    }
}

void putpixel(int x, int y, int color)
{
    (void)x; (void)y; (void)color;
}

/* ==================== Platform Extension Hooks ==================== */

int bgi_is_native_hanzi(void) { return 1; }
int bgi_is_native_mouse(void) { return 1; }

int bgi_draw_hanzi_native(int x0, int y0, const char code[2], int color)
{
    char inbuf[2];
    char outbuf[8];
    char *pin = inbuf;
    char *pout = outbuf;
    size_t inleft = 2;
    size_t outleft = sizeof(outbuf) - 1;
    int col = px_to_col(x0);
    /* Bias 16px Hanzi top coordinate toward vertical center (y0+7) inside viewport */
    int row = (y0 >= EDITERSCREENY1 && y0 < EDITERSCREENY2)
              ? px_to_row(y0 + 7, 0) : px_to_row(y0, 0);

    inbuf[0] = code[0];
    inbuf[1] = code[1];
    memset(outbuf, 0, sizeof(outbuf));

    if (g_iconv_cd != (iconv_t)-1
        && iconv(g_iconv_cd, &pin, &inleft, &pout, &outleft) != (size_t)-1
        && outbuf[0] != '\0')
    {
        put_wide_cell(row, col, outbuf, color);
    }
    else
    {
        put_ascii_cell(row, col, '?', color, 0, 1);
    }
    return 1;
}

int bgi_draw_yao_native(int x0, int y0, int color)
{
    int row = (y0 >= EDITERSCREENY1 && y0 < EDITERSCREENY2)
              ? px_to_row(y0 + 7, 0) : px_to_row(y0, 0);
    /* UTF-8 encoding of U+579A ('垚') = E5 9E 9A */
    put_wide_cell(row, px_to_col(x0), "\xe5\x9e\x9a", color);
    return 1;
}

int bgi_set_cursor(int x, int y, int visible)
{
    int col = px_to_col(x);
    int row = px_to_row(y, 1);
    if (col != g_cursor_col || row != g_cursor_row || visible != g_cursor_visible)
    {
        g_cursor_col = col;
        g_cursor_row = row;
        g_cursor_visible = visible;
        g_dirty = 1;
    }
    return 1;
}

void bgi_present(void)
{
    int r, c;
    int cur_fg = -1, cur_bg = -1;

    if (!g_active || (!g_dirty && !g_full_redraw))
    {
        return;
    }

    /* Hide cursor while flushing diffs to prevent visual flicker */
    fputs("\033[?25l", stdout);

    for (r = 0; r < TUI_ROWS; r++)
    {
        int need_move = 1;
        for (c = 0; c < TUI_COLS; c++)
        {
            TuiCell *bk = &g_back[r][c];
            TuiCell *fr = &g_front[r][c];

            if (!g_full_redraw
                && bk->fg == fr->fg
                && bk->bg == fr->bg
                && bk->wide == fr->wide
                && strcmp(bk->utf8, fr->utf8) == 0)
            {
                need_move = 1;
                continue;
            }

            /* Trailing half of a wide glyph is already painted by its lead cell */
            if (bk->wide == 2)
            {
                *fr = *bk;
                continue;
            }

            if (need_move)
            {
                fprintf(stdout, "\033[%d;%dH", r + 1, c + 1);
                need_move = 0;
            }
            if ((int)bk->fg != cur_fg || (int)bk->bg != cur_bg)
            {
                fprintf(stdout, "\033[%d;%dm",
                        bgi_to_ansi_fg(bk->fg),
                        bgi_to_ansi_bg(bk->bg));
                cur_fg = bk->fg;
                cur_bg = bk->bg;
            }
            fputs(bk->utf8, stdout);
            *fr = *bk;
        }
    }

    if (g_cursor_visible)
    {
        fprintf(stdout, "\033[0m\033[%d;%dH\033[?25h",
                g_cursor_row + 1, g_cursor_col + 1);
    }
    fflush(stdout);
    g_dirty = 0;
    g_full_redraw = 0;
}

/* ==================== DOS BIOS / CONIO / INT 33h Emulation ==================== */

int bioskey(int cmd)
{
    if (cmd == 1)
    {
        bgi_present();
        if (keyq_empty())
        {
            /* Sleep up to 250ms when idle; select() wakes immediately on any
             * keyboard or mouse byte so input latency stays < 1ms at ~0% CPU. */
            poll_stdin(250);
        }
        return !keyq_empty();
    }

    while (keyq_empty())
    {
        bgi_present();
        poll_stdin(250);
    }
    return keyq_pop();
}

int getch(void)
{
    return bioskey(0) & 0xff;
}

int int86(int intno, union REGS *inregs, union REGS *outregs)
{
    (void)intno;
    if (inregs->x.ax == 0)
    {
        /* Report mouse present */
        outregs->x.ax = 1;
        outregs->x.bx = 2;
    }
    else if (inregs->x.ax == 3)
    {
        /* Pump stdin without blocking so press/release events arrive promptly */
        poll_stdin(0);
        outregs->x.cx = (unsigned int)g_mouse_x;
        outregs->x.dx = (unsigned int)g_mouse_y;
        if (g_mouse_pressed_unseen)
        {
            /* Guarantee every quick click is delivered for at least one poll */
            outregs->x.bx = 1;
            g_mouse_pressed_unseen = 0;
        }
        else
        {
            outregs->x.bx = (unsigned int)g_mouse_btn;
        }
    }
    return 0;
}

void delay(unsigned int ms)
{
    struct timespec ts;
    bgi_present();
    ts.tv_sec = (time_t)(ms / 1000);
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}
