/*
	File:		glut_init.c

	Contains:

	Written by:	Mark J. Kilgard
	Mac OS by:	John Stauffer and Geoff Stahl

	Copyright:	Copyright � Mark J. Kilgard, 1994.
				MacOS portions Copyright � Apple Computer, Inc., 1999

	Change History (most recent first):

	   <10+>     2/18/00    ggs     fix negative init x and init y
		<10>    10/13/99    ggs
		 <9>     8/28/99    ggs     From js: Fixed includes, added QDGlobals definition for
									non-Metrowerks builds
		 <8>     7/16/99    ggs     various
		 <7>     7/16/99    ggs     Changed modifier for command line to control key
		 <6>     7/16/99    ggs     Fixed attributions and copyrights
		 <5>     6/22/99    ggs     Added contextual menus (still need to get rid of help if
									possible and add error checking)
		 <4>     6/22/99    ggs     Added option key for command line arguments
		 <3>     6/22/99    ggs     Changed back to original argument handling
		 <2>     5/31/99    ggs     remove extranious includes
		 <1>     5/23/99    GGS     OpenGL glut original source
*/

/* Copyright � Mark J. Kilgard, 1994. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(macintosh)
#include <Events.h>
#include <Fonts.h>
#if TARGET_API_MAC_CARBON
#include <Processes.h>
#endif
#ifndef TRUE
#define TRUE true
#endif
#ifndef FALSE
#define FALSE false
#endif
#endif

#include "glut.h"
#include "glutint.h"

#if defined(__MWERKS__) || defined(__SC__)
#include "console.h"
#endif

#if defined(__MWERKS__)
#include "SIOUX.h"
#endif

#if defined(macintosh)
#ifndef __MWERKS__
QDGlobals qd;
#endif
#endif

/* GLUT inter-file variables */
char *__glutProgramName = NULL;
int __glutArgc = 0;
char **__glutArgv = NULL;
char *__glutGeometry = NULL;
GLboolean __glutDebug = GL_FALSE;
static GLboolean __glutInited = GL_FALSE;
short __glutInitialModifiers = 0; /* Mac: start event modifiers */

unsigned int __glutDisplayMode = GLUT_RGB | GLUT_SINGLE | GLUT_DEPTH;

int __glutInitWidth = 300;
int __glutInitHeight = 300;
int __glutInitX = -1;
int __glutInitY = -1;

static void parseGeometry(char *param, int *x, int *y, int *w, int *h);

// add from glut 3.7 to support better geometry input parsing
static int ReadInteger(char *string, char **NextString);
static int XParseGeometry(char *string, int *x, int *y, unsigned int *width, unsigned int *height);

/*
 * Bitmask returned by XParseGeometry().  Each bit tells if the corresponding
 * value (x, y, width, height) was found in the parsed string.
 */
enum
{
	NoValue = 0x0000,
	XValue = 0x0001,
	YValue = 0x0002,
	WidthValue = 0x0004,
	HeightValue = 0x0008,
	AllValues = 0x000F,
	XNegative = 0x0010,
	YNegative = 0x0020
};

static void DoInitManagers(void);
static void DoInitConsole(void);

#if TARGET_API_MAC_CARBON
// Carbon default directory support
Boolean GetAppFSSpec(FSSpec *pApp_spec);
Boolean SetDefaultDirectory();
#endif

// tad added to support AEQuitEvents:
Boolean gQuit = false;

#if defined(__MWERKS__)
extern OSErr __initialize(const CFragInitBlock *theInitBlock);
extern void __terminate(void);

OSErr __glutInitLibrary(const CFragInitBlock *theInitBlock);
void __glutTerminateLibrary(void);

OSErr __glutInitLibrary(const CFragInitBlock *theInitBlock)
{
	OSErr err;

	err = __initialize(theInitBlock);

	return err;
}

void __glutTerminateLibrary(void)
{
	GLint i;

	/* Remove all windows */
	for (i = 0; i < __glutWindowListSize; i++)
	{
		if (__glutWindowList[i])
		{
			__glutDestroyWindow(__glutWindowList[i], __glutWindowList[i]);
		}
	}

	if (__glutWindowList)
		free(__glutWindowList);
	if (freeTimerList)
		free(freeTimerList);

	/* Free command line args */
	for (i = 0; i < __glutArgc; i++)
	{
		free(__glutArgv[i]);
	}

	if (__glutArgc > 0)
		free(__glutArgv);

	/* Call default terminate */
	__terminate();
}
#endif

double __glutTime(void)
{
	UnsignedWide tk_time;

	Microseconds(&tk_time);

	return (4294.967296 * tk_time.hi + 0.000001 * tk_time.lo) * 1000.0;
}

