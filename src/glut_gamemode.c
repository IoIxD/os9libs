/*Quickdraw.h
	File:		glut_gamemode.c

	Contains:

	Written by:	Mark J. Kilgard
	Mac OS by:	John Stauffer and Geoff Stahl

	Copyright:	Copyright � Mark J. Kilgard, 1998.
				MacOS portions Copyright � Apple Computer, Inc., 1999

	Change History (most recent first):

	   <14+>     8/28/99    ggs
		<14>     7/16/99    ggs     Fixed numerous close down bugs, fixed width and height reversal
									in centering code
		<13>     7/16/99    ggs     fixed multi-monitor window centering support
		<12>     7/16/99    ggs     fixed error handling
		<11>     7/16/99    ggs     Updated error checking
		<10>     7/16/99    ggs     Fixed attributions and copyrights
		 <9>     7/16/99    ggs     Completely rewrote gamemode context support, should work
									smoothly and correctly.
		 <8>     6/28/99    ggs     added some additional full screen stuff
		 <7>     6/22/99    ggs     Fixed Drawsprocket Initialization and ensured page flipping was
									disabled
		 <6>     6/22/99    ggs     Added fade in at end of game initialization
		 <5>     5/28/99    GGS     Cleaned up function definitions, handled fades correctly
		 <4>     5/24/99    GGS     All DSp support is in
		 <3>     5/24/99    GGS     Added DSp display mode iteration that will be compatible  with
									current gamemode mode search routine.
		 <2>     5/23/99    GGS     fixed line endings problem
		 <1>     5/23/99    GGS     initial conversion from standard 3.7 version
*/

/* Copyright � Mark J. Kilgard, 1998. */
/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#if TARGET_API_MAC_CARBON
#define CALL_NOT_IN_CARBON 1 // ensure DrawSprocket calls are found (must weak link and runtime check)
#include <DrawSprocket.h>
#undef CALL_NOT_IN_CARBON
#else
#include <DrawSprocket.h>
#endif
#include <Quickdraw.h>
#include <Displays.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "glutint.h"

int __glutDisplaySettingsChanged = 0;
static DisplayMode gCurrentDM;
static int gInit = false;
GLUTwindow *__glutGameModeWindow = NULL;

#ifdef TEST
static char *compstr[] =
	{
		"none", "=", "!=", "<=", ">=", ">", "<", "~"};
static char *capstr[] =
	{
		"width", "height", "bpp", "hertz", "num"};
#endif

void __glutSuspendGameMode(void)
{
	OSErr err = noErr;
	if (!gInit) // should handle aglFullscreen here
		return;

	err = DSpContext_FadeGammaOut(NULL, NULL); // remove for debug
	if (noErr != err)
		__glutWarning("Unable to fade the display!");

	// pause our context
	if (gCurrentDM.refNum)
	{
		err = DSpContext_SetState(gCurrentDM.refNum, kDSpContextState_Paused);
		if (noErr != err)
			__glutFatalError("Unable to set the display!");
	}

	err = DSpContext_FadeGammaIn(NULL, NULL);
	if (noErr != err)
		__glutWarning("Unable to fade the display!");
}

void __glutResumeGameMode(void)
{
	OSErr err = noErr;

	if (!gInit) // should handle aglFullscreen here
		return;
	err = DSpContext_FadeGammaOut(NULL, NULL); // remove for debug
	if (noErr != err)
		__glutWarning("Unable to fade the display!");

	// activate our context
	if (gCurrentDM.refNum)
	{
		err = DSpContext_SetState(gCurrentDM.refNum, kDSpContextState_Active);
		if (noErr != err)
			__glutFatalError("Unable to set the display!");
	}

	err = DSpContext_FadeGammaIn(NULL, NULL);
	if (noErr != err)
		__glutWarning("Unable to fade the display!");
}

void __glutCloseDownGameMode(void)
{
	OSErr err;
	// when does this get called and what are the cases that __glutDisplaySettingsChanged is false
	if (__glutDisplaySettingsChanged)
	{
		__glutDisplaySettingsChanged = 0;

		/* Assumes that display settings have been changed, that is __glutDisplaySettingsChanged is true. */
		if (!gInit) // should handle aglFullscreen here
			return;
		if (gCurrentDM.refNum)
		{
			// already faded
			err = DSpContext_SetState(gCurrentDM.refNum, kDSpContextState_Inactive);
			if (err != noErr)
				__glutWarning("DSpContext_SetState error");

			DSpContext_FadeGammaIn(NULL, NULL);

			err = DSpContext_Release(gCurrentDM.refNum);
			if (err != noErr)
				__glutWarning("DSpContext_Release error");
			gCurrentDM.refNum = 0;
		}
	}
}

