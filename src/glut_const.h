/*Quickdraw.h
	File:		glut_const.h

	Contains:	Constants used by glutint.h

	Written by:	Various

	Copyright:	1999 Apple Computer, Inc., All Rights Reserved

	Change History (most recent first):

		 <3>     5/24/99    GGS     Removed resources references (bad form in library???) and
									changed to windProcIDs
		 <2>     5/24/99    GGS     Changed header info and moved constants from glut_const.h
		 <1>     5/23/99    GGS     OpenGL glut original source
*/

#include "Quickdraw.h"

/* Windows: */
#define kMainWindow documentProc
#define kPlainWindow plainDBox
#define kGameMode -1 /* window proc ID, just an indicator of the mode \
/* Dilogs: */
#define kAboutDialog 128
/* Menus: */
#define rMenuBar 128
/* Apple menu: */
#define mApple 128
#define iAbout 1
/* File menu: */
#define mFile 129
#define iQuit 1
/* Edit menu: */
#define mEdit 130
#define iUndo 1
#define iCut 3
#define iCopy 4
#define iPaste 5
#define iClear 6
/* Mouse menu items */
#define mMouseMenuStart 131

#define CWX 0x0001
#define CWY 0x0002
#define CWWidth 0x0004
#define CWHeight 0x0008
