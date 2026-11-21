/*****************************************************************************
 * File: platform/dos.h
 * Purpose: Drop-in Linux/POSIX compatibility header for Turbo C <dos.h>,
 *          emulating INT 33h mouse register calls and millisecond delay().
 *****************************************************************************/
#ifndef _PLATFORM_DOS_H_
#define _PLATFORM_DOS_H_

struct WORDREGS {
    unsigned int ax, bx, cx, dx, si, di, cflag, flags;
};

struct BYTEREGS {
    unsigned char al, ah, bl, bh, cl, ch, dl, dh;
};

union REGS {
    struct WORDREGS x;
    struct BYTEREGS h;
};

/* Emulate DOS software interrupt (INT 33h mouse driver AX=0,3,7,8) */
int int86(int intno, union REGS *inregs, union REGS *outregs);

/* Sleep for `ms` milliseconds while keeping host UI events responsive */
void delay(unsigned int ms);

#endif