void glutLeaveGameMode(void)
{
	if (__glutGameModeWindow == NULL)
		return;
	if (!gInit) // should handle aglFullscreen here
		return;

	if (gCurrentDM.refNum)
		DSpContext_FadeGammaOut(gCurrentDM.refNum, NULL);
	if (__glutGameModeWindow)
		__glutDestroyWindow(__glutGameModeWindow, __glutGameModeWindow);
	__glutGameModeWindow = NULL;
	DSpShutdown();
	gInit = false;
}

static void
initGameModeSupport(void)
{
	OSErr err;
	if (__glutGameModeWindow)
		glutLeaveGameMode();

	if (gInit)
		return;

	gCurrentDM.refNum = 0;
	// startup DSp
	if ((Ptr)kUnresolvedCFragSymbolAddress == (Ptr)DSpStartup) // check to ensure DSp library is available
	{
		return;
		// no game mode without DSp
		// __glutFatalError("DrawSprocket not installed.");
	}
	err = DSpStartup(); // enter DSp
	if (err != noErr)
		__glutFatalError("DSpStartup error.");

	gInit = true;
	// don't need display mode list
}

static void GraphicsInitAttributes(DSpContextAttributes *inAttributes)
{
	memset(inAttributes, 0, sizeof(DSpContextAttributes));
}

