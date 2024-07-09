/*Timer.h
	File:		glut_event.c

	Contains:

	Written by:	Mark J. Kilgard
	Mac OS by:	John Stauffer and Geoff Stahl

	Copyright:	Copyright � Mark J. Kilgard, 1994.
				MacOS portions Copyright � Apple Computer, Inc., 1994-1999

	Change History (most recent first):

	   <11+>    10/18/99    ggs     work on handling visibility
		<11>    10/17/99    ggs     Changed timer handling to conform with 3.7 (removed requirement
									for current window)
		<10>    10/13/99    ggs
		 <9>     8/28/99    ggs     From js: Fixed Mac includes, Removed auto front window setting
		 <8>     7/16/99    ggs     fixed siuox exit code
		 <7>     7/16/99    ggs     Remove edit menu reference, changed event modifier ignore to
									optionkey vice shift key
		 <6>     7/16/99    ggs     Fixed attributions and copyrights
		 <5>     7/16/99    ggs     Removed all full screen references
		 <4>     6/22/99    ggs     Added contextual menus (still need to get rid of help if
									possible and add error checking)
		 <3>     6/22/99    ggs     Chnaged menu key from option to control for right button,
									removed more fullscreen threading (do not need it anymore)
		 <2>     5/31/99    ggs     remove extranious includes
		 <1>     5/23/99    GGS     OpenGL glut original source
*/

#if defined(macintosh)
#if TARGET_API_MAC_CARBON
#define CALL_NOT_IN_CARBON 1 // ensure DrawSprocket calls are found (must weak link and runtime check)
#include <DrawSprocket.h>
#undef CALL_NOT_IN_CARBON
#else
#include <DrawSprocket.h>
#endif

#include <Events.h>
#include <MacWindows.h>
#include <Dialogs.h>
#include <ToolUtils.h>
#include <Devices.h>
#include <Script.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <Timer.h>

#if defined(macintosh)
void DoZoomWindow(WindowPtr pWindow, short zoomInOrOut);
#if TARGET_API_MAC_CARBON
pascal void IdleTimer(EventLoopTimerRef inTimer, void *userData);
EventLoopTimerUPP GetTimerUPP(void);
#endif

#if TARGET_API_MAC_CARBON
enum
{
	kActiveSleep = 10,
	kInactiveSleep = 10000
};
EventLoopTimerRef gTimer = NULL;
#else
enum
{
	kActiveSleep = 0,
	kInactiveSleep = 100
};
#endif
SInt32 gSleepTime = kActiveSleep;
#endif

#include "glut.h"
#include "glutint.h"

#ifdef __MWERKS__
#include "SIOUX.h"
#endif

GLUTtimer *freeTimerList = NULL;

GLUTidleCB __glutIdleFunc = NULL;
GLUTtimer *__glutTimerList = NULL;

#ifdef SUPPORT_FORTRAN
GLUTtimer *__glutNewTimer;
#endif

GLUTwindow *__glutWindowWorkList = NULL;
unsigned int __glutModifierMask = ~0; /* ~0 implies not in
										 core input callback. */
int __glutWindowDamaged = 0;

void __glutMenuEvent(long menuAndItem);
void __glutDrag(WindowPtr window, Point mouseloc);
void __glutContentClick(WindowPtr window, EventRecord *myEvent);
Boolean IsDAccWindow(WindowPtr window);
void __glutGoAwayBox(WindowPtr window, Point mouseloc);
void __glutCloseWindow(WindowPtr window);
void __glutZoom(WindowPtr window, Point mouseloc, int part);
void __glutKeyDown(EventRecord *myEvent);
void __glutKeyUp(EventRecord *myEvent);
void __glutDiskEvent(EventRecord *myEvent);
void __glutOSEvent(EventRecord *myEvent);
void __glutUpdate(WindowPtr window);
void __glutActivate(WindowPtr window, int myModifiers);
void __glutAboutBox(void);
void __glutNextEvent(void);
void __glutMouseDown(EventRecord *myEvent);
void __glutMouseUp(EventRecord *myEvent);

void glutIdleFunc(GLUTidleCB idleFunc)
{
	__glutIdleFunc = idleFunc;
}

void glutTimerFunc(unsigned int interval, GLUTtimerCB timerFunc, int value)
{
	GLUTtimer *timer, *other;
	GLUTtimer **prevptr;
	double now;

	if (!timerFunc)
		return;

	if (freeTimerList)
	{
		timer = freeTimerList;
		freeTimerList = timer->next;
	}
	else
	{
		timer = (GLUTtimer *)malloc(sizeof(GLUTtimer));
		if (!timer)
			__glutFatalError("out of memory.");
	}

	timer->func = timerFunc;
	timer->timeout = interval;

	timer->value = value;
	timer->next = NULL;

	now = __glutTime();
	ADD_TIME(timer->timeout, timer->timeout, now);

	prevptr = &__glutTimerList;
	other = *prevptr;

	while (other && IS_AFTER(other->timeout, timer->timeout))
	{
		prevptr = &other->next;
		other = *prevptr;
	}

	timer->next = other;

#ifdef SUPPORT_FORTRAN
	__glutNewTimer = timer; /* for Fortran binding! */
#endif

	*prevptr = timer;
}

static void handleTimeouts(void)
{
	double now;
	GLUTtimer *timer;

	/* Assumption is that __glutTimerList is already determined
	   to be non-NULL. */
	now = __glutTime();
	while (IS_AT_OR_AFTER(__glutTimerList->timeout, now))
	{
		timer = __glutTimerList;
		timer->func(timer->value);
		__glutTimerList = timer->next;
		timer->next = freeTimerList;
		freeTimerList = timer;
		if (!__glutTimerList)
			break;
	}

	/*
		double now;
		GLUTtimer *timer;

		if(!__glutCurrentWindow) return;

		__glutSetCurrentWindow(__glutCurrentWindow);

		if(__glutTimerList)
		{
			now = __glutTime();
			while(IS_AT_OR_AFTER(__glutTimerList->timeout, now))
			{
				timer = __glutTimerList;
				timer->func(timer->value);

				__glutTimerList = timer->next;
				timer->next = freeTimerList;

				freeTimerList = timer;

				if (!__glutTimerList) break;
			}
		}
	*/
}

void __glutPutOnWorkList(GLUTwindow *window, int workMask)
{
	if (workMask)
	{
		if (window->workMask)
		{
			window->workMask |= workMask;
		}
		else
		{
			window->workMask = workMask;
			window->prevWorkWin = __glutWindowWorkList;
			__glutWindowWorkList = window;
		}
	}
}

void __glutRemoveFromWorkList(GLUTwindow *window)
{
	GLUTwindow *win, *prev_win, *next_win;

	prev_win = NULL;
	win = __glutWindowWorkList;
	next_win = NULL;
	while (win)
	{
		next_win = win->prevWorkWin;

		if (win == window)
			break;

		prev_win = win;
		win = next_win;
	}

	if (win)
	{
		if (prev_win)
		{
			prev_win->prevWorkWin = next_win;
		}
		else
		{
			__glutWindowWorkList = next_win;
		}
	}
}

