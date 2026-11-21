/*****************************************************************************
 * File: tests/test_editor.c
 * Purpose: Standalone unit & regression test suite for the editor core,
 *          linked-list operations, clipboard, find/replace, HZK16 font,
 *          pinyin dictionary loader, and Save/Open round-tripping.
 *
 * Run with: `make test`
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include <graphics.h>
#include <conio.h>
#include <bios.h>
#include <dos.h>

#include "h/common.h"
#include "h/struct.h"
#include "h/malloc.h"
#include "h/node.h"
#include "h/view.h"
#include "h/language.h"
#include "h/funckey.h"
#include "h/hanzi.h"
#include "h/spell.h"
#include "h/drawmenu.h"
#include "h/submenus.h"
#include "h/save.h"
#include "h/open.h"
#include "h/inname.h"
#include "h/mouse.h"

/* Lightweight headless BGI/DOS mock implementation for automated testing */
static int g_hanzi_calls = 0;
static int g_outtextxy_calls = 0;
static int g_mouse_x = 0, g_mouse_y = 0, g_mouse_btn = 0;
static int g_keys[64];
static int g_key_head = 0, g_key_tail = 0;

static void mock_push_key(int key) { g_keys[g_key_tail++ % 64] = key; }
static void mock_clear_keys(void) { g_key_head = g_key_tail = 0; }

int bioskey(int cmd) {
    if (cmd == 1) return g_key_head < g_key_tail;
    if (g_key_head < g_key_tail) return g_keys[g_key_head++ % 64];
    return 0;
}
int getch(void) { return 0; }
void initgraph(int *gd, int *gm, const char *path) { (void)gd; (void)gm; (void)path; }
void closegraph(void) {}
void cleardevice(void) {}
void setwritemode(int mode) { (void)mode; }
void setfillstyle(int pattern, int color) { (void)pattern; (void)color; }
void setcolor(int color) { (void)color; }
void settextjustify(int horiz, int vert) { (void)horiz; (void)vert; }
void bar(int l, int t, int r, int b) { (void)l; (void)t; (void)r; (void)b; }
void rectangle(int l, int t, int r, int b) { (void)l; (void)t; (void)r; (void)b; }
void line(int x1, int y1, int x2, int y2) { (void)x1; (void)y1; (void)x2; (void)y2; }
void moveto(int x, int y) { (void)x; (void)y; }
void lineto(int x, int y) { (void)x; (void)y; }
void floodfill(int x, int y, int border) { (void)x; (void)y; (void)border; }
void outtextxy(int x, int y, const char *text) { (void)x; (void)y; (void)text; g_outtextxy_calls++; }
void putpixel(int x, int y, int color) { (void)x; (void)y; (void)color; }
void delay(unsigned int ms) { (void)ms; }
void bgi_present(void) {}
int  bgi_draw_hanzi_native(int x0, int y0, const char code[2], int color)
{ (void)x0; (void)y0; (void)code; (void)color; g_hanzi_calls++; return 1; }
int  bgi_draw_yao_native(int x0, int y0, int color)
{ (void)x0; (void)y0; (void)color; g_hanzi_calls++; return 1; }
int  bgi_is_native_hanzi(void) { return 1; }
int  bgi_set_cursor(int x, int y, int visible) { (void)x; (void)y; (void)visible; return 1; }
int  bgi_is_native_mouse(void) { return 1; }

int int86(int intno, union REGS *inregs, union REGS *outregs) {
    (void)intno;
    if (inregs->x.ax == 0) { outregs->x.ax = 1; }
    else if (inregs->x.ax == 3) {
        outregs->x.cx = (unsigned int)g_mouse_x;
        outregs->x.dx = (unsigned int)g_mouse_y;
        outregs->x.bx = (unsigned int)g_mouse_btn;
    }
    return 0;
}

