/*Timer.h
  File:		glutint.h

  Contains:

  Written by:	Mark J. Kilgard
  Mac OS by:	John Stauffer and Geoff Stahl

  Copyright:	Copyright � Mark J. Kilgard, 1994 - 1999
        MacOS portions Copyright � Apple Computer, Inc., 1999

  Change History (most recent first):

       <10+>     8/28/99    ggs
        <10>     8/28/99    ggs     Fixed headers
         <9>     7/16/99    ggs     Fixed attributions and copyrights
         <8>     6/22/99    ggs     Added contextual menus (still need to get rid of help if
                                    possible and add error checking)
         <7>     6/22/99    ggs     Add rgbWhite and rgbBlack static const RGBColors
         <6>     5/31/99    ggs     removed reference in Mac OS macro to wgl (oops) and made
                                    wglMakeCurrent = aglSetCurrentContxt only
         <5>     5/31/99    ggs     added additional defintions for mac types of work
         <3>     5/31/99    ggs     Brought up to date with glut 3.7 and previosu Mac OS glut.
                                    Should be compatible with both.
         <2>     5/31/99    ggs     Fixed line spacing
         <1>     5/31/99    ggs
*/

#ifndef __glutint_h__
#define __glutint_h__

/* Copyright � Mark J. Kilgard, 1994, 1997, 1998. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#ifdef macintosh
#include <Timer.h>
#include <MacTypes.h>
#include <DrawSprocket.h> // for game mode support

#include <agl.h>
#include <GL/gl.h>
#include <GL/glut.h>

typedef int Bool;

// Mac OS defs

/* Windows: */
#define kMainWindow zoomDocProc
#define kPlainWindow plainDBox
#define kGameMode -1 /* window proc ID, just an indicator of the mode \
/* Dilogs: */
#define kAboutDialog 128
/* Apple menu: */
#define mApple 128
#define iAbout 1
/* File menu: */
#define mFile 129
#define iQuit 1
/* Mouse menu items */
#define mMouseMenuStart 131

#define ADD_TIME(dest, src1, src2) \
  {                                \
    dest = src1 + src2;            \
  }

#define TIMEDELTA(dest, src1, src2) \
  {                                 \
    dest = src1 - src2;             \
  }

#define IS_AFTER(t1, t2) ((t2) > (t1))
#define IS_AT_OR_AFTER(t1, t2) ((t2) >= (t1))

static const RGBColor rgbBlack = {0x0000, 0x0000, 0x0000};
static const RGBColor rgbWhite = {0xFFFF, 0xFFFF, 0xFFFF};

#else /* !macintosh */
#if defined(__CYGWIN32__)
#include <sys/time.h>
#endif

#if defined(_WIN32)
#include "glutwin32.h"
#else
#ifdef __sgi
#define SUPPORT_FORTRAN
#endif
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>
#endif

#include <GL/glut.h>

/* Non-Win32 platforms need APIENTRY defined to nothing
   because all the GLUT routines have the APIENTRY prefix
   to make Win32 happy. */
#ifndef APIENTRY
#define APIENTRY
#endif

#ifdef __vms
#if (__VMS_VER < 70000000)
struct timeval
{
  __int64 val;
};
extern int sys$gettim(struct timeval *);
#else
#include <time.h>
#endif
#else
#include <sys/types.h>
#if !defined(_WIN32)
#include <sys/time.h>
#else
#include <winsock.h>
#endif
#endif
#if defined(__vms) && (__VMS_VER < 70000000)

/* For VMS6.2 or lower :
   One TICK on VMS is 100 nanoseconds; 0.1 microseconds or
   0.0001 milliseconds. This means that there are 0.01
   ticks/ns, 10 ticks/us, 10,000 ticks/ms and 10,000,000
   ticks/second. */

#define TICKS_PER_MILLISECOND 10000
#define TICKS_PER_SECOND 10000000

#define GETTIMEOFDAY(_x) (void)sys$gettim(_x);

#define ADD_TIME(dest, src1, src2)        \
  {                                       \
    (dest).val = (src1).val + (src2).val; \
  }

#define TIMEDELTA(dest, src1, src2)       \
  {                                       \
    (dest).val = (src1).val - (src2).val; \
  }

#define IS_AFTER(t1, t2) ((t2).val > (t1).val)

#define IS_AT_OR_AFTER(t1, t2) ((t2).val >= (t1).val)

#else
#if defined(SVR4) && !defined(sun) /* Sun claims SVR4, but \
                                      wants 2 args. */
#define GETTIMEOFDAY(_x) gettimeofday(_x)
#else
#define GETTIMEOFDAY(_x) gettimeofday(_x, NULL)
#endif
#define ADD_TIME(dest, src1, src2)                        \
  {                                                       \
    if (((dest).tv_usec =                                 \
             (src1).tv_usec + (src2).tv_usec) >= 1000000) \
    {                                                     \
      (dest).tv_usec -= 1000000;                          \
      (dest).tv_sec = (src1).tv_sec + (src2).tv_sec + 1;  \
    }                                                     \
    else                                                  \
    {                                                     \
      (dest).tv_sec = (src1).tv_sec + (src2).tv_sec;      \
      if (((dest).tv_sec >= 1) && (((dest).tv_usec < 0))) \
      {                                                   \
        (dest).tv_sec--;                                  \
        (dest).tv_usec += 1000000;                        \
      }                                                   \
    }                                                     \
  }