void __glutPostRedisplay(GLUTwindow *window, int layerMask)
{
	int shown = (layerMask & (GLUT_REDISPLAY_WORK | GLUT_REPAIR_WORK)) ? window->shownState : window->overlay->shownState;

	/* Post a redisplay if the window is visible (or the
	   visibility of the window is unknown, ie. window->visState
	   == -1) _and_ the layer is known to be shown. */
	if (window->visState != GLUT_HIDDEN && window->visState != GLUT_FULLY_COVERED && shown)
	{
		__glutPutOnWorkList(window, layerMask);
	}
}

void glutPostRedisplay(void)
{
	if (!__glutCurrentWindow)
	{
		__glutWarning("glutPostRedisplay: no active window");
		return;
	}
	__glutPostRedisplay(__glutCurrentWindow, GLUT_REDISPLAY_WORK);
}

/* The advantage of this routine is that it saves the cost of a
   glutSetWindow call (entailing an expensive OpenGL context switch),
   particularly useful when multiple windows need redisplays posted at
   the same times.  See also glutPostWindowOverlayRedisplay. */
void glutPostWindowRedisplay(int win)
{
	__glutPostRedisplay(__glutWindowList[win - 1], GLUT_REDISPLAY_WORK);
}

void updateWindowState(GLUTwindow *window, int visState)
{
	// GLUTwindow* child;

	/* XXX shownState and visState are the same in Win32. */
	window->shownState = visState;
	if (visState != window->visState)
	{
		if (window->window_status)
		{
			window->visState = visState;
			__glutSetCurrentWindow(window);
			window->window_status(visState);
		}
	}
	/* Since Win32 only sends an activate for the toplevel window,
	   update the visibility for all the child windows. */
	//  child = window->children;
	//  while (child) {
	//    updateWindowState(child, visState);
	//    child = child->siblings;
	//  }
}

static void __glutDrawGrowIcon(WindowPtr window)
{
	Rect iconRect;
	RgnHandle oldClip;
	GrafPtr origPort;

	GetPort(&origPort);
#if TARGET_API_MAC_CARBON
	SetPortWindowPort(window);
#else
	SetPort(window);
#endif

	if (window)
	{
		oldClip = NewRgn();

#if TARGET_API_MAC_CARBON
		SetPortWindowPort(window);
#else
		SetPort(window);
#endif
		GetClip(oldClip);

#if TARGET_API_MAC_CARBON
		GetWindowPortBounds(window, &iconRect);
#else
		iconRect = window->portRect;
#endif
		iconRect.top = iconRect.bottom - 15;
		iconRect.left = iconRect.right - 15;

		ClipRect(&iconRect);

		PenNormal();
		DrawGrowIcon(window);

		SetClip(oldClip);
		DisposeRgn(oldClip);
	}

	SetPort(origPort);
}

static void __glutGrow(WindowPtr window, Point mouseloc)
{
	Rect growBounds;
	long newSize;
	GrafPtr origPort;
	GLUTwindow *glut_win;

	GetPort(&origPort);
#if TARGET_API_MAC_CARBON
	SetPortWindowPort(window);
#else
	SetPort(window);
#endif

#if TARGET_API_MAC_CARBON
	GetRegionBounds(GetGrayRgn(), &growBounds);
#else
	growBounds = (**GetGrayRgn()).rgnBBox;
#endif

	growBounds.right -= growBounds.left;
	growBounds.bottom -= growBounds.top;
	growBounds.left = 20;
	growBounds.top = 20;

	newSize = GrowWindow(window, mouseloc, &growBounds);

	if (newSize)
	{
		SizeWindow(window, newSize & 0x0000FFFF, newSize >> 16, false);

#if TARGET_API_MAC_CARBON
		glut_win = __glutGetWindow(window);
#else
		glut_win = __glutGetWindow((CGrafPtr)window);
#endif
		__glutWindowReshape(glut_win, LoWord(newSize), HiWord(newSize));
		__glutPutOnWorkList(glut_win, GLUT_REDISPLAY_WORK);
	}

	SetPort(origPort);
}

void __glutWindowReshape(GLUTwindow *glut_win, int width, int height)
{
	//	GrafPtr		origPort;

	if (!__glutSetCurrentWindow(glut_win))
		return;

	glut_win->width = width;
	glut_win->height = height;

	if (glut_win->reshape)
	{
		(*glut_win->reshape)(glut_win->width, glut_win->height);
	}
	glut_win->forceReshape = false;
	/* A reshape should be considered like posting a
	   repair request. */
	__glutPostRedisplay(glut_win, GLUT_REPAIR_WORK);

	//	SetPort(origPort);
}

void DoZoomWindow(WindowPtr pWindow, short zoomInOrOut)
{
	GDHandle gdZoomOnThisDevice = NULL;
	Rect windRect;

#if TARGET_API_MAC_CARBON
	GetWindowPortBounds(pWindow, &windRect);
	EraseRect(&windRect); // erase to avoid flicker
#else
	EraseRect(&pWindow->portRect); // erase to avoid flicker
#endif
	if (zoomInOrOut == inZoomOut) // zooming to standard state
	{							  // locate window on available graphics devices
#if TARGET_API_MAC_CARBON
		GetWindowPortBounds(pWindow, &windRect);
#else
		windRect = pWindow->portRect;
#endif
		LocalToGlobal((Point *)&windRect.top); // convert to global coordinates
		LocalToGlobal((Point *)&windRect.bottom);
		// calculate height of window's title bar
#if TARGET_API_MAC_CARBON
#else
		{
			GDHandle gdNthDevice;
			Rect zoomRect, theSect;
			long sectArea, greatestArea;
			short wTitleHeight, wFrameHeight;
			Boolean sectFlag;
			wFrameHeight = windRect.left - 1 - (**(((WindowPeek)pWindow)->strucRgn)).rgnBBox.left;
			wTitleHeight = windRect.top - 1 - (**(((WindowPeek)pWindow)->strucRgn)).rgnBBox.top;
			windRect.top = windRect.top - wTitleHeight;
			gdNthDevice = GetDeviceList();
			greatestArea = 0; // initialize to 0
			// check window against all gdRects in gDevice list and remember
			//  which gdRect contains largest area of window}
			while (gdNthDevice)
			{
				if (TestDeviceAttribute(gdNthDevice, screenDevice))
					if (TestDeviceAttribute(gdNthDevice, screenActive))
					{
						// The SectRect routine calculates the intersection
						//  of the window rectangle and this gDevice
						//  rectangle and returns TRUE if the rectangles intersect,
						//  FALSE if they don't.
						sectFlag = SectRect(&windRect, &(**gdNthDevice).gdRect, &theSect);
						// determine which screen holds greatest window area
						//  first, calculate area of rectangle on current device
						sectArea = (long)(theSect.right - theSect.left) * (theSect.bottom - theSect.top);
						if (sectArea > greatestArea)
						{
							greatestArea = sectArea;		  // set greatest area so far
							gdZoomOnThisDevice = gdNthDevice; // set zoom device
						}
						gdNthDevice = GetNextDevice(gdNthDevice);
					}
			} // of WHILE
			// if gdZoomOnThisDevice is on main device, allow for menu bar height
			if (gdZoomOnThisDevice == GetMainDevice())
				wTitleHeight = wTitleHeight + GetMBarHeight();
			// set the zoom rectangle to the full screen, minus window title
			//  height (and menu bar height if necessary), inset by 3 pixels
			SetRect(&zoomRect,
					(**gdZoomOnThisDevice).gdRect.left + wFrameHeight + 3, (**gdZoomOnThisDevice).gdRect.top + wTitleHeight + 3,
					(**gdZoomOnThisDevice).gdRect.right - wFrameHeight - 3 - 1, (**gdZoomOnThisDevice).gdRect.bottom - wFrameHeight - 3 - 1);
			// If your application has a different "most useful" standard
			//  state, then size the zoom window accordingly.
			// set up the WStateData record for this window
			(**(WStateDataHandle)((WindowPeek)pWindow)->dataHandle).stdState = zoomRect;
		}
#endif

	} // of inZoomOut
	// if zoomInOrOut = inZoomIn, just let ZoomWindow zoom to user state}
	//  zoom the window frame}
	ZoomWindow(pWindow, zoomInOrOut, false); // pWindow == FrontWindow());
}

