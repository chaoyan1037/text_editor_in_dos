#ifndef  _NODE_H_
#define  _NODE_H_

void AddNode(struct EDITTYPE *pEdit, char data[]);
void InsertNode(struct EDITTYPE *pEdit, char data[]);
void FreeAllNode( struct EDITTYPE *pEdit );
void ClearClip(struct EDITTYPE *pEdit);
int GetNodePosi( struct EDITTYPE *pEdit, struct NODE *pGoalNode, int *xx, int *yy);
int ConvertNodePosi(struct EDITTYPE *pEdit, int x, int y);

#endif