#define TIMEDELTA(dest, src1, src2)                             \
  {                                                             \
    if (((dest).tv_usec = (src1).tv_usec - (src2).tv_usec) < 0) \
    {                                                           \
      (dest).tv_usec += 1000000;                                \
      (dest).tv_sec = (src1).tv_sec - (src2).tv_sec - 1;        \
    }                                                           \
    else                                                        \
    {                                                           \
      (dest).tv_sec = (src1).tv_sec - (src2).tv_sec;            \
    }                                                           \
  }
#define IS_AFTER(t1, t2)            \
  (((t2).tv_sec > (t1).tv_sec) ||   \
   (((t2).tv_sec == (t1).tv_sec) && \
    ((t2).tv_usec > (t1).tv_usec)))
#define IS_AT_OR_AFTER(t1, t2)      \
  (((t2).tv_sec > (t1).tv_sec) ||   \
   (((t2).tv_sec == (t1).tv_sec) && \
    ((t2).tv_usec >= (t1).tv_usec)))
#endif
#endif /* macintosh */

#define IGNORE_IN_GAME_MODE() \
  {                           \
    if (__glutGameModeWindow) \
      return;                 \
  }

#define GLUT_WIND_IS_RGB(x) (((x) & GLUT_INDEX) == 0)
#define GLUT_WIND_IS_INDEX(x) (((x) & GLUT_INDEX) != 0)
#define GLUT_WIND_IS_SINGLE(x) (((x) & GLUT_DOUBLE) == 0)
#define GLUT_WIND_IS_DOUBLE(x) (((x) & GLUT_DOUBLE) != 0)
#define GLUT_WIND_HAS_ACCUM(x) (((x) & GLUT_ACCUM) != 0)
#define GLUT_WIND_HAS_ALPHA(x) (((x) & GLUT_ALPHA) != 0)
#define GLUT_WIND_HAS_DEPTH(x) (((x) & GLUT_DEPTH) != 0)
#define GLUT_WIND_HAS_STENCIL(x) (((x) & GLUT_STENCIL) != 0)
#define GLUT_WIND_IS_MULTISAMPLE(x) (((x) & GLUT_MULTISAMPLE) != 0)
#define GLUT_WIND_IS_STEREO(x) (((x) & GLUT_STEREO) != 0)
#define GLUT_WIND_IS_LUMINANCE(x) (((x) & GLUT_LUMINANCE) != 0)
#ifdef macintosh
#define GLUT_NO_WORK 0x00
#endif /* macintosh */
#define GLUT_MAP_WORK (1 << 0)
#define GLUT_EVENT_MASK_WORK (1 << 1)
#define GLUT_REDISPLAY_WORK (1 << 2)
#define GLUT_CONFIGURE_WORK (1 << 3)
#define GLUT_COLORMAP_WORK (1 << 4)
#define GLUT_DEVICE_MASK_WORK (1 << 5)
#define GLUT_FINISH_WORK (1 << 6)
#define GLUT_DEBUG_WORK (1 << 7)
#define GLUT_DUMMY_WORK (1 << 8)
#define GLUT_FULL_SCREEN_WORK (1 << 9)
#define GLUT_OVERLAY_REDISPLAY_WORK (1 << 10)
#define GLUT_REPAIR_WORK (1 << 11)
#define GLUT_OVERLAY_REPAIR_WORK (1 << 12)
#ifdef macintosh
#define GLUT_BUTTON_WORK (1 << 13)
#define GLUT_PASSIVE_WORK (1 << 14)
#define GLUT_ENTRY_WORK (1 << 15)
#endif /* macintosh */

/* Frame buffer capability macros and types. */
#define RGBA 0
#define BUFFER_SIZE 1
#define DOUBLEBUFFER 2
#define STEREO 3
#define AUX_BUFFERS 4
#define RED_SIZE 5 /* Used as mask bit for \
                      "color selected". */
#define GREEN_SIZE 6
#define BLUE_SIZE 7
#define ALPHA_SIZE 8
#define DEPTH_SIZE 9
#define STENCIL_SIZE 10
#define ACCUM_RED_SIZE 11 /* Used as mask bit for \
                             "acc selected". */
#define ACCUM_GREEN_SIZE 12
#define ACCUM_BLUE_SIZE 13
#define ACCUM_ALPHA_SIZE 14
#define LEVEL 15

#define NUM_GLXCAPS (LEVEL + 1)

#define XVISUAL (NUM_GLXCAPS + 0)
#define TRANSPARENT (NUM_GLXCAPS + 1)
#define SAMPLES (NUM_GLXCAPS + 2)
#define XSTATICGRAY (NUM_GLXCAPS + 3) /* Used as     \
                                         mask bit    \
                                         for "any    \
                                         visual type \
                                         selected". */