void __glutZoom(WindowPtr window, Point mouseloc, int part)
{
	short w, h;
	GrafPtr origPort;
	GLUTwindow *glut_win;
	Rect window_port_bounds;

	// need to keep this
	GetPort(&origPort);

#if TARGET_API_MAC_CARBON
	SetPortWindowPort(window);
#else
	SetPort(window);
#endif

	if (TrackBox(window, mouseloc, part))
	{
		// do big zoom handling here
		//		ZoomWindow(window, part, false);
		DoZoomWindow(window, part);
#if TARGET_API_MAC_CARBON
		GetWindowPortBounds(window, &window_port_bounds);
#else
		window_port_bounds = window->portRect;
#endif
		w = abs(window_port_bounds.right - window_port_bounds.left);
		h = abs(window_port_bounds.top - window_port_bounds.bottom);

#if TARGET_API_MAC_CARBON
		glut_win = __glutGetWindow(window);
#else
		glut_win = __glutGetWindow((CGrafPtr)window);
#endif
		__glutWindowReshape(glut_win, w, h);
		__glutPutOnWorkList(glut_win, GLUT_REDISPLAY_WORK);
	}
	SetPort(origPort);
}

static void __glutRedraw(GLUTwindow *glut_win)
{
	GrafPtr origPort;
	GetPort(&origPort);

#if TARGET_API_MAC_CARBON
	SetPortWindowPort(glut_win->win);
#else
	SetPort((GrafPtr)glut_win->win);
#endif
	if (glut_win->display)
		(*glut_win->display)();

	SetPort(origPort);
}

void __glutSelectWindowHierarchy(GLUTwindow *glut_win)
{
	GLUTwindow *child;
	SelectWindow((WindowPtr)glut_win->win);
	child = glut_win->children;
	while (child)
	{
		__glutSelectWindowHierarchy(child);
		child = child->siblings;
	}
}

void __glutMouseDown(EventRecord *event)
{
	short part;
	WindowPtr window;
	GLUTwindow *glut_win;

	part = FindWindow(event->where, &window);

	switch (part)
	{
	case inMenuBar:
		__glutMenuEvent(MenuSelect(event->where));
		break;
#if TARGET_API_MAC_CARBON
#else
	case inSysWindow:
		SystemClick(event, window);
		break;
#endif
	case inDrag:
		__glutDrag(window, event->where);
		break;
	case inGoAway:
		__glutGoAwayBox(window, event->where);
		break;
	case inGrow:
		__glutGrow(window, event->where);
		break;
	case inZoomIn:
	case inZoomOut:
		__glutZoom(window, event->where, part);
		break;
	case inContent:
		if (!window)
			return;

			// revised to work on current window better
#if TARGET_API_MAC_CARBON
		glut_win = __glutGetWindow(window);
#else
		glut_win = __glutGetWindow((CGrafPtr)window);
#endif
		if (!__glutSetCurrentWindow(glut_win))
			return;
		if (__glutCurrentWindow)
		{
			if ((WindowPtr)__glutCurrentWindow->win != FrontWindow())
			{
				// SelectWindow((GrafPtr)__glutCurrentWindow->win);
				//  Recursively activate all children
				__glutSelectWindowHierarchy(__glutCurrentWindow);
				__glutModifierMask = event->modifiers;
				__glutPutOnWorkList(__glutCurrentWindow, GLUT_BUTTON_WORK);
#if TARGET_API_MAC_CARBON
				__glutContentClick(__glutCurrentWindow->win, event);
#else
				__glutContentClick((GrafPtr)__glutCurrentWindow->win, event);
#endif
			}
			else
			{
				__glutModifierMask = event->modifiers;
				__glutPutOnWorkList(__glutCurrentWindow, GLUT_BUTTON_WORK);
#if TARGET_API_MAC_CARBON
				__glutContentClick(__glutCurrentWindow->win, event);
#else
				__glutContentClick((GrafPtr)__glutCurrentWindow->win, event);
#endif
			}
		}
		break;
	}
}

void __glutMouseUp(EventRecord *event)
{
	GrafPtr origPort;
	int i;

	//	if(__glutCurrentWindow->fullscreen) return;

	GetPort(&origPort);

	for (i = 0; i < __glutWindowListSize; i++)
	{
		if (!__glutWindowList[i])
			continue;

		if (__glutWindowList[i]->mouse && __glutWindowList[i]->button_press)
		{
#if TARGET_API_MAC_CARBON
			SetPortWindowPort(__glutWindowList[i]->win);
#else
			SetPort((GrafPtr)__glutWindowList[i]->win);
#endif
			GlobalToLocal(&event->where);

			if (!__glutSetCurrentWindow(__glutWindowList[i]))
				continue;

			(*__glutWindowList[i]->mouse)(__glutWindowList[i]->button_press - 1, GLUT_UP, event->where.h, event->where.v);
		}

		__glutWindowList[i]->button_press = 0;
	}

	SetPort(origPort);
}