/* This routine is based on similiar code in glut_dstr.c */
static DisplayMode findMatch(Criterion *criteria, int ncriteria)
{
	OSErr theError;
	DSpContextAttributes foundAttributes;
	OptionBits depthMask = kDSpDepthMask_All;
	short x;

	for (x = 0; x < ncriteria; x++)
		gCurrentDM.cap[criteria[x].capability] = criteria[x].value;

	GraphicsInitAttributes(&(gCurrentDM.attributes));
	gCurrentDM.attributes.displayWidth = gCurrentDM.cap[DM_WIDTH];
	if (gCurrentDM.attributes.displayWidth == 0)
#if TARGET_API_MAC_CARBON
	{
		BitMap bmTemp;
		gCurrentDM.attributes.displayWidth = abs(GetQDGlobalsScreenBits(&bmTemp)->bounds.right - GetQDGlobalsScreenBits(&bmTemp)->bounds.left);
	}
#else
		gCurrentDM.attributes.displayWidth = abs(qd.screenBits.bounds.right - qd.screenBits.bounds.left);
#endif

	gCurrentDM.attributes.displayHeight = gCurrentDM.cap[DM_HEIGHT];
	if (gCurrentDM.attributes.displayHeight == 0)
#if TARGET_API_MAC_CARBON
	{
		BitMap bmTemp;
		gCurrentDM.attributes.displayHeight = abs(GetQDGlobalsScreenBits(&bmTemp)->bounds.bottom - GetQDGlobalsScreenBits(&bmTemp)->bounds.top);
	}
#else
		gCurrentDM.attributes.displayHeight = abs(qd.screenBits.bounds.bottom - qd.screenBits.bounds.top);
#endif

	gCurrentDM.attributes.colorNeeds = kDSpColorNeeds_Require;
	if (gCurrentDM.cap[DM_PIXEL_DEPTH] == 1)
	{
		gCurrentDM.cap[DM_PIXEL_DEPTH] = 1;
		depthMask = kDSpDepthMask_1;
	}
	else if (gCurrentDM.cap[DM_PIXEL_DEPTH] == 2)
	{
		gCurrentDM.cap[DM_PIXEL_DEPTH] = 2;
		depthMask = kDSpDepthMask_2;
	}
	else if (gCurrentDM.cap[DM_PIXEL_DEPTH] <= 4)
	{
		gCurrentDM.cap[DM_PIXEL_DEPTH] = 4;
		depthMask = kDSpDepthMask_4;
	}
	else if (gCurrentDM.cap[DM_PIXEL_DEPTH] <= 8)
	{
		gCurrentDM.cap[DM_PIXEL_DEPTH] = 8;
		depthMask = kDSpDepthMask_8;
	}
	else if (gCurrentDM.cap[DM_PIXEL_DEPTH] <= 16)
	{
		gCurrentDM.cap[DM_PIXEL_DEPTH] = 16;
		depthMask = kDSpDepthMask_16;
	}
	else if (gCurrentDM.cap[DM_PIXEL_DEPTH] <= 32)
	{
		gCurrentDM.cap[DM_PIXEL_DEPTH] = 32;
		depthMask = kDSpDepthMask_32;
	}
	gCurrentDM.attributes.displayDepthMask = gCurrentDM.cap[DM_PIXEL_DEPTH];
	gCurrentDM.attributes.displayBestDepth = depthMask;
	gCurrentDM.attributes.backBufferDepthMask = gCurrentDM.cap[DM_PIXEL_DEPTH];
	gCurrentDM.attributes.backBufferBestDepth = depthMask;

	gCurrentDM.attributes.pageCount = 1; // only the front buffer is needed

	// if we are not using DSp this is all we need
	if (!gInit)
		return gCurrentDM;

	// will display user dialog if context selection is required otherwise as find best context
	theError = DSpUserSelectContext(&(gCurrentDM.attributes), 0L, nil, &(gCurrentDM.refNum));
	while ((theError) && (((gCurrentDM.cap[DM_WIDTH] > 640) && (gCurrentDM.cap[DM_HEIGHT] > 480)) || (gCurrentDM.cap[DM_PIXEL_DEPTH] > 8)))
	{
		// if a width was request
		if (gCurrentDM.cap[DM_WIDTH])
		{
			// try switching to lower resolution
			if ((gCurrentDM.cap[DM_WIDTH] > 1024) || (gCurrentDM.cap[DM_HEIGHT] > 768))
			{
				// try smaller frame
				// a similar ratio of H to W that will fit in 640 x 480
				float ratio = (float)gCurrentDM.cap[DM_HEIGHT] / (float)gCurrentDM.cap[DM_WIDTH];
				if (1024 * ratio <= 768) // fits using width
				{
					gCurrentDM.cap[DM_WIDTH] = 1024;
					gCurrentDM.cap[DM_HEIGHT] = 1024 * ratio;
				}
				else
				{
					gCurrentDM.cap[DM_WIDTH] = 768 / ratio;
					gCurrentDM.cap[DM_HEIGHT] = 768;
				}
			}
			else if ((gCurrentDM.cap[DM_WIDTH] > 640) || (gCurrentDM.cap[DM_HEIGHT] > 480))
			{
				// try smaller frame
				// a similar ratio of H to W that will fit in 640 x 480
				float ratio = (float)gCurrentDM.cap[DM_HEIGHT] / (float)gCurrentDM.cap[DM_WIDTH];
				if (640 * ratio <= 480) // fits using width
				{
					gCurrentDM.cap[DM_WIDTH] = 640;
					gCurrentDM.cap[DM_HEIGHT] = 640 * ratio;
				}
				else
				{
					gCurrentDM.cap[DM_WIDTH] = 480 / ratio;
					gCurrentDM.cap[DM_HEIGHT] = 480;
				}
			}
			else // try switching to lower bit depth
			{
				if (gCurrentDM.cap[DM_PIXEL_DEPTH] == 32)
				{
					gCurrentDM.cap[DM_PIXEL_DEPTH] = 16;
				}
				else
				{
					gCurrentDM.cap[DM_PIXEL_DEPTH] = 8; // last chance
				}
			}
		}
		else
		{
			// try 640 by 480 (will end up in above case next)
			gCurrentDM.cap[DM_WIDTH] = 640;
			gCurrentDM.cap[DM_HEIGHT] = 480;
		}
		GraphicsInitAttributes(&(gCurrentDM.attributes));
		gCurrentDM.attributes.displayWidth = gCurrentDM.cap[DM_WIDTH];
		if (gCurrentDM.attributes.displayWidth == 0)
#if TARGET_API_MAC_CARBON
		{
			BitMap bmTemp;
			gCurrentDM.attributes.displayWidth = abs(GetQDGlobalsScreenBits(&bmTemp)->bounds.right - GetQDGlobalsScreenBits(&bmTemp)->bounds.left);
		}
#else
			gCurrentDM.attributes.displayWidth = abs(qd.screenBits.bounds.right - qd.screenBits.bounds.left);
#endif
		gCurrentDM.attributes.displayHeight = gCurrentDM.cap[DM_HEIGHT];
		if (gCurrentDM.attributes.displayHeight == 0)
#if TARGET_API_MAC_CARBON
		{
			BitMap bmTemp;
			gCurrentDM.attributes.displayHeight = abs(GetQDGlobalsScreenBits(&bmTemp)->bounds.bottom - GetQDGlobalsScreenBits(&bmTemp)->bounds.top);
		}
#else
			gCurrentDM.attributes.displayHeight = abs(qd.screenBits.bounds.bottom - qd.screenBits.bounds.top);
#endif
		gCurrentDM.attributes.colorNeeds = kDSpColorNeeds_Require;
		if (gCurrentDM.cap[DM_PIXEL_DEPTH] == 1)
		{
			gCurrentDM.cap[DM_PIXEL_DEPTH] = 1;
			depthMask = kDSpDepthMask_1;
		}
		else if (gCurrentDM.cap[DM_PIXEL_DEPTH] == 2)
		{
			gCurrentDM.cap[DM_PIXEL_DEPTH] = 2;
			depthMask = kDSpDepthMask_2;
		}
		else if (gCurrentDM.cap[DM_PIXEL_DEPTH] <= 4)
		{
			gCurrentDM.cap[DM_PIXEL_DEPTH] = 4;
			depthMask = kDSpDepthMask_4;
		}
		else if (gCurrentDM.cap[DM_PIXEL_DEPTH] <= 8)
		{
			gCurrentDM.cap[DM_PIXEL_DEPTH] = 8;
			depthMask = kDSpDepthMask_8;
		}
		else if (gCurrentDM.cap[DM_PIXEL_DEPTH] <= 16)
		{
			gCurrentDM.cap[DM_PIXEL_DEPTH] = 16;
			depthMask = kDSpDepthMask_16;
		}
		else if (gCurrentDM.cap[DM_PIXEL_DEPTH] <= 32)
		{
			gCurrentDM.cap[DM_PIXEL_DEPTH] = 32;
			depthMask = kDSpDepthMask_32;
		}
		gCurrentDM.attributes.displayDepthMask = gCurrentDM.cap[DM_PIXEL_DEPTH];
		gCurrentDM.attributes.displayBestDepth = depthMask;
		gCurrentDM.attributes.backBufferDepthMask = gCurrentDM.cap[DM_PIXEL_DEPTH];
		gCurrentDM.attributes.backBufferBestDepth = depthMask;
		gCurrentDM.attributes.pageCount = 1; // only the front buffer is needed

		// will display user dialog if context selection is required otherwise as find best context
		theError = DSpUserSelectContext(&(gCurrentDM.attributes), 0L, nil, &(gCurrentDM.refNum));
	}
	if (theError)
		__glutFatalError("Unable to find context.");
	// see what we actually found
	theError = DSpContext_GetAttributes((gCurrentDM.refNum), &foundAttributes);
	if (theError)
		__glutFatalError("Cannot get found contexts attributes.");

	// reset width and height to full screen and handle our own centering
	// HWA will not correctly center less than full screen size contexts
	gCurrentDM.attributes.displayWidth = foundAttributes.displayWidth;
	gCurrentDM.attributes.displayHeight = foundAttributes.displayHeight;
	gCurrentDM.attributes.pageCount = 1;									  // only the front buffer is needed
	gCurrentDM.attributes.contextOptions = 0 | kDSpContextOption_DontSyncVBL; // no page flipping
	gCurrentDM.attributes.frequency = foundAttributes.frequency;
	if (gCurrentDM.attributes.frequency > (85 << 16)) // ensure the DM does not give use a really freaky refresh rate back
		gCurrentDM.attributes.frequency = (60 << 16);

	DSpSetBlankingColor(&rgbBlack);

	return gCurrentDM;
}

