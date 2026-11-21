/*****************************************************************************
 * File: NODE.H
 * Purpose: Doubly-linked document list primitives, clipboard ownership
 *          management, and screen-to-node coordinate mapping utilities.
 *****************************************************************************/
#ifndef  _NODE_H_
#define  _NODE_H_

struct EDITTYPE;
struct NODE;

void AddNode(struct EDITTYPE *pEdit, char data[]);      /* Append after current/tail */
void InsertNode(struct EDITTYPE *pEdit, char data[]);   /* Insert after current or before pageFirst */
void DeleteNode(struct NODE *pNode, struct EDITTYPE *pEdit); /* Unlink & free node safely */
void FreeAllNode(struct EDITTYPE *pEdit);               /* Free document and owned clipboard */
void ClearClip(struct EDITTYPE *pEdit);                 /* Free detached clipboard list */
void CopySelection(struct EDITTYPE *pEdit);             /* Deep-copy selection into clipboard */
int GetNodePosi(struct EDITTYPE *pEdit, struct NODE *pGoalNode, int *xx, int *yy);
int ConvertNodePosi(struct EDITTYPE *pEdit, int x, int y);     /* Exact click hit-test */
int MoveCursorVertical(struct EDITTYPE *pEdit, int x, int y);  /* Single-pass UP/DOWN target */

#endif