void __glutInitTime(double *beginning)
{
	static int beenhere = 0;
	static double genesis;

	if (!beenhere)
	{
		genesis = __glutTime();
		beenhere = 1;
	}

	*beginning = genesis;
}

static void removeArgs(int *argcp, char **argv, int numToRemove)
{
	int i, j;

	for (i = 0, j = numToRemove; argv[j]; i++, j++)
		argv[i] = argv[j];

	argv[i] = NULL;
	*argcp -= numToRemove;
}

/* the following function was stolen from the X sources as indicated. */

/* Copyright 	Massachusetts Institute of Technology  1985, 1986, 1987 */
/* $XConsortium: XParseGeom.c,v 11.18 91/02/21 17:23:05 rws Exp $ */

/*
Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation, and that the name of M.I.T. not be used in advertising or
publicity pertaining to distribution of the software without specific,
written prior permission.  M.I.T. makes no representations about the
suitability of this software for any purpose.  It is provided "as is"
without express or implied warranty.
*/

/*
 *    XParseGeometry parses strings of the form
 *   "=<width>x<height>{+-}<xoffset>{+-}<yoffset>", where
 *   width, height, xoffset, and yoffset are unsigned integers.
 *   Example:  "=80x24+300-49"
 *   The equal sign is optional.
 *   It returns a bitmask that indicates which of the four values
 *   were actually found in the string.  For each value found,
 *   the corresponding argument is updated;  for each value
 *   not found, the corresponding argument is left unchanged.
 */

static int
ReadInteger(char *string, char **NextString)
{
	register int Result = 0;
	int Sign = 1;

	if (*string == '+')
		string++;
	else if (*string == '-')
	{
		string++;
		Sign = -1;
	}
	for (; (*string >= '0') && (*string <= '9'); string++)
	{
		Result = (Result * 10) + (*string - '0');
	}
	*NextString = string;
	if (Sign >= 0)
		return (Result);
	else
		return (-Result);
}

int XParseGeometry(char *string, int *x, int *y, unsigned int *width, unsigned int *height)
{
	int mask = NoValue;
	register char *strind;
	unsigned int tempWidth, tempHeight;
	int tempX, tempY;
	char *nextCharacter;

	if ((string == NULL) || (*string == '\0'))
		return (mask);
	if (*string == '=')
		string++; /* ignore possible '=' at beg of geometry spec */

	strind = (char *)string;
	if (*strind != '+' && *strind != '-' && *strind != 'x')
	{
		tempWidth = ReadInteger(strind, &nextCharacter);
		if (strind == nextCharacter)
			return (0);
		strind = nextCharacter;
		mask |= WidthValue;
	}

	if (*strind == 'x' || *strind == 'X')
	{
		strind++;
		tempHeight = ReadInteger(strind, &nextCharacter);
		if (strind == nextCharacter)
			return (0);
		strind = nextCharacter;
		mask |= HeightValue;
	}

	if ((*strind == '+') || (*strind == '-'))
	{
		if (*strind == '-')
		{
			strind++;
			tempX = -ReadInteger(strind, &nextCharacter);
			if (strind == nextCharacter)
				return (0);
			strind = nextCharacter;
			mask |= XNegative;
		}
		else
		{
			strind++;
			tempX = ReadInteger(strind, &nextCharacter);
			if (strind == nextCharacter)
				return (0);
			strind = nextCharacter;
		}
		mask |= XValue;
		if ((*strind == '+') || (*strind == '-'))
		{
			if (*strind == '-')
			{
				strind++;
				tempY = -ReadInteger(strind, &nextCharacter);
				if (strind == nextCharacter)
					return (0);
				strind = nextCharacter;
				mask |= YNegative;
			}
			else
			{
				strind++;
				tempY = ReadInteger(strind, &nextCharacter);
				if (strind == nextCharacter)
					return (0);
				strind = nextCharacter;
			}
			mask |= YValue;
		}
	}

	/* If strind isn't at the end of the string the it's an invalid
		geometry specification. */

	if (*strind != '\0')
		return (0);

	if (mask & XValue)
		*x = tempX;
	if (mask & YValue)
		*y = tempY;
	if (mask & WidthValue)
		*width = tempWidth;
	if (mask & HeightValue)
		*height = tempHeight;
	return (mask);
}

static void parseGeometry(char *param, int *x, int *y, int *w, int *h)
{
	int i, j;
	char s[21], *sp;
	char st[4] = {'x', '+', '-', '\0'};
	int *pt[4];

	/* Default numbers */
	*x = 20;
	*y = 20;
	*w = 300;
	*h = 300;

	/* Init param pointers */
	pt[0] = w;
	pt[1] = h;
	pt[2] = x;
	pt[3] = y;

	sp = param;

	for (j = 0; j < 4; j++)
	{
		for (i = 0; (i < 20) && (*sp) && (*sp != st[j]); i++)
		{
			s[i] = *sp;
			sp++;
		}

		sp++;

		if (i == 20)
			return;

		s[i] = 0;

		*pt[j] = atoi(s);
	}
}

