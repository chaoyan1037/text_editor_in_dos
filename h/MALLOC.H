/*****************************************************************************
 * File: MALLOC.H
 * Purpose: Checked heap allocator for document linked-list nodes.
 *****************************************************************************/
#ifndef _MALLOC_H_
#define _MALLOC_H_

struct EDITTYPE;
struct NODE;

struct NODE *MallocNode(struct EDITTYPE *pEdit);

#endif
