#ifndef _SAVE_H_
#define _SAVE_H_

struct EDITTYPE;

void Savewarning(struct EDITTYPE *pEdit);
void Save(char *filename, struct EDITTYPE *pEdit);
void Saveas(struct EDITTYPE *pEdit);

#endif