void __glutMoveWindow(GLUTwindow *glut_win, Point pntDelta)
{
	GLUTwindow *cur, *siblings;
	GrafPtr origPort;
	Point pntLoc;

	cur = glut_win->children;
	while (cur)
	{
		siblings = cur->siblings;
		__glutMoveWindow(cur, pntDelta);
		cur = siblings;
	}
	GetPort(&origPort);
#if TARGET_API_MAC_CARBON
	SetPortWindowPort(glut_win->win);
#else
	SetPort((GrafPtr)glut_win->win);
#endif

#if TARGET_API_MAC_CARBON
	{
		Rect window_bounds_rect;
		GetWindowPortBounds(glut_win->win, &window_bounds_rect);
		pntLoc.h = window_bounds_rect.left;
		pntLoc.v = window_bounds_rect.top;
	}
#else
	pntLoc.h = glut_win->win->portRect.left;
	pntLoc.v = glut_win->win->portRect.top;
#endif
	LocalToGlobal(&pntLoc);
	SetPort(origPort);
	pntLoc.h += pntDelta.h;
	pntLoc.v += pntDelta.v;

#if TARGET_API_MAC_CARBON
	MoveWindow(glut_win->win, pntLoc.h, pntLoc.v, false);
#else
	MoveWindow((GrafPtr)glut_win->win, pntLoc.h, pntLoc.v, false);
#endif
	// ensure selection hieracrhy is maintained
	__glutSelectWindowHierarchy(glut_win);
}

void __glutDrag(WindowPtr window, Point mouseloc)
{
	Point pntLoc1, pntLoc2;
	GrafPtr origPort;
	Rect dragBounds;
	GLUTwindow *glut_win;
	GLUTwindow *cur, *siblings;
#if TARGET_API_MAC_CARBON
	Rect window_bounds_rect;
#endif

	GetPort(&origPort);
#if TARGET_API_MAC_CARBON
	SetPortWindowPort(window);
#else
	SetPort((GrafPtr)window);
#endif

#if TARGET_API_MAC_CARBON
	GetWindowPortBounds(window, &window_bounds_rect);
	pntLoc1.h = window_bounds_rect.left;
	pntLoc1.v = window_bounds_rect.top;
#else
	pntLoc1.h = window->portRect.left;
	pntLoc1.v = window->portRect.top;
#endif

	LocalToGlobal(&pntLoc1);
	SetPort(origPort);

#if TARGET_API_MAC_CARBON
	GetRegionBounds(GetGrayRgn(), &dragBounds);
#else
	dragBounds = (**GetGrayRgn()).rgnBBox;
#endif

	DragWindow(window, mouseloc, &dragBounds);

	GetPort(&origPort);
#if TARGET_API_MAC_CARBON
	SetPortWindowPort(window);
#else
	SetPort((GrafPtr)window);
#endif

#if TARGET_API_MAC_CARBON
	GetWindowPortBounds(window, &window_bounds_rect);

	pntLoc2.h = window_bounds_rect.left;
	pntLoc2.v = window_bounds_rect.top;

#else
	pntLoc2.h = window->portRect.left;
	pntLoc2.v = window->portRect.top;
#endif

	LocalToGlobal(&pntLoc2);
	SetPort(origPort);

	pntLoc2.h -= pntLoc1.h;
	pntLoc2.v -= pntLoc1.v;

#if TARGET_API_MAC_CARBON
	glut_win = __glutGetWindow(window);
#else
	glut_win = __glutGetWindow((CGrafPtr)window);
#endif

	cur = glut_win->children;
	while (cur)
	{
		siblings = cur->siblings;
		__glutMoveWindow(cur, pntLoc2);
		cur = siblings;
	}

	if (!__glutSetCurrentWindow(glut_win))
		return;

	/* beretta: added the following to force a redraw when the window is moved */
	GetPort(&origPort);
#if TARGET_API_MAC_CARBON
	SetPortWindowPort(glut_win->win);
#else
	SetPort((GrafPtr)glut_win->win);
#endif

	//?tad: let's see if we don't need to:
	//? 	__glutPutOnWorkList(glut_win, GLUT_REDISPLAY_WORK);

	SetPort(origPort);
}

void __glutContentClick(WindowPtr window, EventRecord *myEvent)
{
	GrafPtr origPort;
	GLUTwindow *glut_win;
	int x, y;
	short menuID;
	unsigned short menuItem;

	//	if(__glutCurrentWindow->fullscreen)
	//	{
	//		if(!__glutCurrentWindow) return;

	//		__glutSetCurrentWindow(__glutCurrentWindow);

	//		return;
	//	}
	//	else
	{
#if TARGET_API_MAC_CARBON
		glut_win = __glutGetWindow(window);
#else
		glut_win = __glutGetWindow((CGrafPtr)window);
#endif
		if (!__glutSetCurrentWindow(glut_win))
			return;
	}

	GetPort(&origPort);
#if TARGET_API_MAC_CARBON
	SetPortWindowPort(window);
#else
	SetPort(window);
#endif

	x = myEvent->where.h;
	y = myEvent->where.v;
	GlobalToLocal(&myEvent->where);

	__glutModifierMask = myEvent->modifiers;
	//	__glutModifierMask &= ~shiftKey;    /* shift is not used as a modifier here  */
	__glutModifierMask &= ~optionKey;  /* option is not used as a modifier here  */
	__glutModifierMask &= ~controlKey; /* control is not used as a modifier here  */

	if (myEvent->modifiers & controlKey)
	{
		if (__glutContextualMenuSelect(x, y, GLUT_RIGHT_BUTTON, &menuID, &menuItem))
		{
			__glutMenuCommand(menuID, menuItem);
		}
		else
		{
			glut_win->button_press = GLUT_RIGHT_BUTTON + 1;
			if (glut_win->mouse)
			{
				(*glut_win->mouse)(GLUT_RIGHT_BUTTON, GLUT_DOWN, myEvent->where.h, myEvent->where.v);
			}
		}
	}
	else if (myEvent->modifiers & optionKey)
	{
		if (__glutContextualMenuSelect(x, y, GLUT_MIDDLE_BUTTON, &menuID, &menuItem))
		{
			__glutMenuCommand(menuID, menuItem);
		}
		else
		{
			glut_win->button_press = GLUT_MIDDLE_BUTTON + 1;
			if (glut_win->mouse)
			{
				(*glut_win->mouse)(GLUT_MIDDLE_BUTTON, GLUT_DOWN, myEvent->where.h, myEvent->where.v);
			}
		}
	}
	else
	{
		if (__glutContextualMenuSelect(x, y, GLUT_LEFT_BUTTON, &menuID, &menuItem))
		{
			__glutMenuCommand(menuID, menuItem);
		}
		else
		{
			glut_win->button_press = GLUT_LEFT_BUTTON + 1;
			if (glut_win->mouse)
			{
				(*glut_win->mouse)(GLUT_LEFT_BUTTON, GLUT_DOWN, myEvent->where.h, myEvent->where.v);
			}
		}
	}

	SetPort(origPort);
}

Boolean IsDAccWindow(WindowPtr window)
{
#if TARGET_API_MAC_CARBON
	return false;
#else
	if (window == nil)
	{
		return false;
	}
	else
	{
		return (((WindowPeek)window)->windowKind < 0);
	}
#endif
}