#define XGRAYSCALE (NUM_GLXCAPS + 4)
#define XSTATICCOLOR (NUM_GLXCAPS + 5)
#define XPSEUDOCOLOR (NUM_GLXCAPS + 6)
#define XTRUECOLOR (NUM_GLXCAPS + 7)
#define XDIRECTCOLOR (NUM_GLXCAPS + 8)
#define SLOW (NUM_GLXCAPS + 9)
#define CONFORMANT (NUM_GLXCAPS + 10)

#define NUM_CAPS (NUM_GLXCAPS + 11)

/* Frame buffer capablities that don't have a corresponding
   FrameBufferMode entry.  These get used as mask bits. */
#define NUM (NUM_CAPS + 0)
#define RGBA_MODE (NUM_CAPS + 1)
#define CI_MODE (NUM_CAPS + 2)
#define LUMINANCE_MODE (NUM_CAPS + 3)

#define NONE 0
#define EQ 1
#define NEQ 2
#define LTE 3
#define GTE 4
#define GT 5
#define LT 6
#define MIN 7

typedef struct _Criterion
{
  int capability;
  int comparison;
  int value;
} Criterion;

typedef struct _FrameBufferMode
{
#ifndef macintosh
  XVisualInfo *vi;
#if defined(GLX_VERSION_1_1) && defined(GLX_SGIX_fbconfig)

  /* fbc is non-NULL when the XVisualInfo* is not OpenGL-capable
     (ie, GLX_USE_GL is false), but the SGIX_fbconfig extension shows
     the visual's fbconfig is OpenGL-capable.  The reason for this is typically
     an RGBA luminance fbconfig such as 16-bit StaticGray that could
     not be advertised as a GLX visual since StaticGray visuals are
     required (by the GLX specification) to be color index.  The
     SGIX_fbconfig allows StaticGray visuals to instead advertised as
     fbconfigs that can provide RGBA luminance support. */

  GLXFBConfigSGIX fbc;
#endif
#endif /* macintosh */
  int valid;
  int cap[NUM_CAPS];
} FrameBufferMode;

/* DisplayMode capability macros for game mode. */
#define DM_WIDTH 0       /* "width" */
#define DM_HEIGHT 1      /* "height" */
#define DM_PIXEL_DEPTH 2 /* "bpp" (bits per pixel) */
#define DM_HERTZ 3       /* "hertz" */
#define DM_NUM 4         /* "num" */

#define NUM_DM_CAPS (DM_NUM + 1)

typedef struct _DisplayMode
{
#ifdef macintosh
  DSpContextAttributes attributes; // GGS: the screen mode attributes
  DSpContextReference refNum;      // GGS: ref num for screen mode describe above
  // translate into below to ensure perform same as other versions

#else
#ifdef _WIN32
  DEVMODE devmode;
#else
  /* XXX The X Window System does not have a standard
     mechanism for display setting changes.  On SGI
     systems, GLUT could use the XSGIvc (SGI X video
     control extension).  Perhaps this can be done in
     a future release of GLUT. */
#endif
#endif /* macintosh */
  int valid;
  int cap[NUM_DM_CAPS];
} DisplayMode;

/* GLUT  function types */
typedef void (*GLUTdisplayCB)(void);
typedef void (*GLUTreshapeCB)(int, int);
typedef void (*GLUTkeyboardCB)(unsigned char, int, int);
typedef void (*GLUTmouseCB)(int, int, int, int);
typedef void (*GLUTmotionCB)(int, int);
typedef void (*GLUTpassiveCB)(int, int);
typedef void (*GLUTentryCB)(int);
typedef void (*GLUTvisibilityCB)(int);
typedef void (*GLUTwindowStatusCB)(int);
typedef void (*GLUTidleCB)(void);
typedef void (*GLUTtimerCB)(int);
typedef void (*GLUTmenuStateCB)(int); /* DEPRICATED. */
typedef void (*GLUTmenuStatusCB)(int, int, int);
typedef void (*GLUTselectCB)(int);
typedef void (*GLUTspecialCB)(int, int, int);
typedef void (*GLUTspaceMotionCB)(int, int, int);
typedef void (*GLUTspaceRotateCB)(int, int, int);
typedef void (*GLUTspaceButtonCB)(int, int);
typedef void (*GLUTdialsCB)(int, int);
typedef void (*GLUTbuttonBoxCB)(int, int);
typedef void (*GLUTtabletMotionCB)(int, int);
typedef void (*GLUTtabletButtonCB)(int, int, int, int);
typedef void (*GLUTjoystickCB)(unsigned int buttonMask, int x, int y, int z);
#ifdef SUPPORT_FORTRAN
typedef void (*GLUTdisplayFCB)(void);
typedef void (*GLUTreshapeFCB)(int *, int *);
/* NOTE the pressed key is int, not unsigned char for Fortran! */
typedef void (*GLUTkeyboardFCB)(int *, int *, int *);
typedef void (*GLUTmouseFCB)(int *, int *, int *, int *);
typedef void (*GLUTmotionFCB)(int *, int *);
typedef void (*GLUTpassiveFCB)(int *, int *);
typedef void (*GLUTentryFCB)(int *);
typedef void (*GLUTvisibilityFCB)(int *);
typedef void (*GLUTwindowStatusFCB)(int *);
typedef void (*GLUTidleFCB)(void);
typedef void (*GLUTtimerFCB)(int *);
typedef void (*GLUTmenuStateFCB)(int *); /* DEPRICATED. */
typedef void (*GLUTmenuStatusFCB)(int *, int *, int *);
typedef void (*GLUTselectFCB)(int *);
typedef void (*GLUTspecialFCB)(int *, int *, int *);
typedef void (*GLUTspaceMotionFCB)(int *, int *, int *);
typedef void (*GLUTspaceRotateFCB)(int *, int *, int *);
typedef void (*GLUTspaceButtonFCB)(int *, int *);
typedef void (*GLUTdialsFCB)(int *, int *);
typedef void (*GLUTbuttonBoxFCB)(int *, int *);
typedef void (*GLUTtabletMotionFCB)(int *, int *);
typedef void (*GLUTtabletButtonFCB)(int *, int *, int *, int *);
typedef void (*GLUTjoystickFCB)(unsigned int *buttonMask, int *x, int *y, int *z);
#endif

