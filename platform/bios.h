/*****************************************************************************
 * File: platform/bios.h
 * Purpose: Drop-in Linux/POSIX compatibility header for Turbo C <bios.h>.
 *****************************************************************************/
#ifndef _PLATFORM_BIOS_H_
#define _PLATFORM_BIOS_H_

/*
 * bioskey - Emulate DOS BIOS INT 16h keyboard polling/reading:
 *   cmd == 1: non-blocking check; returns non-zero if a key is queued.
 *   cmd == 0: blocking read; returns 16-bit (scan_code << 8 | ascii) key code.
 */
int bioskey(int cmd);

#endif