void __glutGoAwayBox(WindowPtr window, Point mouseloc)
{
	GrafPtr origPort;

	GetPort(&origPort);
#if TARGET_API_MAC_CARBON
	SetPortWindowPort(window);
#else
	SetPort(window);
#endif
	if (TrackGoAway(window, mouseloc))
	{
		__glutCloseWindow(window);
	}

	SetPort(origPort);
}

void __glutCloseWindow(WindowPtr window)
{
	GLUTwindow *glut_win;
	GrafPtr origPort;

	//	if(__glutCurrentWindow->fullscreen) return;

	if (window)
	{
		GetPort(&origPort);
#if TARGET_API_MAC_CARBON
		SetPortWindowPort(window);
#else
		SetPort(window);
#endif
		if (IsDAccWindow(window))
		{
#if !TARGET_API_MAC_CARBON
			CloseDeskAcc(((WindowPeek)window)->windowKind);
#endif
		}
		else
		{
#if TARGET_API_MAC_CARBON
			glut_win = __glutGetWindow(window);
#else
			glut_win = __glutGetWindow((CGrafPtr)window);
#endif
			__glutDestroyWindow(glut_win, glut_win);
		}

		SetPort(origPort);
	}
}

void __glutKeyDown(EventRecord *myEvent)
{
	char myCharCode, myKeyCode;
	GrafPtr origPort;
	GLboolean specasc;
	GLUTwindow *glut_win;

	// tad: moved menu code here to get menu keys even if no open GLUT window:

	myCharCode = (char)BitAnd(myEvent->message, charCodeMask);
	myKeyCode = (char)(BitAnd(myEvent->message, keyCodeMask) >> 8);

	if (myEvent->modifiers & cmdKey)
	{
		if (myEvent->what != autoKey)
		{
			__glutMenuKeyCommand();
			__glutMenuEvent(MenuKey(myCharCode));
		}
	}

	if (!__glutCurrentWindow)
		return;

	//	if(__glutCurrentWindow->fullscreen)
	//	{
	//		if(!__glutSetCurrentWindow(__glutCurrentWindow)) return;
	//	}
	//	else
	{
#if TARGET_API_MAC_CARBON
		glut_win = __glutGetWindow(FrontWindow());
#else
		glut_win = __glutGetWindow((CGrafPtr)FrontWindow());
#endif
		if (!__glutSetCurrentWindow(glut_win))
			return;

		GetPort(&origPort);
#if TARGET_API_MAC_CARBON
		SetPortWindowPort(__glutCurrentWindow->win);
#else
		SetPort((GrafPtr)__glutCurrentWindow->win);
#endif
	}

	if (__glutCurrentWindow->keyboard || __glutCurrentWindow->special)
	{
		__glutModifierMask = myEvent->modifiers;
		// mirror right modifiers to normal mask
		if (__glutModifierMask & rightShiftKey)
			__glutModifierMask |= shiftKey;
		if (__glutModifierMask & rightOptionKey)
			__glutModifierMask |= optionKey;
		if (__glutModifierMask & rightControlKey)
			__glutModifierMask |= controlKey;
		if (__glutModifierMask & optionKey) // convert alt (option) keys to to plain values that glut expects
		{
			UInt32 state = 0;
			long keyResult = KeyTranslate((void *)GetScriptManagerVariable(smKCHRCache), (UInt16)myKeyCode, &state);
			myCharCode = 0xFF & keyResult;
		}

		GlobalToLocal(&myEvent->where);

		if (__glutCurrentWindow->keyboard && (myCharCode >= 0x20 || (myCharCode < 0x1C && myCharCode != 0x01 && myCharCode != 0x04 && myCharCode != 0x05 && myCharCode != 0x0B && myCharCode != 0x0C && myCharCode != 0x10)))
		{
			(*__glutCurrentWindow->keyboard)(myCharCode, myEvent->where.h, myEvent->where.v);
		}
		else if (__glutCurrentWindow->special)
		{
			specasc = true;
			switch (myCharCode)
			{
			case 0x1C:
				(*__glutCurrentWindow->special)(GLUT_KEY_LEFT, myEvent->where.h, myEvent->where.v);
				break;
			case 0x1E:
				(*__glutCurrentWindow->special)(GLUT_KEY_UP, myEvent->where.h, myEvent->where.v);
				break;
			case 0x1D:
				(*__glutCurrentWindow->special)(GLUT_KEY_RIGHT, myEvent->where.h, myEvent->where.v);
				break;
			case 0x1F:
				(*__glutCurrentWindow->special)(GLUT_KEY_DOWN, myEvent->where.h, myEvent->where.v);
				break;
			case 0x0B:
				(*__glutCurrentWindow->special)(GLUT_KEY_PAGE_UP, myEvent->where.h, myEvent->where.v);
				break;
			case 0x0C:
				(*__glutCurrentWindow->special)(GLUT_KEY_PAGE_DOWN, myEvent->where.h, myEvent->where.v);
				break;
			case 0x01:
				(*__glutCurrentWindow->special)(GLUT_KEY_HOME, myEvent->where.h, myEvent->where.v);
				break;
			case 0x04:
				(*__glutCurrentWindow->special)(GLUT_KEY_END, myEvent->where.h, myEvent->where.v);
				break;
			case 0x05:
				(*__glutCurrentWindow->special)(GLUT_KEY_INSERT, myEvent->where.h, myEvent->where.v);
				break;
			default:
				specasc = false;
				break;
			}

			if (!specasc)
			{
				switch (myKeyCode)
				{
				case 0x7A:
					(*__glutCurrentWindow->special)(GLUT_KEY_F1, myEvent->where.h, myEvent->where.v);
					break;
				case 0x78:
					(*__glutCurrentWindow->special)(GLUT_KEY_F2, myEvent->where.h, myEvent->where.v);
					break;
				case 0x63:
					(*__glutCurrentWindow->special)(GLUT_KEY_F3, myEvent->where.h, myEvent->where.v);
					break;
				case 0x76:
					(*__glutCurrentWindow->special)(GLUT_KEY_F4, myEvent->where.h, myEvent->where.v);
					break;
				case 0x60:
					(*__glutCurrentWindow->special)(GLUT_KEY_F5, myEvent->where.h, myEvent->where.v);
					break;
				case 0x61:
					(*__glutCurrentWindow->special)(GLUT_KEY_F6, myEvent->where.h, myEvent->where.v);
					break;
				case 0x62:
					(*__glutCurrentWindow->special)(GLUT_KEY_F7, myEvent->where.h, myEvent->where.v);
					break;
				case 0x64:
					(*__glutCurrentWindow->special)(GLUT_KEY_F8, myEvent->where.h, myEvent->where.v);
					break;
				case 0x65:
					(*__glutCurrentWindow->special)(GLUT_KEY_F9, myEvent->where.h, myEvent->where.v);
					break;
				case 0x6D:
					(*__glutCurrentWindow->special)(GLUT_KEY_F10, myEvent->where.h, myEvent->where.v);
					break;
				case 0x67:
					(*__glutCurrentWindow->special)(GLUT_KEY_F11, myEvent->where.h, myEvent->where.v);
					break;
				case 0x6F:
					(*__glutCurrentWindow->special)(GLUT_KEY_F12, myEvent->where.h, myEvent->where.v);
					break;
				default:
					break;
				}
			}
		}
	}

	//	if(!__glutCurrentWindow->fullscreen)
	SetPort(origPort);
}