typedef struct _GLUTcolorcell GLUTcolorcell;
struct _GLUTcolorcell
{
  /* GLUT_RED, GLUT_GREEN, GLUT_BLUE */
  GLfloat component[3];
};

typedef struct _GLUTcolormap GLUTcolormap;
struct _GLUTcolormap
{
#ifdef macintosh
  CSpecArray cmap; /* Macintosh colormap ID */
#else
  Visual *visual; /* visual of the colormap */
  Colormap cmap;  /* X colormap ID */
#endif
  int refcnt;           /* number of windows using colormap */
  int size;             /* number of cells in colormap */
  int transparent;      /* transparent pixel, or -1 if opaque */
  GLUTcolorcell *cells; /* array of cells */
  GLUTcolormap *next;   /* next colormap in list */
};

typedef struct _GLUTwindow GLUTwindow;
typedef struct _GLUToverlay GLUToverlay;
struct _GLUTwindow
{
  int num; /* Small integer window id (0-based). */

  /* Window system related state. */
#ifdef macintosh
  AGLPixelFormat vis; /* visual for window */
#if TARGET_API_MAC_CARBON
  WindowRef win;
#else
  CWindowPtr win; /* X window for GLUT window */
#endif
  AGLContext ctx; /* OpenGL context GLUT glut window */
  char title[100];
  CSpecArray cmap; /* RGB colormap for window; None if CI */
#else
#if defined(_WIN32)
  int pf;  /* Pixel format. */
  HDC hdc; /* Window's Win32 device context. */
#endif
  Window win;       /* X window for GLUT window */
  GLXContext ctx;   /* OpenGL context GLUT glut window */
  XVisualInfo *vis; /* visual for window */
  Bool visAlloced;  /* if vis needs deallocate on destroy */
  Colormap cmap;    /* RGB colormap for window; None if CI */
#endif                    /*macintosh */
  GLUTcolormap *colormap; /* colormap; NULL if RGBA */
  GLUToverlay *overlay;   /* overlay; NULL if no overlay */
#ifdef macintosh
  GLboolean fullscreen; /* if game mode fullscreen is active */
  CWindowPtr renderWin; /* AGL window for rendering (might be overlay) */
  AGLContext renderCtx; /* OpenGL context for rendering (might be overlay) */
#else
#if defined(_WIN32)
  HDC renderDc; /* Win32's device context for rendering. */
#endif
  Window renderWin;     /* X window for rendering (might be
                           overlay) */
  GLXContext renderCtx; /* OpenGL context for rendering (might
                           be overlay) */
#endif /*macintosh */
  /* GLUT settable or visible window state. */
  int width;      /* window width in pixels */
  int height;     /* window height in pixels */
  int cursor;     /* cursor name */
  int visState;   /* visibility state (-1 is unknown) */
  int shownState; /* if window mapped */
  int entryState; /* entry state (-1 is unknown) */
#define GLUT_MAX_MENUS 3

