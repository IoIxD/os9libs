/*	File:		glut_device.c	Contains:		Written by:	Mark J. Kilgard	Mac OS by:	John Stauffer and Geoff Stahl	Copyright:	Copyright � Mark J. Kilgard, 1996.					MacOS portions Copyright � Apple Computer, Inc., 1999	Change History (most recent first):        <3+>     8/31/99    ggs     Add 3.7 key repeat query support         <3>     7/16/99    ggs     Fixed attributions and copyrights         <2>     5/31/99    ggs     remove extranious includes         <1>     5/23/99    GGS     OpenGL glut original source*//* Copyright � Mark J. Kilgard, 1996. *//* This program is freely distributable without licensing fees   and is provided without guarantee or warrantee expressed or   implied. This program is -not- in the public domain. */#include <stdlib.h>#include <stdarg.h>#include <stdio.h>#include "glutint.h"// ggs: parts of glut_input.c from 3.7int __glutNumDials = 0;int __glutNumSpaceballButtons = 0;int __glutNumButtonBoxButtons = 0;int __glutNumTabletButtons = 1;int __glutNumMouseButtons = 3;  /* we emulate all three */void *__glutTablet = NULL;		// used to be XDevice *void *__glutDials = NULL;		// "void *__glutSpaceball = NULL; // "int __glutHasJoystick = 0;int __glutNumJoystickButtons = 0;int __glutNumJoystickAxes = 0;int glutDeviceGet(GLenum type){	switch(type)	{/*		case GLUT_HAS_KEYBOARD:			return GL_TRUE;		break;		case GLUT_HAS_MOUSE:			return GL_TRUE;		break;		case GLUT_HAS_SPACEBALL:			return GL_FALSE;		break;		case GLUT_HAS_DIAL_AND_BUTTON_BOX:			return GL_FALSE;		break;		case GLUT_HAS_TABLET:			return GL_FALSE;		break;		case GLUT_NUM_MOUSE_BUTTONS:			return 1;		break;		case GLUT_NUM_SPACEBALL_BUTTONS:			return 0;		break;		case GLUT_NUM_BUTTON_BOX_BUTTONS:			return 0;		break;		case GLUT_NUM_DIALS:			return 0;		break;		case GLUT_NUM_TABLET_BUTTONS:			return 0;		break;*/  			case GLUT_HAS_KEYBOARD:		case GLUT_HAS_MOUSE:		 /* Assume window system always has mouse and keyboard. */		 return 1;		case GLUT_HAS_SPACEBALL:		 	return __glutSpaceball != NULL;		case GLUT_HAS_DIAL_AND_BUTTON_BOX:		 	return __glutDials != NULL;		case GLUT_HAS_TABLET:		 	return __glutTablet != NULL;		case GLUT_NUM_MOUSE_BUTTONS:		 	return __glutNumMouseButtons;		case GLUT_NUM_SPACEBALL_BUTTONS:		 	return __glutNumSpaceballButtons;		case GLUT_NUM_BUTTON_BOX_BUTTONS:		 	return __glutNumButtonBoxButtons;		case GLUT_NUM_DIALS:			return __glutNumDials;		case GLUT_NUM_TABLET_BUTTONS:			return __glutNumTabletButtons;		case GLUT_DEVICE_IGNORE_KEY_REPEAT:    		return __glutCurrentWindow->ignoreKeyRepeat;		break;		case GLUT_DEVICE_KEY_REPEAT:		/* Win32 cannot globally disable key repeat. */			return GLUT_KEY_REPEAT_ON;		case GLUT_JOYSTICK_POLL_RATE:			return __glutCurrentWindow->joyPollInterval;		case GLUT_HAS_JOYSTICK:			return __glutHasJoystick;		case GLUT_JOYSTICK_BUTTONS:			return __glutNumJoystickButtons;		case GLUT_JOYSTICK_AXES:			return __glutNumJoystickAxes;		default:			__glutWarning("invalid glutDeviceGet parameter: %d", type);			return -1;	}}