/**
 * Parses strings in the form of:
 *  800x600
 *  800x600:16
 *  800x600@60
 *  800x600:16@60
 *  @60
 *  :16
 *  :16@60
 * NOTE that @ before : is not parsed.
 */
static int
specialCaseParse(char *word, Criterion *criterion, int mask)
{
	char *xstr, *response;
	int got;
	int width, height, bpp, hertz;
	switch (word[0])
	{
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		/* The WWWxHHH case. */
		if (mask & (1 << DM_WIDTH))
		{
			return -1;
		}
		xstr = strpbrk(&word[1], "x");
		if (xstr)
		{
			width = (int)strtol(word, &response, 0);
			if (response == word || response[0] != 'x')
			{
				/* Not a valid number OR needs to be followed by 'x'. */
				return -1;
			}
			height = (int)strtol(&xstr[1], &response, 0);
			if (response == &xstr[1])
			{
				/* Not a valid number. */
				return -1;
			}
			criterion[0].capability = DM_WIDTH;
			criterion[0].comparison = EQ;
			criterion[0].value = width;
			criterion[1].capability = DM_HEIGHT;
			criterion[1].comparison = EQ;
			criterion[1].value = height;
			got = specialCaseParse(response,
								   &criterion[2], 1 << DM_WIDTH);
			if (got >= 0)
			{
				return got + 2;
			}
			else
			{
				return -1;
			}
		}
		return -1;
	case ':':
		/* The :BPP case. */
		if (mask & (1 << DM_PIXEL_DEPTH))
		{
			return -1;
		}
		bpp = (int)strtol(&word[1], &response, 0);
		if (response == &word[1])
		{
			/* Not a valid number. */
			return -1;
		}
		criterion[0].capability = DM_PIXEL_DEPTH;
		criterion[0].comparison = EQ;
		criterion[0].value = bpp;
		got = specialCaseParse(response,
							   &criterion[1], 1 << DM_WIDTH | 1 << DM_PIXEL_DEPTH);
		if (got >= 0)
		{
			return got + 1;
		}
		else
		{
			return -1;
		}
	case '@':
		/* The @HZ case. */
		if (mask & (1 << DM_HERTZ))
		{
			return -1;
		}
		hertz = (int)strtol(&word[1], &response, 0);
		if (response == &word[1])
		{
			/* Not a valid number. */
			return -1;
		}
		criterion[0].capability = DM_HERTZ;
		criterion[0].comparison = EQ;
		criterion[0].value = hertz;
		got = specialCaseParse(response,
							   &criterion[1], ~DM_HERTZ);
		if (got >= 0)
		{
			return got + 1;
		}
		else
		{
			return -1;
		}
	case '\0':
		return 0;
	}
	return -1;
}