  int menu[GLUT_MAX_MENUS]; /* attatched menu nums */
  /* Window relationship state. */
  GLUTwindow *parent;   /* parent window */
  GLUTwindow *children; /* list of children */
  GLUTwindow *siblings; /* list of siblings */
  /* Misc. non-API visible (hidden) state. */
  Bool treatAsSingle; /* treat this window as single-buffered
                         (it might be "fake" though) */
  Bool forceReshape;  /* force reshape before display */
#ifdef macintosh
  int button_press;
#else
#if !defined(_WIN32)
  Bool isDirect; /* if direct context (X11 only) */
#endif
#endif                  /*macintosh */
  Bool usedSwapBuffers; /* if swap buffers used last display */
  long eventMask;       /* mask of X events selected for */
  int buttonUses;       /* number of button uses, ref cnt */
  int tabletPos[2];     /* tablet position (-1 is invalid) */
  /* Work list related state. */
  unsigned int workMask;   /* mask of window work to be done */
  GLUTwindow *prevWorkWin; /* link list of windows to work on */
  Bool desiredMapState;    /* how to mapped window if on map work
                              list */
  Bool ignoreKeyRepeat;    /* if window ignores autorepeat */
  int desiredConfMask;     /* mask of desired window configuration
                            */
  int desiredX;            /* desired X location */
  int desiredY;            /* desired Y location */
  int desiredWidth;        /* desired window width */
  int desiredHeight;       /* desired window height */
  int desiredStack;        /* desired window stack */
  /* Per-window callbacks. */
  GLUTdisplayCB display;     /* redraw */
  GLUTreshapeCB reshape;     /* resize (width,height) */
  GLUTmouseCB mouse;         /* mouse (button,state,x,y) */
  GLUTmotionCB motion;       /* motion (x,y) */
  GLUTpassiveCB passive;     /* passive motion (x,y) */
  GLUTentryCB entry;         /* window entry/exit (state) */
  GLUTkeyboardCB keyboard;   /* keyboard (ASCII,x,y) */
  GLUTkeyboardCB keyboardUp; /* keyboard up (ASCII,x,y) */
#ifdef macintosh
  GLUTwindowStatusCB window_status; /* window status */
#else
  GLUTwindowStatusCB windowStatus; /* window status */
#endif
  GLUTvisibilityCB visibility;     /* visibility */
  GLUTspecialCB special;           /* special key */
  GLUTspecialCB specialUp;         /* special up key */
  GLUTbuttonBoxCB buttonBox;       /* button box */
  GLUTdialsCB dials;               /* dials */
  GLUTspaceMotionCB spaceMotion;   /* Spaceball motion */
  GLUTspaceRotateCB spaceRotate;   /* Spaceball rotate */
  GLUTspaceButtonCB spaceButton;   /* Spaceball button */
  GLUTtabletMotionCB tabletMotion; /* tablet motion */
  GLUTtabletButtonCB tabletButton; /* tablet button */
#ifdef macintosh
  GLUTmenuStateCB menu_state;
  GLUTmenuStatusCB menu_status;
  unsigned int entry_mode; /* last entry mode */
  GLUTjoystickCB joystick; /* joystick */
  int joyPollInterval;     /* joystick polling interval */
#endif                     /* macintosh */
#ifdef _WIN32
  GLUTjoystickCB joystick; /* joystick */
  int joyPollInterval;     /* joystick polling interval */
#endif
#ifdef SUPPORT_FORTRAN
  /* Special Fortran display  unneeded since no
     parameters! */
  GLUTreshapeFCB freshape;           /* Fortran reshape  */
  GLUTmouseFCB fmouse;               /* Fortran mouse  */
  GLUTmotionFCB fmotion;             /* Fortran motion  */
  GLUTpassiveFCB fpassive;           /* Fortran passive  */
  GLUTentryFCB fentry;               /* Fortran entry  */
  GLUTkeyboardFCB fkeyboard;         /* Fortran keyboard  */
  GLUTkeyboardFCB fkeyboardUp;       /* Fortran keyboard up */
  GLUTwindowStatusFCB fwindowStatus; /* Fortran visibility
                                      */
  GLUTvisibilityFCB fvisibility;     /* Fortran visibility
                                      */
  GLUTspecialFCB fspecial;           /* special key */
  GLUTspecialFCB fspecialUp;         /* special key up */
  GLUTbuttonBoxFCB fbuttonBox;       /* button box */
  GLUTdialsFCB fdials;               /* dials */
  GLUTspaceMotionFCB fspaceMotion;   /* Spaceball motion
                                      */
  GLUTspaceRotateFCB fspaceRotate;   /* Spaceball rotate
                                      */
  GLUTspaceButtonFCB fspaceButton;   /* Spaceball button
                                      */
  GLUTtabletMotionFCB ftabletMotion; /* tablet motion
                                      */
  GLUTtabletButtonFCB ftabletButton; /* tablet button
                                      */
#ifdef _WIN32
  GLUTjoystickFCB fjoystick; /* joystick */
#endif
#endif
};

struct _GLUToverlay
{
#ifdef macintosh
  AGLPixelFormat vis; /* visual for window */
  CWindowPtr win;     /* X window for GLUT window */
  AGLContext ctx;     /* OpenGL context GLUT glut window */
  char title[100];
  CSpecArray cmap; /* RGB colormap for window; None if CI */
#else
#if defined(_WIN32)
  int pf;
  HDC hdc;
#endif
  Window win;
  GLXContext ctx;
  XVisualInfo *vis; /* visual for window */
  Bool visAlloced;  /* if vis needs deallocate on destroy */
  Colormap cmap;    /* RGB colormap for window; None if CI */
#endif                    /* macintosh */
  GLUTcolormap *colormap; /* colormap; NULL if RGBA */
  int shownState;         /* if overlay window mapped */
  Bool treatAsSingle;     /* treat as single-buffered */
#ifndef macintosh
#if !defined(_WIN32)
  Bool isDirect; /* if direct context */
#endif
#endif                   /* not macintosh */
  int transparentPixel;  /* transparent pixel value */
  GLUTdisplayCB display; /* redraw  */
  /* Special Fortran display  unneeded since no
     parameters! */
};

