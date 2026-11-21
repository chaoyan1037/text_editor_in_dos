#ifndef _FUNCKEY_H_
#define _FUNCKEY_H_

struct EDITTYPE;

void ExitEditer(struct EDITTYPE *pEdit);
void MoveCursor(struct EDITTYPE *pEdit);
void BlackWhiteChoosen(struct EDITTYPE *pEdit);
void Paste(struct EDITTYPE *pEdit);
void BackSpace(struct EDITTYPE *pEdit);
void CopyCut(struct EDITTYPE *pEdit);
void FindFunction(struct EDITTYPE *pEdit);
void FingGoal(struct EDITTYPE *pEdit);
void CancleFind(struct EDITTYPE *pEdit);
void ReplaceFunction(struct EDITTYPE *pEdit);
void FindAndReplace(struct EDITTYPE *pEdit);
void CancleReplace(struct EDITTYPE *pEdit);
void AllChosen(struct EDITTYPE *pEdit);
void MouseBWChosen(struct EDITTYPE *pEdit);

#endif
