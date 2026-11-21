/*****************************************************************************
 * File: platform/conio.h
 * Purpose: Drop-in Linux/POSIX compatibility header for Turbo C <conio.h>.
 *****************************************************************************/
#ifndef _PLATFORM_CONIO_H_
#define _PLATFORM_CONIO_H_

/* Blocking single-key read without echo */
int getch(void);

#endif