/* This routine is based on similiar code in glut_dstr.c */
static int
parseCriteria(char *word, Criterion *criterion)
{
	char *cstr, *vstr, *response;
	int comparator, value;
	cstr = strpbrk(word, "=><!~");
	if (cstr)
	{
		switch (cstr[0])
		{
		case '=':
			comparator = EQ;
			vstr = &cstr[1];
			break;
		case '~':
			comparator = MIN;
			vstr = &cstr[1];
			break;
		case '>':
			if (cstr[1] == '=')
			{
				comparator = GTE;
				vstr = &cstr[2];
			}
			else
			{
				comparator = GT;
				vstr = &cstr[1];
			}
			break;
		case '<':
			if (cstr[1] == '=')
			{
				comparator = LTE;
				vstr = &cstr[2];
			}
			else
			{
				comparator = LT;
				vstr = &cstr[1];
			}
			break;
		case '!':
			if (cstr[1] == '=')
			{
				comparator = NEQ;
				vstr = &cstr[2];
			}
			else
			{
				return -1;
			}
			break;
		default:
			return -1;
		}
		value = (int)strtol(vstr, &response, 0);
		if (response == vstr)
		{
			/* Not a valid number. */
			return -1;
		}
		*cstr = '\0';
	}
	else
	{
		comparator = NONE;
	}
	switch (word[0])
	{
	case 'b':
		if (!strcmp(word, "bpp"))
		{
			criterion[0].capability = DM_PIXEL_DEPTH;
			if (comparator == NONE)
			{
				return -1;
			}
			else
			{
				criterion[0].comparison = comparator;
				criterion[0].value = value;
				return 1;
			}
		}
		return -1;
	case 'h':
		if (!strcmp(word, "height"))
		{
			criterion[0].capability = DM_HEIGHT;
			if (comparator == NONE)
			{
				return -1;
			}
			else
			{
				criterion[0].comparison = comparator;
				criterion[0].value = value;
				return 1;
			}
		}
		if (!strcmp(word, "hertz"))
		{
			criterion[0].capability = DM_HERTZ;
			if (comparator == NONE)
			{
				return -1;
			}
			else
			{
				criterion[0].comparison = comparator;
				criterion[0].value = value;
				return 1;
			}
		}
		return -1;
	case 'n':
		if (!strcmp(word, "num"))
		{
			criterion[0].capability = DM_NUM;
			if (comparator == NONE)
			{
				return -1;
			}
			else
			{
				criterion[0].comparison = comparator;
				criterion[0].value = value;
				return 1;
			}
		}
		return -1;
	case 'w':
		if (!strcmp(word, "width"))
		{
			criterion[0].capability = DM_WIDTH;
			if (comparator == NONE)
			{
				return -1;
			}
			else
			{
				criterion[0].comparison = comparator;
				criterion[0].value = value;
				return 1;
			}
		}
		return -1;
	}
	if (comparator == NONE)
	{
		return specialCaseParse(word, criterion, 0);
	}
	return -1;
}

