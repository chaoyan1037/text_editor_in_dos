/*****************************************************************************
 * File: VIEW.H
 * Purpose: Full-viewport text renderer and visual line-wrap synchronizer.
 *****************************************************************************/
#ifndef _VIEW_H_
#define _VIEW_H_

struct EDITTYPE;

/* Repaint text viewport when editFlag==1 and title bar when fileflag==1 */
void ViewAll(struct EDITTYPE *pEdit);

#endif
