/*****************************************************************************
 * File: SAVE.H
 * Purpose: Save confirmation dialog and document serialization to SAVE/.
 *****************************************************************************/
#ifndef _SAVE_H_
#define _SAVE_H_

struct EDITTYPE;

/* Draw modal "保存？ 确定(Y) / 取消(N)" confirmation dialog */
void Savewarning(struct EDITTYPE *pEdit);

/* Write all document nodes to `filename` inside SAVE/ directory */
void Save(char *filename, struct EDITTYPE *pEdit);

/* Save document under pEdit->spell.name and update title-bar file.fname */
void Saveas(struct EDITTYPE *pEdit);

#endif