extern int Find(struct EDITTYPE *pEdit, char *pGoal);
extern void LocateRow(struct EDITTYPE *pEdit, int *pRow);
extern void LocateLine(struct EDITTYPE *pEdit, int *pLine);

static void init_edit(struct EDITTYPE *e) {
    memset(e, 0, sizeof(*e));
    e->x = EDITERSCREENX1;
    e->y = EDITERSCREENY1 + CHARHIGH;
    e->lastPosX = -1;
    e->lastPosY = -1;
    strcpy(e->file.fname, "NONAME.TXT");
    e->submenu.flag = 1;
    e->submenu.SubmenuNum[0] = 5;
    e->submenu.SubmenuNum[1] = 4;
    e->submenu.SubmenuNum[2] = 2;
    e->submenu.SubmenuNum[3] = 2;
}

static void type_ascii(struct EDITTYPE *e, const char *s) {
    while (*s) {
        if (*s == '\n') {
            e->state.ascii = 13;
            GetEnter(e);
        } else {
            e->state.ascii = (unsigned char)*s;
            GetEnglish(e);
        }
        s++;
    }
}

static void verify_list_integrity(struct EDITTYPE *e) {
    struct NODE *p = e->start;
    struct NODE *prev = NULL;
    int seen_current = (e->current == NULL);
    int seen_page = (e->pageFirst == NULL);
    if (e->start == NULL) {
        assert(e->last == NULL);
        assert(e->current == NULL);
        assert(e->pageFirst == NULL);
        return;
    }
    assert(e->start->last == NULL);
    while (p != NULL) {
        assert(p->last == prev);
        if (p == e->current) seen_current = 1;
        if (p == e->pageFirst) seen_page = 1;
        prev = p;
        p = p->next;
    }
    assert(prev == e->last);
    assert(seen_current);
    assert(seen_page);
}

static void list_to_str(struct EDITTYPE *e, char *buf, int maxLen) {
    int i = 0;
    struct NODE *p = e->start;
    while (p != NULL && i < maxLen - 3) {
        if (p->flag == 0) buf[i++] = '\n';
        else if (p->flag == 1) buf[i++] = p->data[0];
        else if (p->flag == 2) { buf[i++] = p->data[0]; buf[i++] = p->data[1]; }
        p = p->next;
    }
    buf[i] = '\0';
}

static void test_empty_doc_safety(void) {
    struct EDITTYPE e;
    int r = 0, l = 0;
    init_edit(&e);

    BackSpace(&e);
    e.state.key = LEFT; MoveCursor(&e);
    e.state.key = RIGHT; MoveCursor(&e);
    e.state.key = UP; MoveCursor(&e);
    e.state.key = DOWN; MoveCursor(&e);
    AllChosen(&e);
    CopySelection(&e);
    CopyCut(&e);
    Paste(&e);
    assert(Find(&e, "abc") == 0);
    assert(Find(&e, "") == 0);
    ReplaceFunction(&e);
    LocateRow(&e, &r); assert(r == 1);
    LocateLine(&e, &l); assert(l == 1);
    ShowPosition(&e);
    RollBar(&e);
    e.editFlag = 1; ViewAll(&e);
    verify_list_integrity(&e);
    FreeAllNode(&e);
    printf("[PASS] test_empty_doc_safety\n");
}