typedef struct _GLUTstale GLUTstale;
struct _GLUTstale
{
  GLUTwindow *window;
#ifdef macintosh
  CWindowPtr win;
#else
  Window win;
#endif /* not macintosh */
  GLUTstale *next;
};

extern GLUTstale *__glutStaleWindowList;

#define GLUT_OVERLAY_EVENT_FILTER_MASK \
  (ExposureMask |                      \
   StructureNotifyMask |               \
   EnterWindowMask |                   \
   LeaveWindowMask)
#define GLUT_DONT_PROPAGATE_FILTER_MASK \
  (ButtonReleaseMask |                  \
   ButtonPressMask |                    \
   KeyPressMask |                       \
   KeyReleaseMask |                     \
   PointerMotionMask |                  \
   Button1MotionMask |                  \
   Button2MotionMask |                  \
   Button3MotionMask)
#define GLUT_HACK_STOP_PROPAGATE_MASK \
  (KeyPressMask |                     \
   KeyReleaseMask)

#ifndef macintosh
typedef struct _GLUTmenu GLUTmenu;
typedef struct _GLUTmenuItem GLUTmenuItem;
struct _GLUTmenu
{
  int id;              /* small integer menu id (0-based) */
  Window win;          /* X window for the menu */
  GLUTselectCB select; /*  function of menu */
  GLUTmenuItem *list;  /* list of menu entries */
  int num;             /* number of entries */
#if !defined(_WIN32)
  Bool managed;  /* are the InputOnly windows size
                    validated? */
  Bool searched; /* help detect menu loops */
  int pixheight; /* height of menu in pixels */
  int pixwidth;  /* width of menu in pixels */
#endif
  int submenus;              /* number of submenu entries */
  GLUTmenuItem *highlighted; /* pointer to highlighted menu
                                entry, NULL not highlighted */
  GLUTmenu *cascade;         /* currently cascading this menu  */
  GLUTmenuItem *anchor;      /* currently anchored to this entry */
  int x;                     /* current x origin relative to the
                                root window */
  int y;                     /* current y origin relative to the
                                root window */
#ifdef SUPPORT_FORTRAN
  GLUTselectFCB fselect; /*  function of menu */
#endif
};

struct _GLUTmenuItem
{
  Window win;     /* InputOnly X window for entry */
  GLUTmenu *menu; /* menu entry belongs to */
  Bool isTrigger; /* is a submenu trigger? */
  int value;      /* value to return for selecting this
                     entry; doubles as submenu id
                     (0-base) if submenu trigger */
#if defined(_WIN32)
  UINT unique; /* unique menu item id (Win32 only) */
#endif
  char *label;        /* __glutStrdup'ed label string */
  int len;            /* length of label string */
  int pixwidth;       /* width of X window in pixels */
  GLUTmenuItem *next; /* next menu entry on list for menu */
};
#endif /* not macintosh */

typedef struct _GLUTtimer GLUTtimer;
struct _GLUTtimer
{
  GLUTtimer *next; /* list of timers */
#ifdef macintosh
  double timeout; /* time to be called */
#else
  struct timeval timeout; /* time to be called */
#endif              /* macintosh */
  GLUTtimerCB func; /* timer  (value) */
  int value;        /*  return value */
#ifdef SUPPORT_FORTRAN
  GLUTtimerFCB ffunc; /* Fortran timer  */
#endif
};

#ifndef macintosh
typedef struct _GLUTeventParser GLUTeventParser;
struct _GLUTeventParser
{
  int (*func)(XEvent *);
  GLUTeventParser *next;
};

/* Declarations to implement glutFullScreen support with
   mwm/4Dwm. */

/* The following X property format is defined in Motif 1.1's
   Xm/MwmUtils.h, but GLUT should not depend on that header
   file. Note: Motif 1.2 expanded this structure with
   uninteresting fields (to GLUT) so just stick with the
   smaller Motif 1.1 structure. */
typedef struct
{
#define MWM_HINTS_DECORATIONS 2
  long flags;
  long functions;
  long decorations;
  long input_mode;
} MotifWmHints;
#endif /* not macintosh */

/* private variables from glut_event.c */
extern GLUTwindow *__glutWindowWorkList;
extern int __glutWindowDamaged;
#ifdef macintosh
extern GLUTtimer *freeTimerList;
#endif /* macintosh */
#ifdef SUPPORT_FORTRAN
extern GLUTtimer *__glutTimerList;
extern GLUTtimer *__glutNewTimer;
#endif
#ifndef macintosh
extern GLUTmenu *__glutMappedMenu;

extern void (*__glutUpdateInputDeviceMaskFunc)(GLUTwindow *);
#if !defined(_WIN32)
extern void (*__glutMenuItemEnterOrLeave)(GLUTmenuItem *item,
                                          int num, int type);
