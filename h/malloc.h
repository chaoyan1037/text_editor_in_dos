/*****************************************************************************
 * File: MALLOC.H
 * Purpose: Checked heap allocator for `struct NODE` linked-list elements.
 *****************************************************************************/
#ifndef _MALLOC_H_
#define _MALLOC_H_

struct EDITTYPE;
struct NODE;

/* Allocate and zero-initialize one NODE; cleanly exits(1) on out-of-memory */
struct NODE *MallocNode(struct EDITTYPE *pEdit);

#endif