static void test_insert_delete_and_scrolled_page(void) {
    struct EDITTYPE e;
    char buf[256];
    init_edit(&e);

    type_ascii(&e, "bc");
    verify_list_integrity(&e);
    e.current = NULL;
    e.state.ascii = 'a'; GetEnglish(&e);
    list_to_str(&e, buf, sizeof(buf));
    assert(strcmp(buf, "abc") == 0);
    verify_list_integrity(&e);

    e.current = NULL;
    e.state.ascii = 13; GetEnter(&e);
    list_to_str(&e, buf, sizeof(buf));
    assert(strcmp(buf, "\nabc") == 0);
    verify_list_integrity(&e);

    e.current = NULL;
    e.spell.pcode[0] = (char)0xba;
    e.spell.pcode[1] = (char)0xba;
    Gethanzi(&e);
    verify_list_integrity(&e);

    while (e.last != NULL) {
        DeleteNode(e.last, &e);
        verify_list_integrity(&e);
    }
    assert(e.start == NULL && e.last == NULL && e.current == NULL && e.pageFirst == NULL);

    type_ascii(&e, "line1\nline2\nline3");
    e.editFlag = 1; ViewAll(&e);
    e.pageFirst = e.start;
    while (e.pageFirst->flag != 0) e.pageFirst = e.pageFirst->next;
    e.pageFirst = e.pageFirst->next;
    e.current = NULL;
    e.state.ascii = 'X'; GetEnglish(&e);
    verify_list_integrity(&e);
    list_to_str(&e, buf, sizeof(buf));
    assert(strcmp(buf, "line1\nXline2\nline3") == 0);

    DeleteNode(e.pageFirst, &e);
    verify_list_integrity(&e);
    list_to_str(&e, buf, sizeof(buf));
    assert(strcmp(buf, "line1\nline2\nline3") == 0);

    FreeAllNode(&e);
    printf("[PASS] test_insert_delete_and_scrolled_page\n");
}

static void test_copy_cut_paste_and_self_paste(void) {
    struct EDITTYPE e;
    char buf[256];
    int i;
    init_edit(&e);

    type_ascii(&e, "hello world");
    e.editFlag = 1; ViewAll(&e);

    e.current = NULL;
    for (i = 0; i < 5; i++) {
        e.state.key = CTRL_RIGHT;
        BlackWhiteChoosen(&e);
    }
    CopySelection(&e);
    assert(e.copyCutFlag == 1 && e.copyActive == 1);

    e.current = e.start->next;
    Paste(&e);
    verify_list_integrity(&e);
    list_to_str(&e, buf, sizeof(buf));
    assert(strcmp(buf, "hehellollo world") == 0);

    e.editFlag = 1; ViewAll(&e);
    AllChosen(&e);
    e.state.key = CTRL_X;
    CopyCut(&e);
    verify_list_integrity(&e);
    assert(e.start == NULL);

    Paste(&e);
    verify_list_integrity(&e);
    list_to_str(&e, buf, sizeof(buf));
    assert(strcmp(buf, "hehellollo world") == 0);

    FreeAllNode(&e);
    printf("[PASS] test_copy_cut_paste_and_self_paste\n");
}

static void test_find_and_replace(void) {
    struct EDITTYPE e;
    char buf[256];
    char hzGoal[5];
    init_edit(&e);

    type_ascii(&e, "foo bar baz foo");
    e.editFlag = 1; ViewAll(&e);
    e.current = NULL;

    assert(Find(&e, "bar") == 1);
    strcpy(e.replaceGoal, "12345");
    ReplaceFunction(&e);
    list_to_str(&e, buf, sizeof(buf));
    assert(strcmp(buf, "foo 12345 baz foo") == 0);

    e.current = NULL;
    assert(Find(&e, "12345") == 1);
    strcpy(e.replaceGoal, "x");
    ReplaceFunction(&e);
    list_to_str(&e, buf, sizeof(buf));
    assert(strcmp(buf, "foo x baz foo") == 0);

    e.current = NULL;
    assert(Find(&e, "baz") == 1);
    strcpy(e.replaceGoal, "");
    ReplaceFunction(&e);
    list_to_str(&e, buf, sizeof(buf));
    assert(strcmp(buf, "foo x  foo") == 0);

    e.current = e.last;
    e.spell.pcode[0] = (char)0xb9; e.spell.pcode[1] = (char)0xfe; Gethanzi(&e);
    e.spell.pcode[0] = (char)0xc4; e.spell.pcode[1] = (char)0xe3; Gethanzi(&e);
    e.current = NULL;
    hzGoal[0] = (char)0xb9; hzGoal[1] = (char)0xfe;
    hzGoal[2] = (char)0xc4; hzGoal[3] = (char)0xe3;
    hzGoal[4] = '\0';
    assert(Find(&e, hzGoal) == 1);

    FreeAllNode(&e);
    printf("[PASS] test_find_and_replace\n");
}

