/*****************************************************************************
 * File: NODE.H
 * Purpose: Doubly-linked document list primitives, detached clipboard
 *          lifecycle management, and O(page) coordinate mapping helpers.
 *****************************************************************************/
#ifndef  _NODE_H_
#define  _NODE_H_

struct EDITTYPE;
struct NODE;

/* Append a new node after `current` (or at tail/head if empty) */
void AddNode(struct EDITTYPE *pEdit, char data[]);

/* Insert a new node after `current`, or before `pageFirst` if current==NULL */
void InsertNode(struct EDITTYPE *pEdit, char data[]);

/* Unlink and free pNode, repairing start/last/pageFirst/current/find/clip */
void DeleteNode(struct NODE *pNode, struct EDITTYPE *pEdit);

/* Free entire document list and any owned (copyCutFlag==1) clipboard list */
void FreeAllNode(struct EDITTYPE *pEdit);

/* Free all heap nodes in detached clipboard (`clipStart..clipLast`) */
void ClearClip(struct EDITTYPE *pEdit);

/* Deep-copy live selection (`clipStart..clipLast`) into detached clipboard */
void CopySelection(struct EDITTYPE *pEdit);

/* Compute screen caret coordinate (*xx, *yy) immediately AFTER pGoalNode */
int GetNodePosi(struct EDITTYPE *pEdit, struct NODE *pGoalNode, int *xx, int *yy);

/* Single-pass O(page) exact click hit-test from pixel (x, y) to caret node */
int ConvertNodePosi(struct EDITTYPE *pEdit, int x, int y);

/* Single-pass O(page) vertical navigation to row y at column closest to x */
int MoveCursorVertical(struct EDITTYPE *pEdit, int x, int y);

#endif