extern void (*__glutFinishMenu)(Window win, int x, int y);
extern void (*__glutPaintMenu)(GLUTmenu *menu);
extern void (*__glutStartMenu)(GLUTmenu *menu,
                               GLUTwindow *window, int x, int y, int x_win, int y_win);
extern GLUTmenu *(*__glutGetMenuByNum)(int menunum);
extern GLUTmenuItem *(*__glutGetMenuItem)(GLUTmenu *menu,
                                          Window win, int *which);
extern GLUTmenu *(*__glutGetMenu)(Window win);
#endif
#endif /* not macintosh */

/* private variables from glut_init.c */
#ifdef macintosh
extern Boolean gQuit;
#endif /* macintosh */
#ifndef macintosh
extern Atom __glutWMDeleteWindow;
extern Display *__glutDisplay;
#endif /* not macintosh */
extern unsigned int __glutDisplayMode;
extern char *__glutDisplayString;
#ifndef macintosh
extern XVisualInfo *(*__glutDetermineVisualFromString)(char *string, Bool *treatAsSingle,
                                                       Criterion *requiredCriteria, int nRequired, int requiredMask, void **fbc);
#endif /* not macintosh */
extern GLboolean __glutDebug;
#ifndef macintosh
extern GLboolean __glutForceDirect;
extern GLboolean __glutIconic;
extern GLboolean __glutTryDirect;
extern Window __glutRoot;
extern XSizeHints __glutSizeHints;
#endif /* not macintosh */
extern char **__glutArgv;
extern char *__glutProgramName;
extern int __glutArgc;
#ifndef macintosh
extern int __glutConnectionFD;
#endif /* not macintosh */
extern int __glutInitHeight;
extern int __glutInitWidth;
extern int __glutInitX;
extern int __glutInitY;
#ifndef macintosh
extern int __glutScreen;
#endif /* not macintosh */
extern int __glutScreenHeight;
extern int __glutScreenWidth;
#ifndef macintosh
extern Atom __glutMotifHints;
#endif /* not macintosh */
extern unsigned int __glutModifierMask;

/* private variables from glut_menu.c */
#ifdef macintosh
extern void (*__glutMenuStatusFunc)(int, int, int);
#else
extern GLUTmenuItem *__glutItemSelected;
extern GLUTmenu **__glutMenuList;
extern void (*__glutMenuStatusFunc)(int, int, int);
extern void __glutMenuModificationError(void);
extern void __glutSetMenuItem(GLUTmenuItem *item,
                              const char *label, int value, Bool isTrigger);
#endif /* macintosh */

/* private variables from glut_win.c */
extern GLUTwindow **__glutWindowList;
extern GLUTwindow *__glutCurrentWindow;
#ifndef macintosh
extern GLUTwindow *__glutMenuWindow;
extern GLUTmenu *__glutCurrentMenu;
#endif /* not macintosh */
extern int __glutWindowListSize;
#ifdef macintosh
extern void __glutWindowReshape(GLUTwindow *gwindow, int width, int height);
#else
extern void (*__glutFreeOverlayFunc)(GLUToverlay *);
extern XVisualInfo *__glutDetermineWindowVisual(Bool *treatAsSingle,
                                                Bool *visAlloced, void **fbc);
#endif /* macintosh */

/* private variables from glut_mesa.c */
extern int __glutMesaSwapHackSupport;

/* private variables from glut_gamemode.c */
extern GLUTwindow *__glutGameModeWindow;

/* private routines from glut_cindex.c */
#ifdef macintosh
extern GLUTcolormap *__glutAssociateColormap(AGLPixelFormat vis);
#else
extern GLUTcolormap *__glutAssociateNewColormap(XVisualInfo *vis);
#endif /* macintosh */
extern void __glutFreeColormap(GLUTcolormap *);

#ifndef macintosh
/* private routines from glut_cmap.c */
extern void __glutSetupColormap(
    XVisualInfo *vi,
    GLUTcolormap **colormap,
    Colormap *cmap);
#if !defined(_WIN32)
extern void __glutEstablishColormapsProperty(
    GLUTwindow *window);
extern GLUTwindow *__glutToplevelOf(GLUTwindow *window);
#endif

/* private routines from glut_cursor.c */
extern void __glutSetCursor(GLUTwindow *window);

/* private routines from glut_event.c */
extern void __glutPutOnWorkList(GLUTwindow *window,
                                int work_mask);
extern void __glutRegisterEventParser(GLUTeventParser *parser);
#endif /* not macintosh */
#ifdef macintosh
extern void __glutMoveWindow(GLUTwindow *glut_win, Point pntDelta);
extern void __glutSelectWindowHierarchy(GLUTwindow *glut_win);
extern void updateWindowState(GLUTwindow *window, int visState);
#endif /* macintosh */
extern void __glutPostRedisplay(GLUTwindow *window, int layerMask);