static void test_hanzi_spell_save_open(void) {
    struct EDITTYPE e;
    char buf[256];
    int prevHanzi;
    init_edit(&e);

    prevHanzi = g_hanzi_calls;
    Printfhz(10, 10, "\xb9\xfe\xb9\xfe", BLACK, &e);
    assert(g_hanzi_calls == prevHanzi + 2);

    type_ascii(&e, "hello\nworld");
    e.spell.pcode[0] = (char)0xb9; e.spell.pcode[1] = (char)0xfe; Gethanzi(&e);
    strcpy(e.spell.name, "TEST_RT.TXT");
    Saveas(&e);
    assert(strcmp(e.file.fname, "TEST_RT.TXT") == 0);

    FreeAllNode(&e);
    strcpy(e.spell.name, "TEST_RT.TXT");
    Open(&e);
    verify_list_integrity(&e);
    list_to_str(&e, buf, sizeof(buf));
    assert(strcmp(buf, "hello\nworld\xb9\xfe") == 0);
    remove("SAVE/TEST_RT.TXT");

    mock_clear_keys();
    {
        const char *name = "ABCDEFGH.TXT";
        int i;
        for (i = 0; name[i]; i++) mock_push_key((int)name[i]);
        /* Tagged Hanzi keys must be ignored by ASCII filename prompt */
        mock_push_key(HANZI_KEY_TAG | 0xc4e3);
        mock_push_key(BACKSPACE);
        mock_push_key('C');
        mock_push_key(ENTER);
    }
    InName(&e);
    assert(strcmp(e.spell.name, "ABCDEFGH.TXC") == 0);

    FreeAllNode(&e);
    printf("[PASS] test_hanzi_spell_save_open\n");
}

static void test_wrap_firstInLine_and_perf(void) {
    struct EDITTYPE e;
    int i, x, y;
    init_edit(&e);

    for (i = 0; i < 200; i++) {
        type_ascii(&e, "0123456789abcdefghij\n");
    }
    e.editFlag = 1;
    g_outtextxy_calls = 0;
    ViewAll(&e);
    assert(g_outtextxy_calls <= 30);

    for (i = 0; i < 1000; i++) {
        assert(MoveCursorVertical(&e, EDITERSCREENX1 + 100, EDITERSCREENY1 + 10 * CHARHIGH) == 1);
        assert(ConvertNodePosi(&e, EDITERSCREENX1 + 16, EDITERSCREENY1 + 5 * CHARHIGH) == 1);
    }
    assert(GetNodePosi(&e, e.current, &x, &y) == 1);
    RollBar(&e);
    ShowPosition(&e);

    FreeAllNode(&e);
    printf("[PASS] test_wrap_firstInLine_and_perf\n");
}