void __glutKeyUp(EventRecord *myEvent)
{
	char myCharCode, myKeyCode;
	GrafPtr origPort;
	GLboolean specasc;
	GLUTwindow *glut_win;

	if (!__glutCurrentWindow)
		return;

#if TARGET_API_MAC_CARBON
	glut_win = __glutGetWindow(FrontWindow());
#else
	glut_win = __glutGetWindow((CGrafPtr)FrontWindow());
#endif
	if (!__glutSetCurrentWindow(glut_win))
		return;

	GetPort(&origPort);
#if TARGET_API_MAC_CARBON
	SetPortWindowPort(__glutCurrentWindow->win);
#else
	SetPort((GrafPtr)__glutCurrentWindow->win);
#endif

	myCharCode = (char)BitAnd(myEvent->message, charCodeMask);
	myKeyCode = (char)(BitAnd(myEvent->message, keyCodeMask) >> 8);

	if (__glutCurrentWindow->keyboardUp || __glutCurrentWindow->specialUp)
	{
		__glutModifierMask = myEvent->modifiers;

		GlobalToLocal(&myEvent->where);

		if (__glutCurrentWindow->keyboardUp && (myCharCode >= 0x20 || (myCharCode < 0x1C && myCharCode != 0x01 && myCharCode != 0x04 && myCharCode != 0x05 && myCharCode != 0x0B && myCharCode != 0x0C && myCharCode != 0x10)))
		{
			(*__glutCurrentWindow->keyboardUp)(myCharCode, myEvent->where.h, myEvent->where.v);
		}
		else if (__glutCurrentWindow->specialUp)
		{
			specasc = true;
			switch (myCharCode)
			{
			case 0x1C:
				(*__glutCurrentWindow->specialUp)(GLUT_KEY_LEFT, myEvent->where.h, myEvent->where.v);
				break;
			case 0x1E:
				(*__glutCurrentWindow->specialUp)(GLUT_KEY_UP, myEvent->where.h, myEvent->where.v);
				break;
			case 0x1D:
				(*__glutCurrentWindow->specialUp)(GLUT_KEY_RIGHT, myEvent->where.h, myEvent->where.v);
				break;
			case 0x1F:
				(*__glutCurrentWindow->specialUp)(GLUT_KEY_DOWN, myEvent->where.h, myEvent->where.v);
				break;
			case 0x0B:
				(*__glutCurrentWindow->specialUp)(GLUT_KEY_PAGE_UP, myEvent->where.h, myEvent->where.v);
				break;
			case 0x0C:
				(*__glutCurrentWindow->specialUp)(GLUT_KEY_PAGE_DOWN, myEvent->where.h, myEvent->where.v);
				break;
			case 0x01:
				(*__glutCurrentWindow->specialUp)(GLUT_KEY_HOME, myEvent->where.h, myEvent->where.v);
				break;
			case 0x04:
				(*__glutCurrentWindow->specialUp)(GLUT_KEY_END, myEvent->where.h, myEvent->where.v);
				break;
			case 0x05:
				(*__glutCurrentWindow->specialUp)(GLUT_KEY_INSERT, myEvent->where.h, myEvent->where.v);
				break;
			default:
				specasc = false;
				break;
			}

			if (!specasc)
			{
				switch (myKeyCode)
				{
				case 0x7A:
					(*__glutCurrentWindow->specialUp)(GLUT_KEY_F1, myEvent->where.h, myEvent->where.v);
					break;
				case 0x78:
					(*__glutCurrentWindow->specialUp)(GLUT_KEY_F2, myEvent->where.h, myEvent->where.v);
					break;
				case 0x63:
					(*__glutCurrentWindow->specialUp)(GLUT_KEY_F3, myEvent->where.h, myEvent->where.v);
					break;
				case 0x76:
					(*__glutCurrentWindow->specialUp)(GLUT_KEY_F4, myEvent->where.h, myEvent->where.v);
					break;
				case 0x60:
					(*__glutCurrentWindow->specialUp)(GLUT_KEY_F5, myEvent->where.h, myEvent->where.v);
					break;
				case 0x61:
					(*__glutCurrentWindow->specialUp)(GLUT_KEY_F6, myEvent->where.h, myEvent->where.v);
					break;
				case 0x62:
					(*__glutCurrentWindow->specialUp)(GLUT_KEY_F7, myEvent->where.h, myEvent->where.v);
					break;
				case 0x64:
					(*__glutCurrentWindow->specialUp)(GLUT_KEY_F8, myEvent->where.h, myEvent->where.v);
					break;
				case 0x65:
					(*__glutCurrentWindow->specialUp)(GLUT_KEY_F9, myEvent->where.h, myEvent->where.v);
					break;
				case 0x6D:
					(*__glutCurrentWindow->specialUp)(GLUT_KEY_F10, myEvent->where.h, myEvent->where.v);
					break;
				case 0x67:
					(*__glutCurrentWindow->specialUp)(GLUT_KEY_F11, myEvent->where.h, myEvent->where.v);
					break;
				case 0x6F:
					(*__glutCurrentWindow->specialUp)(GLUT_KEY_F12, myEvent->where.h, myEvent->where.v);
					break;
				default:
					break;
				}
			}
		}
	}
	SetPort(origPort);
}

void __glutDiskEvent(EventRecord *myEvent)
{
}

void __glutOSEvent(EventRecord *myEvent)
{
	if (0x01000000 & myEvent->message) //	Suspend/resume event
	{
		// need to handle DSp suspend resume events here
		if (0x00000001 & myEvent->message) //	Resume
		{
			if (__glutGameModeWindow)
				__glutResumeGameMode();
			gSleepTime = kActiveSleep;
		}
		else // suspend
		{
			if (__glutGameModeWindow)
				__glutSuspendGameMode();
			gSleepTime = kInactiveSleep;
		}
	}
}

void __glutUpdate(WindowPtr window)
{
	GrafPtr origPort;
	GLUTwindow *glut_win;

	if (!__glutCurrentWindow)
		return;

	//	if(!__glutCurrentWindow->fullscreen)
	{
		/* This is done to reattach the drawable in case of a color depth change */
		__glutSetCurrentWindow(__glutCurrentWindow);

		GetPort(&origPort);
#if TARGET_API_MAC_CARBON
		SetPortWindowPort(window);
#else
		SetPort(window);
#endif

		BeginUpdate(window);
		EndUpdate(window);

#if TARGET_API_MAC_CARBON
		glut_win = __glutGetWindow(window);
#else
		glut_win = __glutGetWindow((CGrafPtr)window);
#endif
		if (window)
		{
			if (glut_win)
				updateWindowState(glut_win, __glutFindVisState(window));
		}

		__glutPostRedisplay(glut_win, GLUT_REPAIR_WORK);
		//		__glutPutOnWorkList(glut_win, GLUT_REDISPLAY_WORK);

		SetPort(origPort);
	}
	//	else
	//	{
	//		__glutPutOnWorkList(__glutCurrentWindow, GLUT_REDISPLAY_WORK);
	//	}
}