static pascal OSErr QuitAppleEventHandler(const AppleEvent *, AppleEvent *, UInt32)
{
	gQuit = true;
	return noErr;
}

static void DoInitManagers(void)
{
	EventRecord event;

#if TARGET_API_MAC_CARBON
	OSErr err;
#endif

#if !TARGET_API_MAC_CARBON
	MaxApplZone();
	MoreMasters();

	InitGraf(&qd.thePort);
	InitFonts();
	FlushEvents(everyEvent, 0);
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(nil);
	qd.randSeed = TickCount();
#endif
	InitCursor();
	InitContextualMenus();

#if TARGET_API_MAC_CARBON
	err = AEInstallEventHandler(kCoreEventClass, kAEQuitApplication, NewAEEventHandlerUPP(QuitAppleEventHandler), 0, false);
#endif

	// init events
	EventAvail(everyEvent, &event);
	__glutInitialModifiers |= event.modifiers;
	EventAvail(everyEvent, &event);
	__glutInitialModifiers |= event.modifiers;
	EventAvail(everyEvent, &event);
	__glutInitialModifiers |= event.modifiers;
}

static void DoInitConsole(void)
{
#if defined(__MWERKS__)
	SIOUXSettings.initializeTB = FALSE;
	SIOUXSettings.standalone = FALSE;
	SIOUXSettings.setupmenus = FALSE;
	SIOUXSettings.autocloseonquit = TRUE;
	SIOUXSettings.asktosaveonclose = FALSE;
	SIOUXSettings.showstatusline = TRUE;
	SIOUXSettings.userwindowtitle = NULL;
	SIOUXSettings.tabspaces = 4;
	SIOUXSettings.columns = 60;
	SIOUXSettings.rows = 8;
	SIOUXSettings.toppixel = 100;
	SIOUXSettings.leftpixel = 100;
	SIOUXSettings.fontid = kFontIDMonaco;
	SIOUXSettings.fontsize = 9;
	SIOUXSettings.fontface = normal;
	SIOUXSettings.enabledraganddrop = FALSE;
	SIOUXSettings.outlinehilite = FALSE;
	SIOUXSettings.wasteusetempmemory = TRUE;
#endif
}

void __glutInit()
{
	double unused;
	if (__glutInited)
		return;

	DoInitConsole();

	DoInitManagers();

	__glutBuildMenuBar();

	__glutInitTime(&unused);

	__glutInited = GL_TRUE;
}

/* C++ static initializer
struct StaticInit
{
	StaticInit()
	{
		__glutInit();
	}
};

StaticInit _static_init;
*/