/* This routine is based on similiar code in glut_dstr.c */
static Criterion *
parseDisplayString(const char *display, int *ncriteria)
{
	Criterion *criteria = NULL;
	int n, parsed;
	char *copy, *word;
	copy = __glutStrdup(display);
	/* Attempt to estimate how many criteria entries should be
	   needed. */
	n = 0;
	word = strtok(copy, " \t");
	while (word)
	{
		n++;
		word = strtok(NULL, " \t");
	}
	/* Allocate number of words of criteria.  A word
	   could contain as many as four criteria in the
	   worst case.  Example: 800x600:16@60 */
	criteria = (Criterion *)malloc(4 * n * sizeof(Criterion));
	if (!criteria)
	{
		__glutFatalError("out of memory.");
	}
	/* Re-copy the copy of the display string. */
	strcpy(copy, display);
	n = 0;
	word = strtok(copy, " \t");
	while (word)
	{
		parsed = parseCriteria(word, &criteria[n]);
		if (parsed >= 0)
		{
			n += parsed;
		}
		else
		{
			__glutWarning("Unrecognized game mode string word: %s (ignoring)\n", word);
		}
		word = strtok(NULL, " \t");
	}
	free(copy);
	*ncriteria = n;
	return criteria;
}

void glutGameModeString(const char *string)
{
	Criterion *criteria;
	int ncriteria;
	initGameModeSupport();
	criteria = parseDisplayString(string, &ncriteria);
	gCurrentDM = findMatch(criteria, ncriteria);
	free(criteria);
}