void __glutActivate(WindowPtr window, int modifiers)
{
	GLUTwindow *glut_win;

	if (modifiers & activeFlag != 0)
	{
		if (window)
		{
#if TARGET_API_MAC_CARBON
			glut_win = __glutGetWindow(window);
#else
			glut_win = __glutGetWindow((CGrafPtr)window);
#endif
			if (glut_win)
				__glutSetCurrentWindow(glut_win);
		}

		__glutRebuildMenuBar();
	}
	else
	{
		// check to see if window is now completely hidden
		// if vis region bbox is not null
		// compare vis region to window rect
		// we know that there is a vis change
		// call the call back
		// set vistate (new var) to invis
		if (window)
		{
#if TARGET_API_MAC_CARBON
			glut_win = __glutGetWindow(window);
#else
			glut_win = __glutGetWindow((CGrafPtr)window);
#endif
			if (glut_win)
				updateWindowState(glut_win, __glutFindVisState(window));
		}
	}
}

void __glutAboutBox(void)
{
	DialogPtr myDialog;
	short itemHit;

	myDialog = GetNewDialog(kAboutDialog, nil, (WindowPtr)-1);
	ModalDialog(nil, &itemHit);
	DisposeDialog(myDialog);
}

void __glutMenuEvent(long menuAndItem)
{
	int menuNum;
	int itemNum;
#if !TARGET_API_MAC_CARBON
	int result;
	Str255 DAName;
#endif
	Point point;
	DrawMenuBar();

	menuNum = HiWord(menuAndItem);
	itemNum = LoWord(menuAndItem);

	if (__glutCurrentWindow)
	{
		GLUTwindow *glut_win, *pWinSave = __glutCurrentWindow;

#if TARGET_API_MAC_CARBON
		glut_win = __glutGetWindow(FrontWindow());
#else
		glut_win = __glutGetWindow((CGrafPtr)FrontWindow());
#endif
		if (glut_win != NULL)
		{
			if (!__glutSetCurrentWindow(glut_win))
				__glutSetCurrentWindow(pWinSave);
		}
		else
		{
			__glutSetCurrentWindow(pWinSave);
		}

		if (__glutCurrentWindow->menu_status)
		{
			GetMouse(&point);
			__glutCurrentWindow->menu_status(GLUT_MENU_IN_USE, point.h, point.v);
		}
		else if (__glutCurrentWindow->menu_state)
		{
			__glutCurrentWindow->menu_state(GLUT_MENU_IN_USE);
		}

		if (__glutCurrentWindow->menu_status)
		{
			GetMouse(&point);
			__glutCurrentWindow->menu_status(GLUT_MENU_NOT_IN_USE, point.h, point.v);
		}
		else if (__glutCurrentWindow->menu_state)
		{
			__glutCurrentWindow->menu_state(GLUT_MENU_NOT_IN_USE);
		}
	}

	switch (menuNum)
	{
	case mApple:
		switch (itemNum)
		{
		case iAbout:
			__glutAboutBox();
			break;
		default:
#if !TARGET_API_MAC_CARBON
			GetMenuItemText(GetMenuHandle(mApple), itemNum, DAName);
			result = OpenDeskAcc(DAName);
#endif
			break;
		}
		break;
	case mFile:
		switch (itemNum)
		{
		case iQuit:
			//	if (__glutCurrentWindow)
			//	  __glutDestroyWindow(__glutCurrentWindow, __glutCurrentWindow);
			glutLeaveGameMode();
			ExitToShell();
			exit(0);
			break;
		}
		break;
	default:
		if (__glutCurrentWindow)
			__glutMenuCommand(menuNum, itemNum);
		break;
	}

	HiliteMenu(0);
}

static Boolean __glutConsoleEvent(EventRecord *myEvent)
{
	GLboolean flag = GL_FALSE;

#ifdef __MWERKS__
	flag = SIOUXHandleOneEvent(myEvent);
#endif

	return flag;
}

GLboolean __glutPointInWindow(GLUTwindow *window, Point *point)
{
#if TARGET_API_MAC_CARBON
	Rect window_port_bounds;

	if (window->win != FrontWindow())
		return GL_FALSE;

	GetWindowPortBounds(window->win, &window_port_bounds);

	return point->v > window_port_bounds.top && point->v < window_port_bounds.bottom && point->h > window_port_bounds.left && point->h < window_port_bounds.right;

#else

	GrafPtr win = (GrafPtr)window->win;
	Rect *rect = &win->portRect;

	if (win == FrontWindow())
	{
		if (point->v > rect->top && point->v < rect->bottom &&
			point->h > rect->left && point->h < rect->right)
		{
			return GL_TRUE;
		}
	}
	return GL_FALSE;
#endif
}

