/*	File:		glut_winmisc.c	Contains:		Written by:	Mark J. Kilgard	Mac OS by:	John Stauffer and Geoff Stahl	Copyright:	Copyright � Mark J. Kilgard, 1994.				MacOS portions Copyright � Apple Computer, Inc., 1999	Change History (most recent first):        <3+>     3/19/00    ggs     convert to 3.7 imp.         <3>     7/16/99    ggs     Fixed attributions and copyrights         <2>     5/31/99    ggs     remove extranious includes         <1>     5/23/99    GGS     OpenGL glut original source*//* Copyright � Mark J. Kilgard, 1994. *//* This program is freely distributable without licensing fees    and is provided without guarantee or warrantee expressed or    implied. This program is -not- in the public domain. */#include <stdlib.h>#include <stdio.h>#include <string.h>#include <assert.h>#include "glutint.h"void __glutCToStr255(const char *cs, Str255 ps){	int i = 255;	*ps++ = strlen(cs);	while(*cs && i--) *ps++ = *cs++;}/* CENTRY */void glutSetWindowTitle(const char *title){	Str255 ps;		if(!__glutCurrentWindow || !__glutCurrentWindow->win)	{		__glutWarning("glutSetWindowTitle: no active window");		return;	}		__glutCToStr255(title, ps);	#if TARGET_API_MAC_CARBON	SetWTitle(__glutCurrentWindow->win, ps);#else	SetWTitle((GrafPort *) __glutCurrentWindow->win, ps);#endif}void glutSetIconTitle(const char *title){}void glutPositionWindow(int x, int y){	if(!__glutCurrentWindow || !__glutCurrentWindow->win)	{		__glutWarning("glutPositionWindow: no active window");		return;	}	#if TARGET_API_MAC_CARBON	MoveWindow(__glutCurrentWindow->win, x, y, GL_FALSE);#else	MoveWindow((GrafPort *) __glutCurrentWindow->win, x, y, GL_FALSE);#endif}void glutReshapeWindow(int w, int h){	if(!__glutCurrentWindow || !__glutCurrentWindow->win)	{		__glutWarning("glutReshapeWindow: no active window");		return;	}		if(w <= 0 || h <= 0)	{		__glutWarning("glutReshapeWindow: non-positive width or height not allowed");		return;	}	#if TARGET_API_MAC_CARBON	SizeWindow(__glutCurrentWindow->win, w, h, GL_FALSE);#else	SizeWindow((GrafPort *) __glutCurrentWindow->win, w, h, GL_FALSE);#endif	__glutWindowReshape(__glutCurrentWindow, w, h);}void glutPopWindow(void){	if(!__glutCurrentWindow || !__glutCurrentWindow->win)	{		__glutWarning("glutPopWindow: no active window");		return;	}#if TARGET_API_MAC_CARBON	BringToFront(__glutCurrentWindow->win);#else	BringToFront((GrafPort *) __glutCurrentWindow->win);#endif}void glutPushWindow(void){	if(!__glutCurrentWindow || !__glutCurrentWindow->win)	{		__glutWarning("glutPushWindow: no active window");		return;	}#if TARGET_API_MAC_CARBON	SendBehind(__glutCurrentWindow->win, 0);#else	SendBehind((GrafPort *) __glutCurrentWindow->win, 0);#endif}void glutIconifyWindow(void){}int __glutFindVisState (WindowPtr pWindow){#if TARGET_API_MAC_CARBON	// this may be expense since it copies a region...	Rect rectVis;	static RgnHandle  rgnTemp = NULL;	long width, height;	if (rgnTemp == NULL)		rgnTemp = NewRgn ();	GetPortVisibleRegion (GetWindowPort (pWindow), rgnTemp);	GetRegionBounds (rgnTemp, &rectVis);	width = rectVis.right - rectVis.left; 	height = rectVis.bottom - rectVis.top; //	DisposeRgn (rgnTemp); // will only leak one region per run, should be disposed on app clean up#else	long width = (**pWindow->visRgn).rgnBBox.right - (**pWindow->visRgn).rgnBBox.left; 	long height = (**pWindow->visRgn).rgnBBox.bottom - (**pWindow->visRgn).rgnBBox.top; #endif	if ((width > 0) && (height > 0))		return GLUT_VISIBLE;	else 		return GLUT_FULLY_COVERED;}void glutShowWindow(void){	if(!__glutCurrentWindow || !__glutCurrentWindow->win)	{		__glutWarning("glutShowWindow: no active window");		return;	}	#if TARGET_API_MAC_CARBON	ShowWindow(__glutCurrentWindow->win);#else	ShowWindow((GrafPort *) __glutCurrentWindow->win);#endif	__glutCurrentWindow->visState = __glutFindVisState ((WindowPtr)__glutCurrentWindow->win);}void glutHideWindow(void){	if(!__glutCurrentWindow || !__glutCurrentWindow->win)	{		__glutWarning("glutHideWindow: no active window");		return;	}		// this is guarenteed to be correct here    __glutCurrentWindow->visState = GLUT_HIDDEN;#if TARGET_API_MAC_CARBON	HideWindow(__glutCurrentWindow->win);#else	HideWindow((GrafPort *) __glutCurrentWindow->win);#endif}/* ENDCENTRY */