void glutInitMac(int *argcp, char ***argv)
{
	double unused;
	int i;
	char *geometry = NULL;

	__glutInit();

	if (__glutInitialModifiers & cmdKey) // if command key is down at start
	{
#if defined(__MWERKS__) || defined(__SC__)
		*argcp = ccommand(argv);
#endif
	}
	else if (*argcp == 0)
	{
		*argcp = 1;
		*argv = (char **)malloc((*argcp + 1) * sizeof(char *));
		(*argv)[0] = strdup("GLUT Program");
		(*argv)[1] = NULL;
	}

	/* Make private copy of command line arguments. */
	__glutArgc = *argcp;
	if (__glutArgc > 0)
	{
		__glutArgv = (char **)malloc(__glutArgc * sizeof(char *));
		if (!__glutArgv)
			__glutFatalError("out of memory.");

		for (i = 0; i < __glutArgc; i++)
		{
			__glutArgv[i] = strdup((*argv)[i]);
			if (!__glutArgv[i])
				__glutFatalError("out of memory.");
		}
	}
	else
	{
		__glutArgc = 1;
		__glutArgv = (char **)malloc(__glutArgc * sizeof(char *));
		__glutArgv[0] = strdup("GLUT Program");
	}

	/* determine permanent program name */
	__glutProgramName = __glutArgv[0];

#if 0
	DoInitConsole();
	__glutBuildMenuBar();
#endif

	/* parse arguments for standard options */
	for (i = 1; i < __glutArgc; i++)
	{
		if (!strcmp(__glutArgv[i], "-display"))
		{
#if defined(macintosh)
			__glutWarning("-display option invalid for macintosh glut.");
#endif
			if (++i >= __glutArgc)
			{
				__glutFatalError(
					"follow -display option with X display name.");
			}
			//			display = __glutArgv[i];
			removeArgs(argcp, &(*argv)[1], 2);
		}
		else if (!strcmp(__glutArgv[i], "-geometry"))
		{
			if (++i >= __glutArgc)
			{
				__glutFatalError(
					"follow -geometry option with geometry parameter.");
			}
			geometry = __glutArgv[i];
			removeArgs(argcp, &(*argv)[1], 2);
		}
		else if (!strcmp(__glutArgv[i], "-direct"))
		{
#if defined(macintosh)
			__glutWarning("-direct option invalid for macintosh glut.");
#endif
			//			if (!__glutTryDirect)
			//				__glutFatalError("cannot force both direct and indirect rendering.");
			//			__glutForceDirect = GL_TRUE;
			removeArgs(argcp, &(*argv)[1], 1);
		}
		else if (!strcmp(__glutArgv[i], "-indirect"))
		{
#if defined(macintosh)
			__glutWarning("-indirect option invalid for macintosh glut.");
#endif
			//			if (__glutForceDirect)
			//				__glutFatalError("cannot force both direct and indirect rendering.");
			//			__glutTryDirect = GL_FALSE;
			removeArgs(argcp, &(*argv)[1], 1);
		}
		else if (!strcmp(__glutArgv[i], "-iconic"))
		{
#if defined(macintosh)
			__glutWarning("-indirect option invalid for macintosh glut.");
#endif
			//			__glutIconic = GL_TRUE;
			removeArgs(argcp, &(*argv)[1], 1);
		}
		else if (!strcmp(__glutArgv[i], "-gldebug"))
		{
			__glutDebug = GL_TRUE;
			removeArgs(argcp, &(*argv)[1], 1);
		}
		else if (!strcmp(__glutArgv[i], "-sync"))
		{
#if defined(_WIN32)
			__glutWarning("-indirect option invalid for win32 glut.");
#endif
			//			synchronize = GL_TRUE;
			removeArgs(argcp, &(*argv)[1], 1);
		}
		else
		{
			/* Once unknown option encountered, stop command line
			processing. */
			break;
		}
	}
	if (geometry)
	{
		int flags, x = 0, y = 0, width = 0, height = 0;

		flags = XParseGeometry(geometry, &x, &y,
							   (unsigned int *)&width, (unsigned int *)&height);
		if (WidthValue & flags)
		{
			/* Careful because X does not allow zero or negative
			width windows */
			if (width > 0)
				__glutInitWidth = width;
		}
		if (HeightValue & flags)
		{
			/* Careful because X does not allow zero or negative
			height windows */
			if (height > 0)
				__glutInitHeight = height;
		}
		glutInitWindowSize(__glutInitWidth, __glutInitHeight);
		if (XValue & flags)
		{
			if (XNegative & flags)
				x = glutGet(GLUT_SCREEN_WIDTH) + x - __glutInitWidth;
			/* Play safe: reject negative X locations */
			if (x >= 0)
				__glutInitX = x;
		}
		if (YValue & flags)
		{
			if (YNegative & flags)
				y = glutGet(GLUT_SCREEN_HEIGHT) + y - __glutInitHeight;
			/* Play safe: reject negative Y locations */
			if (y >= 0)
				__glutInitY = y;
		}
		glutInitWindowPosition(__glutInitX, __glutInitY);
		/*
				int x, y, width, height;
				if (++i >= __glutArgc) {
					__glutFatalError("follow -geometry option with geometry parameter.");
				}
				// Fix bogus "{width|height} may be used before set" warning
				width  = 0;
				height = 0;

				XParseGeometry(__glutArgv[i], &x, &y, &width, &height);

				if(width > 0)  __glutInitWidth = width;
				if(height > 0) __glutInitHeight = height;

				glutInitWindowSize(__glutInitWidth, __glutInitHeight);

				if(x >= 0)
					__glutInitX = x;
				else
					__glutInitX = glutGet(GLUT_SCREEN_WIDTH) - __glutInitWidth - x;

				if(y >= 0)
					__glutInitY = y;
				else
					__glutInitY = glutGet(GLUT_SCREEN_HEIGHT) - __glutInitHeight - y;


				glutInitWindowPosition(__glutInitX, __glutInitY);
		*/
	}
	__glutInitTime(&unused);

	__glutInited = GL_TRUE;
}

void glutInitWindowPosition(int x, int y)
{
	__glutInitX = x;
	__glutInitY = y;
}

void glutInitWindowSize(int width, int height)
{
	__glutInitWidth = width;
	__glutInitHeight = height;
}

void glutInitDisplayMode(unsigned int mask)
{
	// ggs: ensure we are inited correctly
	__glutInit();
	__glutDisplayMode = mask;
}