static void processEvents(void)
{
	EventRecord myEvent;

#if TARGET_API_MAC_CARBON
	while (WaitNextEvent(everyEvent, &myEvent, gSleepTime, nil))
	{
#else
	while (OSEventAvail(everyEvent, &myEvent) || CheckUpdate(&myEvent))
	{
		if (WaitNextEvent(everyEvent, &myEvent, gSleepTime, nil))
#endif
		{
			Boolean wasProcessed = false;

			if (__glutGameModeWindow)
				DSpProcessEvent(&myEvent, &wasProcessed);
			if (!wasProcessed && !__glutConsoleEvent(&myEvent))
			{
				switch (myEvent.what)
				{
				case mouseDown:
					__glutMouseDown(&myEvent);
					break;
				case mouseUp:
					__glutMouseUp(&myEvent);
					break;
				case keyDown:
					__glutKeyDown(&myEvent);
					__glutModifierMask = ~0;
					break;
				case keyUp:
					__glutKeyUp(&myEvent);
					__glutModifierMask = ~0;
					break;
				case autoKey:
					if ((__glutCurrentWindow) && (__glutCurrentWindow->ignoreKeyRepeat == false))
						__glutKeyDown(&myEvent);
					__glutModifierMask = ~0;
					break;
				case updateEvt:
					__glutUpdate((WindowPtr)myEvent.message);
					break;
				case diskEvt:
					__glutDiskEvent(&myEvent);
					break;
				case activateEvt:
					__glutActivate((WindowPtr)myEvent.message, myEvent.modifiers);
					break;
				case osEvt:
					__glutOSEvent(&myEvent);
					break;
				}
			}
		}
	}
}

static void processWindowWorkList(GLUTwindow *window)
{
	unsigned int workMask;
	Point point;
	GrafPtr origPort;

	/* Recursively call window work */
	if (window->prevWorkWin)
		processWindowWorkList(window->prevWorkWin);
	window->prevWorkWin = NULL;

	/* Get work mask*/
	workMask = window->workMask;

	/* Clear work mask */
	window->workMask = GLUT_NO_WORK;

	/* Make context current.
	** We do not needlessly set the current window here. */
	if (window != __glutCurrentWindow && !__glutSetCurrentWindow(window))
		return;

	GetPort(&origPort);
#if TARGET_API_MAC_CARBON
	SetPortWindowPort(window->win);
#else
	SetPort((GrafPtr)window->win); /* sets port for local coordinates below */
#endif
	/* If mouse motion */
	if (window->button_press)
	{
		if (window->motion)
		{
			GetMouse(&point);
			window->motion(point.h, point.v);
		}

		__glutPutOnWorkList(window, GLUT_BUTTON_WORK);
	}
	/* Else if passive work */
	else if (window->passive)
	{
		GetMouse(&point);
		window->passive(point.h, point.v);

		__glutPutOnWorkList(window, GLUT_PASSIVE_WORK);
	}

	/* If entry work */
	if (window->entry)
	{
		GLboolean in_window;

		GetMouse(&point);

		in_window = __glutPointInWindow(window, &point);

		if (window->entry_mode == GLUT_LEFT && in_window)
		{
			window->entry(GLUT_ENTERED);
			window->entry_mode = GLUT_ENTERED;
		}
		else if (window->entry_mode == GLUT_ENTERED && !in_window)
		{
			window->entry(GLUT_LEFT);
			window->entry_mode = GLUT_LEFT;
		}

		__glutPutOnWorkList(window, GLUT_ENTRY_WORK);
	}

	/* Do display work */
	if (workMask & (GLUT_REDISPLAY_WORK | GLUT_OVERLAY_REDISPLAY_WORK | GLUT_REPAIR_WORK | GLUT_OVERLAY_REPAIR_WORK))
	{
		if (window->forceReshape)
		{
			/* Guarantee that before a display callback is generated
			   for a window, a reshape callback must be generated. */
			__glutSetCurrentWindow(window);
			window->reshape(window->width, window->height);
			window->forceReshape = false;

			/* Setting the redisplay bit on the first reshape is
			   necessary to make the "Mesa glXSwapBuffers to repair
			   damage" hack operate correctly.  Without indicating a
			   redisplay is necessary, there's not an initial back
			   buffer render from which to blit from when damage
			   happens to the window. */
			workMask |= GLUT_REDISPLAY_WORK;
		}
		/* 3.6
				if(window->forceReshape)
				{
					if(window->visibility) window->visibility(GLUT_VISIBLE);
					if(window->reshape)    window->reshape(window->width, window->height);

					window->forceReshape = false;
				}
		*/
		/* Render to normal plane (and possibly overlay). */
		__glutWindowDamaged = (workMask & (GLUT_OVERLAY_REPAIR_WORK | GLUT_REPAIR_WORK));
		__glutSetCurrentWindow(window);
		window->usedSwapBuffers = 0;
		//			window->display();
		//		if (window->visState)
		__glutRedraw(window);
		__glutWindowDamaged = 0;

		//	3.6
		//			__glutRedraw(window);
	}
	/* Combine workMask with window->workMask to determine what
	 finish and debug work there is. */
	workMask |= window->workMask;

	if (workMask & GLUT_FINISH_WORK)
	{
		/* Finish work makes sure a glFinish gets done to indirect
		   rendering contexts.  Indirect contexts tend to have much
		   longer latency because lots of OpenGL extension requests
		   can queue up in the X protocol stream. __glutSetWindow
		   is where the finish works gets queued for indirect
		   contexts. */
		__glutSetCurrentWindow(window);
		glFinish();
	}
	/* Do finish work */
	/* 3.6
		if(workMask & GLUT_FINISH_WORK)
		{
			glFinish();
		}
	*/

	if (workMask & GLUT_DEBUG_WORK)
	{
		//		__glutSetWindow(window);
		glutReportErrors();
	}
	/* Do debug work */
	/* 3.6
		if(workMask & GLUT_DEBUG_WORK)
		{
			GLenum error;

			while((error = glGetError()) != GL_NO_ERROR)
			{
				__glutWarning("GL error: %s", gluErrorString(error));
			}
		}
	*/

	/* Strip out dummy, finish, and debug work bits. */
	window->workMask &= ~(GLUT_DUMMY_WORK | GLUT_FINISH_WORK | GLUT_DEBUG_WORK);

	SetPort(origPort);

	/* 3.7
		if (window->workMask) {
			// Leave on work list.
			return window;
		} else {
			// Remove current window from work list.
			return window->prevWorkWin;
		}
	*/
}

#if TARGET_API_MAC_CARBON
pascal void IdleTimer(EventLoopTimerRef inTimer, void *userData)
{
#pragma unused(inTimer, userData)
	GLUTwindow *glut_win;
	/* Process work list */
	if (__glutWindowWorkList)
	{
		glut_win = __glutWindowWorkList;
		__glutWindowWorkList = NULL;

		processWindowWorkList(glut_win);
	}

	/* If idle */
	if (__glutIdleFunc)
	{
		if (__glutCurrentWindow)
		{
			__glutIdleFunc();
		}
	}

	/* If timer list */
	if (__glutTimerList)
		handleTimeouts();
}

EventLoopTimerUPP GetTimerUPP(void)
{
	static EventLoopTimerUPP sTimerUPP = NULL;

	if (sTimerUPP == NULL)
		sTimerUPP = NewEventLoopTimerUPP(IdleTimer);

	return sTimerUPP;
}
#endif

void glutMainLoop(void)
{
#if !TARGET_API_MAC_CARBON
	GLUTwindow *glut_win;
#endif

	if (!__glutWindowListSize)
	{
		__glutFatalUsage("main loop entered with no windows created.");
	}

#if TARGET_API_MAC_CARBON
	InstallEventLoopTimer(GetCurrentEventLoop(), 0, 0.000001, GetTimerUPP(), 0, &gTimer);
#endif
	while (!gQuit)
	{
		/* Process OS events */
		processEvents();

#if !TARGET_API_MAC_CARBON
		/* Process work list */
		if (__glutWindowWorkList)
		{
			glut_win = __glutWindowWorkList;
			__glutWindowWorkList = NULL;

			processWindowWorkList(glut_win);
		}

		/* If idle */
		if (__glutIdleFunc)
		{
			if (__glutCurrentWindow)
			{
				__glutIdleFunc();
			}
		}

		/* If timer list */
		if (__glutTimerList)
			handleTimeouts();
#endif
	}
#if TARGET_API_MAC_CARBON
	RemoveEventLoopTimer(gTimer);
	gTimer = NULL;
#endif
}