/* private routines from glut_init.c */
#ifdef macintosh
extern void __glutInitTime(double *beginning);
extern double __glutTime(void);
#else
#if !defined(_WIN32)
extern void __glutOpenXConnection(char *display);
#else
extern void __glutOpenWin32Connection(char *display);
#endif
extern void __glutInitTime(struct timeval *beginning);
#endif /* macintosh */

/* private routines for glut_menu.c (or win32_menu.c) */
#ifdef macintosh
extern void __glutMenuKeyCommand(void);
extern void __glutBuildMenuBar(void);
extern void __glutMenuCommand(short menuNum, short itemNum);
extern int __glutContextualMenuSelect(int x, int y, int button, short *menuID, unsigned short *menuItem);
#else
#if defined(_WIN32)
extern GLUTmenu *__glutGetMenu(Window win);
extern GLUTmenu *__glutGetMenuByNum(int menunum);
extern GLUTmenuItem *__glutGetMenuItem(GLUTmenu *menu,
                                       Window win, int *which);
extern void __glutStartMenu(GLUTmenu *menu,
                            GLUTwindow *window, int x, int y, int x_win, int y_win);
extern void __glutFinishMenu(Window win, int x, int y);
#endif
extern void __glutSetMenu(GLUTmenu *menu);
#endif /* macintosh */

/* private routines from glut_util.c */
extern char *__glutStrdup(const char *string);
extern void __glutWarning(char *format, ...);
extern void __glutFatalError(char *format, ...);
extern void __glutFatalUsage(char *format, ...);

/* private routines from glut_win.c */
extern void __glutChangeWindowEventMask(long mask, Bool add);
#ifdef macintosh
extern GLUTwindow *__glutGetWindow(CWindowPtr win);
extern void __glutRemoveFromWorkList(GLUTwindow *window);
extern void __glutEstablishColormapsProperty(
    GLUTwindow *window);
extern int __glutFindVisState(WindowPtr pWindow);
extern AGLPixelFormat __glutDetermineVisual(
#else
extern GLUTwindow *__glutGetWindow(Window win);
extern void __glutChangeWindowEventMask(long mask, Bool add);
extern XVisualInfo *__glutDetermineVisual(
#endif /* macintosh */
    unsigned int mode,
    Bool *fakeSingle,
#ifdef macintosh
    AGLPixelFormat(getVisualInfo)(unsigned int));
extern AGLPixelFormat __glutGetVisualInfo(unsigned int mode);
extern int __glutSetCurrentWindow(GLUTwindow *window);
extern void __glutDefaultReshape(int, int);

extern GLboolean __glutPointInWindow(GLUTwindow *window, Point *point);
extern void __glutCToStr255(const char *cs, Str255 ps);

extern void __glutSetupColormap(
    AGLPixelFormat vi,
    GLUTcolormap **colormap,
    CSpecArray *cmap);

extern void __glutRebuildMenuBar(void);
extern int __glutGetCurNumMenuItems(void);

extern void __glutDefaultDisplay(void);
extern void __glutCToPStr(const char *cs, Str255 ps);

extern void __glutInit(void);

extern GLUTwindow *__glutCreateWindow(GLUTwindow *parent, const char *title, int x, int y, int width, int height, int win_type);
extern void __glutDestroyWindow(GLUTwindow *window, GLUTwindow *initialWindow);

/* private routines from glut_ext.c */
extern int __glutIsSupportedByAGL(const char *);

extern void __glutPutOnWorkList(GLUTwindow *window, int workMask);

// extern char * __glutStrdup(const char *string);
extern AGLPixelFormat __glutVisualInfoFromString(char *string);
#else
    XVisualInfo *(getVisualInfo)(unsigned int));
extern XVisualInfo *__glutGetVisualInfo(unsigned int mode);
extern void __glutSetWindow(GLUTwindow *window);
extern void __glutReshapeFunc(GLUTreshapeCB reshapeFunc,
                              int callingConvention);
extern void __glutDefaultReshape(int, int);
extern GLUTwindow *__glutCreateWindow(
    GLUTwindow *parent,
    int x, int y, int width, int height, int gamemode);
extern void __glutDestroyWindow(
    GLUTwindow *window,
    GLUTwindow *initialWindow);

#if !defined(_WIN32)
/* private routines from glut_glxext.c */
extern int __glutIsSupportedByGLX(char *);
#endif
#endif /* macintosh */

/* private routines from glut_input.c */
extern void __glutUpdateInputDeviceMask(GLUTwindow *window);

/* private routines from glut_mesa.c */
extern void __glutDetermineMesaSwapHackSupport(void);

/* private routines from glut_gameglut.c */
extern void __glutCloseDownGameMode(void);
#ifdef macintosh
extern void __glutSuspendGameMode(void);
extern void __glutResumeGameMode(void);
extern void __glutSetGameModeGWorld(void);
#endif /* macintosh */

#ifndef macintosh
#if defined(_WIN32)
/* private routines from win32_*.c */
extern LONG WINAPI __glutWindowProc(HWND win, UINT msg, WPARAM w, LPARAM l);
extern HDC XHDC;
#endif
#endif /* not macintosh */

#endif /* __glutint_h__ */