static void test_submenu_switching(void) {
    struct EDITTYPE e;
    init_edit(&e);

    /* Ensure edge detector starts released */
    e.state.x = 320; e.state.y = 240; e.state.button = 0;
    MouseKey(&e);

    /* Click File (<F1>) menu button -> opens menu 1 */
    e.state.x = 20; e.state.y = 35; e.state.button = 1;
    MouseKey(&e);
    assert(e.editActive == 1);
    assert(e.state.function == 6);

    /* Holding the button on a second frame must NOT toggle it closed */
    e.state.function = 0;
    MouseKey(&e);
    assert(e.editActive == 1);

    /* Release button */
    e.state.button = 0;
    MouseKey(&e);

    /* Click leftmost column of Edit (<F2>, x=92) directly while File is open */
    e.editFlag = 0;
    e.state.x = 92; e.state.y = 35; e.state.button = 1;
    MouseKey(&e);
    assert(e.editActive == 2);
    assert(e.editFlag == 1);
    assert(e.state.function == 6);

    /* Release, then click Tools (<F3>) directly */
    e.state.button = 0; MouseKey(&e);
    e.editFlag = 0; e.state.function = 0;
    e.state.x = 200; e.state.y = 35; e.state.button = 1;
    MouseKey(&e);
    assert(e.editActive == 3);
    assert(e.editFlag == 1);
    assert(e.state.function == 6);

    /* Release, then click Help (<F4>) directly */
    e.state.button = 0; MouseKey(&e);
    e.editFlag = 0; e.state.function = 0;
    e.state.x = 280; e.state.y = 35; e.state.button = 1;
    MouseKey(&e);
    assert(e.editActive == 4);
    assert(e.editFlag == 1);
    assert(e.state.function == 6);

    /* Release, then click Help (<F4>) again -> toggles closed */
    e.state.button = 0; MouseKey(&e);
    e.editFlag = 0; e.state.function = 0;
    e.state.x = 280; e.state.y = 35; e.state.button = 1;
    MouseKey(&e);
    assert(e.editActive == 0);
    assert(e.editFlag == 1);

    /* Release, open F1, click empty menu-bar space -> closes */
    e.state.button = 0; MouseKey(&e);
    e.state.x = 20; e.state.y = 35; e.state.button = 1;
    MouseKey(&e);
    assert(e.editActive == 1);
    e.state.button = 0; MouseKey(&e);
    e.editFlag = 0;
    e.state.x = 450; e.state.y = 35; e.state.button = 1;
    MouseKey(&e);
    assert(e.editActive == 0);
    assert(e.editFlag == 1);
    e.state.button = 0; MouseKey(&e);

    /* Keyboard switching inside Submenu(): F1 -> RIGHT (F2) -> F4 -> LEFT (F3) -> F3 (close) */
    e.editActive = 1;
    e.submenu.menuflag = 1;
    mock_clear_keys();
    mock_push_key(RIGHT);
    Submenu(&e);
    assert(e.editActive == 2 && e.editFlag == 1 && e.state.function == 6);

    e.editFlag = 0; e.state.function = 0; e.submenu.menuflag = 1;
    mock_push_key(F4);
    Submenu(&e);
    assert(e.editActive == 4 && e.editFlag == 1 && e.state.function == 6);

    e.editFlag = 0; e.state.function = 0; e.submenu.menuflag = 1;
    mock_push_key(LEFT);
    Submenu(&e);
    assert(e.editActive == 3 && e.editFlag == 1 && e.state.function == 6);

    e.editFlag = 0; e.state.function = 0; e.submenu.menuflag = 1;
    mock_push_key(F3);
    Submenu(&e);
    assert(e.editActive == 0 && e.editFlag == 1);

    /* Single-row (16px) dropdown item click hit-testing: File item 5 (Close) at Y=122 */
    e.editActive = 1; e.state.function = 0; e.state.button = 0;
    MouseKey(&e);
    e.state.x = 30; e.state.y = 50 + 4 * CHARHIGH + 8; e.state.button = 1;
    MouseKey(&e);
    assert(e.editActive == 0 && e.state.function == 1);
    e.state.button = 0; MouseKey(&e);

    FreeAllNode(&e);
    printf("[PASS] test_submenu_switching\n");
}

int main(void) {
    test_empty_doc_safety();
    test_insert_delete_and_scrolled_page();
    test_copy_cut_paste_and_self_paste();
    test_find_and_replace();
    test_hanzi_spell_save_open();
    test_wrap_firstInLine_and_perf();
    test_submenu_switching();
    printf("ALL TESTS PASSED!\n");
    return 0;
}