int glutEnterGameMode(void)
{
	GLUTwindow *window;
	int width, height;
	CWindowPtr win;
	OSErr err;
	DSpContextState theState;
	GDHandle hGD;
	Rect rectWin;
	DisplayIDType displayID;
	GrafPtr pGrafSave = NULL;
	CGrafPtr pCGrafNew = NULL;

	if (!gInit)
		return -1;
	//  if (cur_active_menu) {
	//    __glutFatalUsage("entering game mode not allowed while menus in use");
	//  }

	if (__glutGameModeWindow)
	{
		/* Already in game mode, so blow away game mode
		   window so apps can change resolutions. */
		window = __glutGameModeWindow;
		/* Setting the game mode window to NULL tricks
		   the window destroy code into not undoing the
		   screen display change since we plan on immediately
		   doing another mode change. */
		__glutGameModeWindow = NULL;
		__glutDestroyWindow(window, window);
	}
	/* Assume default screen size until we find out if we
	   can actually change the display settings. */
#if TARGET_API_MAC_CARBON
	{
		BitMap bmTemp;
		width = abs(GetQDGlobalsScreenBits(&bmTemp)->bounds.right - GetQDGlobalsScreenBits(&bmTemp)->bounds.left);
		height = abs(GetQDGlobalsScreenBits(&bmTemp)->bounds.bottom - GetQDGlobalsScreenBits(&bmTemp)->bounds.top);
	}
#else
	width = abs(qd.screenBits.bounds.right - qd.screenBits.bounds.left);
	height = abs(qd.screenBits.bounds.bottom - qd.screenBits.bounds.top);
#endif
	// change display mode
	if (gCurrentDM.refNum)
	{
		static int registered = 0;

		// change display res to currentDM
		// get our device for future use
		err = DSpContext_GetDisplayID(gCurrentDM.refNum, &displayID);
		if (err)
			__glutFatalError("DSpContext_GetDisplayID() had an error.");

		// get GDHandle for ID'd device
		err = DMGetGDeviceByDisplayID(displayID, &hGD, false);
		if (err)
			__glutFatalError("DMGetGDeviceByDisplayID() had an error.");

		err = DSpContext_Reserve(gCurrentDM.refNum, &(gCurrentDM.attributes));
		if (err != noErr)
			__glutFatalError("DSpContext_Reserve error.");
		else
		{
			// what if we need to confirm switch (need a dialog)
			DSpContext_FadeGammaOut(NULL, NULL);
			// set to active
			err = noErr;
			DSpContext_GetState(gCurrentDM.refNum, &theState);
			if (theState != kDSpContextState_Active)
				err = DSpContext_SetState(gCurrentDM.refNum, kDSpContextState_Active);
			if ((err) & (err != kDSpConfirmSwitchWarning))
			{
				DSpContext_FadeGammaIn(NULL, NULL);
				DSpContext_Release(gCurrentDM.refNum);
				__glutFatalError("pDSpContext_SetState error.");
			}
		}

		// center window in our context's gdevice
		rectWin.top = (**hGD).gdRect.top + ((**hGD).gdRect.bottom - (**hGD).gdRect.top) / 2; // h center
		rectWin.top -= gCurrentDM.cap[DM_HEIGHT] / 2;
		rectWin.left = (**hGD).gdRect.left + ((**hGD).gdRect.right - (**hGD).gdRect.left) / 2; // v center
		rectWin.left -= gCurrentDM.cap[DM_WIDTH] / 2;
		rectWin.right = rectWin.left + gCurrentDM.cap[DM_WIDTH];
		rectWin.bottom = rectWin.top + gCurrentDM.cap[DM_HEIGHT];

		__glutDisplaySettingsChanged = 1;
		width = gCurrentDM.cap[DM_WIDTH];
		height = gCurrentDM.cap[DM_HEIGHT];

		if (!registered) // ensure things are closed down properly
		{
			atexit(glutLeaveGameMode);
			registered = 1;
		}
	}

	window = __glutCreateWindow(NULL, "Game Window", rectWin.left, rectWin.top, width, height, /* game mode */ kGameMode);
	win = window->win;
	/* Schedule the fullscreen property to be added and to
	   make sure the window is configured right.  Win32
	   doesn't need this. */
	//  window->desiredX = 0;
	//  window->desiredY = 0;
	//  window->desiredWidth = width;
	//  window->desiredHeight = height;
	//  window->desiredConfMask |= CWX | CWY | CWWidth | CWHeight;
	// #ifdef (_WIN32)
	/* Win32 does not want to use GLUT_FULL_SCREEN_WORK
	   for game mode because we need to be maximizing
	   the window in game mode, not just sizing it to
	   take up the full screen.  The Win32-ness of game
	   mode happens when you pass 1 in the gameMode parameter
	   to __glutCreateWindow above.  A gameMode of creates
	   a WS_POPUP window, not a standard WS_OVERLAPPEDWINDOW
	   window.  WS_POPUP ensures the taskbar is hidden. */
	//  __glutPutOnWorkList(window,
	//   GLUT_CONFIGURE_WORK);
	// #else
	__glutPutOnWorkList(window,
						GLUT_CONFIGURE_WORK | GLUT_FULL_SCREEN_WORK);
	// #endif
	__glutGameModeWindow = window;
	DSpContext_FadeGammaIn(NULL, NULL);
	ShowCursor(); // ensure cursor is visible
	return window->num + 1;
}

int glutGameModeGet(GLenum mode)
{
	switch (mode)
	{
	case GLUT_GAME_MODE_ACTIVE:
		return __glutGameModeWindow != NULL;
	case GLUT_GAME_MODE_POSSIBLE:
		return gCurrentDM.refNum != 0;
	case GLUT_GAME_MODE_WIDTH:
		return gCurrentDM.refNum ? gCurrentDM.cap[DM_WIDTH] : -1;
	case GLUT_GAME_MODE_HEIGHT:
		return gCurrentDM.refNum ? gCurrentDM.cap[DM_HEIGHT] : -1;
	case GLUT_GAME_MODE_PIXEL_DEPTH:
		return gCurrentDM.refNum ? gCurrentDM.cap[DM_PIXEL_DEPTH] : -1;
	case GLUT_GAME_MODE_REFRESH_RATE:
		return gCurrentDM.refNum ? gCurrentDM.cap[DM_HERTZ] : -1;
	case GLUT_GAME_MODE_DISPLAY_CHANGED:
		return __glutDisplaySettingsChanged;
	default:
		return -1;
	}
}
